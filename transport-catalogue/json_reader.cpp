#include "json_reader.h"

#include <algorithm>
#include <sstream>
#include <vector>

using namespace std::literals;

namespace transport {

void FillTransportCatalogue(TransportCatalogue& db, const json::Document& doc) {
    auto base_requests(doc.GetRoot().AsMap().at("base_requests").AsArray());

    //Создаем остановки 
    for (const auto& request : base_requests) {
        auto request_map(request.AsMap());
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
        auto request_map(base_request.AsMap());
        if (request_map.at("type").AsString() != "Stop"s) {
            continue;
        }
        
        auto road_distances(request_map.at("road_distances").AsMap());
        for (const auto& [stop_id, distance] : road_distances) {
            db.SetStopDistance(db.GetStop(request_map.at("name").AsString()),
                               db.GetStop(stop_id),
                               distance.AsInt());
        }
    }

    //Создаем маршруты
    for (const auto& base_request : base_requests) {
        auto request_map(base_request.AsMap());
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
}

json::Document ExecuteStatRequests(const RequestHandler& request_handler, const json::Document& doc) {
    auto stat_requests(doc.GetRoot().AsMap().at("stat_requests").AsArray());

    // Обрабатываем запросы
    json::Array request_results;
    request_results.reserve(stat_requests.size());
    for (const auto& request : stat_requests) {
        auto request_map(request.AsMap());

        // Результат запроса
        json::Dict request_result;
        request_result["request_id"] = json::Node(request_map.at("id").AsInt());

        // Обрабатываем запрос взависимости от типа запроса
        if (request_map.at("type").AsString() == "Bus"s) {
            auto bus_stat(request_handler.GetBusStat(request_map.at("name").AsString()));
            if (bus_stat) {
                request_result["curvature"] = json::Node((*bus_stat).curvature);
                request_result["route_length"] = json::Node((*bus_stat).route_length);
                request_result["stop_count"] = json::Node((*bus_stat).stop_count);
                request_result["unique_stop_count"] = json::Node((*bus_stat).unique_stop_count);
            } else {
                request_result["error_message"] = json::Node("not found"s);
            }
        } else if (request_map.at("type").AsString() == "Stop"s) {
            auto stop_stat(request_handler.GetStopStat(request_map.at("name").AsString()));
            if (stop_stat) {
                json::Array bus_names;
                bus_names.reserve((*stop_stat).bus_names.size());
                for (auto& bus_name : (*stop_stat).bus_names) {
                    bus_names.push_back(json::Node(bus_name));
                }
                request_result["buses"] = json::Node(std::move(bus_names));
            } else {
                request_result["error_message"] = json::Node("not found"s);
            }
        } else if (request_map.at("type").AsString() == "Map"s) {
            std::ostringstream out; 
            request_handler.RenderMap().Render(out);
            request_result["map"] = json::Node(out.str());
        }
        
        /*
            WHANT_TO_KNOW: Почему нельзя написать так:
            request_results.push_back(json::Node(std::move(request_result)));
        */
        json::Node request_result_node(std::move(request_result));
        request_results.push_back(request_result_node);
    }

    return json::Document(json::Node(request_results));
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
    auto settings_map(doc.GetRoot().AsMap().at("render_settings").AsMap());
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
