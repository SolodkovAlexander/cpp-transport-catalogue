#include "graph.h"
#include "json_builder.h"
#include "json_reader.h"

#include <algorithm>
#include <sstream>
#include <vector>

using namespace std::literals;

namespace transport {

void FillTransportCatalogue(TransportCatalogue& db, const json::Document& doc) {
    auto base_requests(doc.GetRoot().AsDict().at("base_requests").AsArray());

    //Создаем остановки 
    for (const auto& request : base_requests) {
        auto request_map(request.AsDict());
        if (request_map.at("type").AsString() != "Stop"s) {
            continue;
        }
        db.AddStop(request_map.at("name").AsString(),
                   {
                       request_map.at("latitude").AsDouble(),
                       request_map.at("longitude").AsDouble()
                   });
    }

    //Определяем расстояния между остановками
    for (const auto& base_request : base_requests) {
        auto request_map(base_request.AsDict());
        if (request_map.at("type").AsString() != "Stop"s) {
            continue;
        }
        
        auto road_distances(request_map.at("road_distances").AsDict());
        for (const auto& [stop_id, distance] : road_distances) {
            db.SetStopDistance(db.GetStop(request_map.at("name").AsString()),
                               db.GetStop(stop_id),
                               distance.AsInt());
        }
    }

    //Создаем маршруты
    for (const auto& base_request : base_requests) {
        auto request_map(base_request.AsDict());
        if (request_map.at("type").AsString() != "Bus"s) {
            continue;
        }

        //Получаем остановки
        auto stops(request_map.at("stops").AsArray());
        std::vector<StopPtr> route;
        route.reserve(stops.size());
        for (const auto& stop_id : stops) {
            auto stop(db.GetStop(stop_id.AsString()));
            route.push_back(stop);
        }

        //Получаем маршрут из остановок
        std::vector<StopPtr> route_stops(route.begin(), route.end());
        bool is_roundtrip(request_map.at("is_roundtrip").AsBool());
        if (!is_roundtrip) {
            route_stops.insert(route_stops.end(), std::next(route.rbegin()), route.rend());
        }       

        //Создаем маршрут  
        db.AddBus(request_map.at("name").AsString(), 
                  std::move(route_stops),
                  is_roundtrip);
    }

    // Устанавливаем общие настройки маршрутов
    static constexpr double km_to_m_modifier = 1000.0 / 60.0;
    auto routing_settings(doc.GetRoot().AsDict().at("routing_settings").AsDict());
    db.SetRoutingSettings(RoutingSettings{routing_settings.at("bus_wait_time").AsInt(),
                                          routing_settings.at("bus_velocity").AsDouble() * km_to_m_modifier});
}

json::Document ExecuteStatRequests(const RequestHandler& request_handler, const json::Document& doc) {
    auto stat_requests(doc.GetRoot().AsDict().at("stat_requests").AsArray());

    // Обрабатываем запросы
    json::Builder request_results;
    request_results.StartArray();
    for (const auto& request : stat_requests) {
        auto request_map(request.AsDict());

        // Результат запроса
        json::Builder request_result;
        request_result.StartDict()
                      .Key("request_id").Value(request_map.at("id").AsInt());

        // Обрабатываем запрос взависимости от типа запроса
        if (request_map.at("type").AsString() == "Bus"s) {
            auto bus_stat(request_handler.GetBusStat(request_map.at("name").AsString()));
            if (bus_stat) {
                request_result.Key("curvature").Value((*bus_stat).curvature)
                              .Key("route_length").Value((*bus_stat).route_length)
                              .Key("stop_count").Value((*bus_stat).stop_count)
                              .Key("unique_stop_count").Value((*bus_stat).unique_stop_count);
            } else {
                request_result.Key("error_message").Value("not found"s);
            }
        } else if (request_map.at("type").AsString() == "Stop"s) {
            auto stop_stat(request_handler.GetStopStat(request_map.at("name").AsString()));
            if (stop_stat) {
                json::Builder bus_names;
                bus_names.StartArray();
                for (auto& bus_name : (*stop_stat).bus_names) {
                    bus_names.Value(bus_name);
                }
                request_result.Key("buses").Value(bus_names.EndArray()
                                                           .Build().GetValue());
            } else {
                request_result.Key("error_message").Value("not found"s);
            }
        } else if (request_map.at("type").AsString() == "Map"s) {
            std::ostringstream out; 
            request_handler.RenderMap().Render(out);
            request_result.Key("map").Value(out.str());
        } else if (request_map.at("type").AsString() == "Route"s) {
            auto route_stat = request_handler.GetRouteStat(request_map.at("from").AsString(), 
                                                           request_map.at("to").AsString());
            if (route_stat) {
                json::Builder items;
                items.StartArray();
                for (const auto& item : (*route_stat).items) {
                    json::Builder item_as_dict;
                    item_as_dict.StartDict();
                    if (holds_alternative<RouteStat::WaitingOnStopItem>(item)) {
                        item_as_dict
                            .Key("stop_name").Value(std::string(get<RouteStat::WaitingOnStopItem>(item).name))
                            .Key("time").Value((*route_stat).bus_wait_time)
                            .Key("type").Value("Wait"s);
                    } else {
                        const auto& bus_item = get<RouteStat::BusItem>(item);
                        item_as_dict
                            .Key("bus").Value(std::string(bus_item.name))
                            .Key("span_count").Value(bus_item.span_count)
                            .Key("time").Value(bus_item.time)
                            .Key("type").Value("Bus"s);
                    }
                    items.Value(item_as_dict
                                .EndDict()
                                .Build().GetValue());
                }
                request_result
                    .Key("total_time").Value((*route_stat).total_time)
                    .Key("items").Value(items
                                        .EndArray()
                                        .Build().GetValue());
            } else {
                request_result.Key("error_message").Value("not found"s);
            }
        }

        request_results.Value(std::move(request_result
                                        .EndDict()
                                        .Build().GetValue()));
    }

    return json::Document(request_results
                          .EndArray()
                          .Build());
}

}  // namespace transport

namespace renderer {

namespace utils {
    svg::Color JSONNodeToColor(const json::Node& node) {
        if (node.IsString()) {
            return node.AsString();
        }
        if (node.IsArray()) {
            auto node_array(node.AsArray());
            if (node_array.size() == 3) {
                return svg::Rgb{
                    static_cast<uint8_t>(node_array.at(0).AsInt()),
                    static_cast<uint8_t>(node_array.at(1).AsInt()),
                    static_cast<uint8_t>(node_array.at(2).AsInt())
                };
            } else if (node_array.size() == 4) {
                return svg::Rgba{
                    static_cast<uint8_t>(node_array.at(0).AsInt()),
                    static_cast<uint8_t>(node_array.at(1).AsInt()),
                    static_cast<uint8_t>(node_array.at(2).AsInt()),
                    node_array.at(3).AsDouble()
                };
            }
        }
        return {};
    }

    std::vector<svg::Color> JSONNodeToColors(const json::Node& node) {
        std::vector<svg::Color> colors;
        colors.reserve(node.AsArray().size());
        for (const auto& node_color : node.AsArray()) {
            colors.push_back(JSONNodeToColor(node_color));
        }
        return colors;
    }
}  // namespace renderer::utils

void FillMapRenderer(MapRenderer& map_renderer, const json::Document & doc) {
    auto settings_map(doc.GetRoot().AsDict().at("render_settings").AsDict());
    MapRenderer::RenderSettings settings{
        settings_map.at("width").AsDouble(),
        settings_map.at("height").AsDouble(),
        settings_map.at("padding").AsDouble(),
        settings_map.at("line_width").AsDouble(),
        settings_map.at("stop_radius").AsDouble(),
        static_cast<uint32_t>(settings_map.at("bus_label_font_size").AsInt()),
        {
            settings_map.at("bus_label_offset").AsArray().at(0).AsDouble(),
            settings_map.at("bus_label_offset").AsArray().at(1).AsDouble()
        },
        static_cast<uint32_t>(settings_map.at("stop_label_font_size").AsInt()),
        {
            settings_map.at("stop_label_offset").AsArray().at(0).AsDouble(),
            settings_map.at("stop_label_offset").AsArray().at(1).AsDouble()
        },
        utils::JSONNodeToColor(settings_map.at("underlayer_color")),
        settings_map.at("underlayer_width").AsDouble(),
        utils::JSONNodeToColors(settings_map.at("color_palette"))
    };
    map_renderer.SetRenderSettings(std::move(settings));
}

}  // namespace renderer
