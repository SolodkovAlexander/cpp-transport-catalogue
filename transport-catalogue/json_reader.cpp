#include "json_reader.h"
#include "request_handler.h"

#include <algorithm>

using namespace std::literals;

namespace transport {

void FillTransportCatalogue(TransportCatalogue& db, 
                            const json::Document& doc) {
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
        if (!request_map.at("is_roundtrip").AsBool()) {
            route_stops.insert(route_stops.end(), std::next(route.rbegin()), route.rend());
        }       

        //Создаем маршрут  
        db.AddBus(request_map.at("name").AsString(), 
                  std::move(route_stops));
    }
}

json::Document ExecuteStatRequests(const TransportCatalogue& db, 
                                   const json::Document& doc) {
    auto stat_requests(doc.GetRoot().AsMap().at("stat_requests").AsArray());

    // Обработчик запросов
    renderer::MapRenderer map_renderer;
    RequestHandler request_handler(db, map_renderer);

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