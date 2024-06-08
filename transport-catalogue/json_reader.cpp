#include "json_reader.h"
#include "request_handler.h"

#include <cassert>

using namespace std::literals;

namespace transport {

void FillTransportCatalogue(TransportCatalogue& db, 
                            const json::Document& doc) {
    assert(doc.GetRoot().IsMap());

    auto doc_as_map(doc.GetRoot().AsMap());
    assert(doc_as_map.count(JSON_BASE_REQUESTS_KEY) && doc_as_map.at(JSON_BASE_REQUESTS_KEY).IsArray());

    auto base_requests(doc_as_map.at(JSON_BASE_REQUESTS_KEY).AsArray());

    //Создаем остановки 
    for (const auto& request : base_requests) {
        assert(request.IsMap());

        auto request_map(request.AsMap());
        assert(request_map.count("type"s) && request_map.at("type"s).IsString());
        if (request_map.at("type").AsString() != "Stop"s) {
            continue;
        }

        assert(request_map.count("name"s) && request_map.at("name"s).IsString());
        assert(request_map.count("latitude"s) && request_map.at("latitude"s).IsDouble());
        assert(request_map.count("longitude"s) && request_map.at("longitude"s).IsDouble());
        assert(request_map.count("road_distances"s) && request_map.at("road_distances"s).IsMap());
        db.AddStop(request_map.at("name"s).AsString(),
                   {
                       request_map.at("latitude"s).AsDouble(),
                       request_map.at("longitude"s).AsDouble()
                   });
    }

    //Определяем расстояния между остановками
    for (const auto& base_request : base_requests) {
        auto request_map(base_request.AsMap());
        if (request_map.at("type").AsString() != "Stop"s) {
            continue;
        }
        
        auto road_distances(request_map.at("road_distances"s).AsMap());
        for (const auto& [stop_id, distance] : road_distances) {
            assert(distance.IsInt());
            StopPtr stop_to(db.GetStop(stop_id));
            assert(stop_to);
            db.SetStopDistance(db.GetStop(request_map.at("name"s).AsString()),
                               stop_to,
                               distance.AsInt());
        }
    }

    //Создаем маршруты
    for (const auto& base_request : base_requests) {
        auto request_map(base_request.AsMap());
        if (request_map.at("type").AsString() != "Bus"s) {
            continue;
        }
        
        assert(request_map.count("name"s) && request_map.at("name"s).IsString());
        assert(request_map.count("stops"s) && request_map.at("stops"s).IsArray());
        assert(request_map.count("is_roundtrip"s) && request_map.at("is_roundtrip"s).IsBool());

        //Получаем остановки
        auto stops(request_map.at("stops"s).AsArray());
        std::vector<StopPtr> route;
        route.reserve(stops.size());
        for (const auto& stop_id : stops) {
            assert(stop_id.IsString());
            auto stop(db.GetStop(stop_id.AsString()));
            assert(stop);
            route.push_back(stop);
        }

        //Получаем маршрут из остановок
        std::vector<StopPtr> route_stops(route.begin(), route.end());
        if (!request_map.at("is_roundtrip"s).AsBool()) {
            route_stops.insert(route_stops.end(), std::next(route.rbegin()), route.rend());
        }       

        //Создаем маршрут  
        db.AddBus(request_map.at("name"s).AsString(), 
                  std::move(route_stops));
    }
}

json::Document ExecuteStatRequests(const TransportCatalogue& db, 
                                   const json::Document& doc) {
    assert(doc.GetRoot().IsMap());

    auto doc_as_map(doc.GetRoot().AsMap());
    assert(doc_as_map.count(JSON_STAT_REQUESTS_KEY) && doc_as_map.at(JSON_STAT_REQUESTS_KEY).IsArray());

    // Обработчик запросов
    renderer::MapRenderer map_renderer;
    RequestHandler request_handler(db, map_renderer);

    // Обрабатываем запросы
    auto stat_requests(doc_as_map.at(JSON_STAT_REQUESTS_KEY).AsArray());
    json::Array request_results;
    request_results.reserve(stat_requests.size());
    for (const auto& request : stat_requests) {
        assert(request.IsMap());

        auto request_map(request.AsMap());
        assert(request_map.count("id"s) && request_map.at("id"s).IsInt());
        assert(request_map.count("type"s) && request_map.at("type"s).IsString());

        // Результат запроса
        json::Dict request_result;
        request_result["request_id"s] = json::Node(request_map.at("id"s).AsInt());

        // Обрабатываем запрос взависимости от типа запроса
        if (request_map.at("type").AsString() == "Bus"s) {
            assert(request_map.count("name"s) && request_map.at("name"s).IsString());
            auto bus_stat(request_handler.GetBusStat(request_map.at("name"s).AsString()));
            if (bus_stat) {
                request_result["curvature"s] = json::Node((*bus_stat).curvature);
                request_result["route_length"s] = json::Node((*bus_stat).route_length);
                request_result["stop_count"s] = json::Node((*bus_stat).stop_count);
                request_result["unique_stop_count"s] = json::Node((*bus_stat).unique_stop_count);
            } else {
                request_result["error_message"s] = "not found"s;
            }
        } else if (request_map.at("type").AsString() == "Stop"s) {
            assert(request_map.count("name"s) && request_map.at("name"s).IsString());
            auto stop_stat(request_handler.GetStopStat(request_map.at("name"s).AsString()));
            if (stop_stat) {
                json::Array buses;
                buses.reserve((*stop_stat).bus_names.size());
                for (const auto& bus_name : (*stop_stat).bus_names) {
                    buses.push_back(json::Node(bus_name));
                }
                request_result["buses"s] = buses;
            } else {
                request_result["error_message"s] = "not found"s;
            }
        }
        
        json::Node request_result_node(std::move(request_result));
        request_results.push_back(request_result_node);
    }

    return json::Document(request_results);
}

}  // namespace transport