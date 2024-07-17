#pragma once

#include "graph.h"
#include "map_renderer.h"
#include "router.h"
#include "svg.h"
#include "transport_catalogue.h"

#include <set>

/*
 * Код обработчика запросов к базе, содержащего логику, которую не
 * хотелось бы помещать ни в transport_catalogue, ни в json reader.
 */

// Класс RequestHandler играет роль Фасада, упрощающего взаимодействие JSON reader-а
// с другими подсистемами приложения.
// См. паттерн проектирования Фасад: https://ru.wikipedia.org/wiki/Фасад_(шаблон_проектирования)

struct BusStat {
    double curvature = 0.0;
    int route_length = 0;
    int stop_count = 0;
    int unique_stop_count = 0;
};

struct StopStat {
    std::set<std::string> bus_names;
};

struct RouteStat {
    struct WaitingOnStopItem {
        std::string_view name;
    };
    struct BusItem {
        std::string_view name;
        int span_count = 0;
        double time = 0.0;
    };

    int bus_wait_time = 0.0;
    double total_time = 0.0;
    std::vector<std::variant<WaitingOnStopItem, BusItem>> items;
};
using RouteStatItem = std::variant<RouteStat::WaitingOnStopItem, RouteStat::BusItem>;

class RequestHandler {
public:
    // MapRenderer понадобится в следующей части итогового проекта
    RequestHandler(const transport::TransportCatalogue& db, const renderer::MapRenderer& renderer);

    // Возвращает информацию о маршруте (запрос Bus)
    std::optional<BusStat> GetBusStat(std::string_view bus_name) const;

    // Возвращает информацию о маршруте (запрос Bus)
    std::optional<StopStat> GetStopStat(std::string_view stop_name) const;

    // Возвращает информацию о прохождении маршрута (запрос Route)
    std::optional<RouteStat> GetRouteStat(std::string_view stop_name_from, std::string_view stop_name_to) const;

    // Рендерит транспортный каталог
    svg::Document RenderMap() const;


private:
    template <typename GraphEdgeWeight>
    class RouteRequestHelper {
    public:
        RouteRequestHelper(const transport::TransportCatalogue& db) : 
            db_(db) {
            InitGraph();
            router_ = std::make_unique<graph::Router<GraphEdgeWeight>>(*graph_);
        }

        std::optional<RouteStat> GetMinTimeRoute(std::string_view stop_name_from, std::string_view stop_name_to) const {
            auto route = router_->BuildRoute(stop_to_vertex_info_.at(db_.GetStop(stop_name_from)).waiting_bus_vertex_id, 
                                             stop_to_vertex_info_.at(db_.GetStop(stop_name_to)).waiting_bus_vertex_id);
            if (!route) {
                return std::nullopt;
            }

            RouteStat route_stat{ 
                db_.GetRoutingSettings().bus_wait_time,
                (*route).weight,
                {}
            };
            route_stat.items.reserve((*route).edges.size());

            const auto& stops = db_.GetStops();
            for (auto edge_id_it = (*route).edges.begin(); edge_id_it != (*route).edges.end(); ++edge_id_it) {
                auto edge_id = *edge_id_it;
                if (bool is_bus_wait_edge = edge_id < stops.size()) {
                    route_stat.items.push_back(RouteStat::WaitingOnStopItem{stops.at(edge_id)->id});
                } else {
                    const auto& edge = graph_->GetEdge(edge_id);
                    auto [bus, span_count] = edge_to_bus_info_.at(edge_id);
                    route_stat.items.push_back(RouteStat::BusItem{ bus->id, span_count, edge.weight });
                }
            }
            return route_stat;
        }

    private:
    struct StopGraphVertexInfo {
        graph::VertexId waiting_bus_vertex_id;
        graph::VertexId stop_vertex_id;
    };
    struct GraphEdgeBusInfo {
        transport::BusPtr bus;
        int span_count;
    };

    private:
        void InitGraph() {
            const auto& stops = db_.GetStops();

            // Количество вершин = остановки и точки ожидания автобуса на остановках
            graph_ = std::make_unique<graph::DirectedWeightedGraph<GraphEdgeWeight>>(stops.size() * 2);

            const auto& routing_settings = db_.GetRoutingSettings();
            for (size_t i = 0; i < stops.size(); ++i) {
                // Определяем идентификаторы вершин графа: точки остановок и доп. точки ожидания автобуса на остановках
                stop_to_vertex_info_[stops.at(i)] = StopGraphVertexInfo{ stops.size() + i, i };
                // Добавляем ребро: от точки ожидания автобуса до точки остановки
                graph_->AddEdge(graph::Edge<GraphEdgeWeight>{
                    stops.size() + i,
                    i,
                    static_cast<GraphEdgeWeight>(routing_settings.bus_wait_time)
                });
            }

            // Идем по маршрутам
            const auto& buses = db_.GetBuses();
            const auto& roundtrip_buses = db_.GetRoundtripBuses();
            for (auto bus : buses) {
                if (roundtrip_buses.count(bus)) {
                    AddBusEdgesByStop(bus->stops.begin(), 
                                      bus->stops.begin() + 1, 
                                      bus->stops.end(),
                                      bus,
                                      routing_settings.bus_velocity);
                } else {
                    AddBusEdgesByStop(bus->stops.begin(), 
                                      bus->stops.begin() + 1, 
                                      bus->stops.begin() + (bus->stops.size() + 1) / 2,
                                      bus,
                                      routing_settings.bus_velocity);
                    AddBusEdgesByStop(bus->stops.begin() + bus->stops.size() / 2, 
                                      bus->stops.begin() + bus->stops.size() / 2 + 1, 
                                      bus->stops.end(),
                                      bus,
                                      routing_settings.bus_velocity);
                }
            }
        }

        template <typename StopIt>
        void AddBusEdgesByStop(StopIt internal_from, StopIt to_start, StopIt to_end,
                               transport::BusPtr bus,
                               GraphEdgeWeight bus_velocity) {
            GraphEdgeWeight edge_weight{};
            auto from_vertex_id = stop_to_vertex_info_.at(*internal_from).stop_vertex_id;
            int span_count = 0;
            for (auto from = internal_from, to = to_start; to != to_end; ++from, ++to) {
                if (*to == *internal_from) {
                    continue;
                }
                edge_weight += static_cast<GraphEdgeWeight>(db_.GetStopDistance(*from, *to)) / bus_velocity;
                auto edgeId = graph_->AddEdge(graph::Edge<GraphEdgeWeight>{ 
                    from_vertex_id, 
                    stop_to_vertex_info_.at(*to).waiting_bus_vertex_id, 
                    edge_weight 
                });
                edge_to_bus_info_[edgeId] = GraphEdgeBusInfo{ bus, ++span_count};
            }

            if (internal_from + 1 != to_end) {
                AddBusEdgesByStop(internal_from + 1, 
                                  internal_from + 2, 
                                  to_end,
                                  bus,
                                  bus_velocity);
            }            
        }

    private:
        const transport::TransportCatalogue& db_;
        std::unique_ptr<graph::DirectedWeightedGraph<GraphEdgeWeight>> graph_;
        std::unique_ptr<graph::Router<GraphEdgeWeight>> router_;
        std::unordered_map<transport::StopPtr, StopGraphVertexInfo> stop_to_vertex_info_;
        std::unordered_map<graph::EdgeId, GraphEdgeBusInfo> edge_to_bus_info_;
    };

private:
    // RequestHandler использует агрегацию объектов "Транспортный Справочник" и "Визуализатор Карты"
    const transport::TransportCatalogue& db_;
    const renderer::MapRenderer& renderer_;
    RouteRequestHelper<double> route_request_helper_;
};


