#include "transport_router.h"

TransportRouter::TransportRouter(const transport::TransportCatalogue& db) :
    db_(db)
{
    InitGraph();
    router_ = std::make_unique<graph::Router<RouteTime>>(*graph_);
}

std::optional<RouteInfo> TransportRouter::FindRoute(transport::StopPtr stop_from, 
                                                    transport::StopPtr stop_to) const {
    auto route = router_->BuildRoute(stop_to_vertex_info_.at(stop_from).waiting_bus_vertex_id, 
                                     stop_to_vertex_info_.at(stop_to).waiting_bus_vertex_id);
    if (!route) {
        return std::nullopt;
    }

    RouteInfo route_info{ (*route).weight, {} };
    route_info.items.reserve((*route).edges.size());

    const auto& routing_settings = db_.GetRoutingSettings();
    const auto& stops = db_.GetStops();
    for (auto route_part_it = (*route).edges.begin(); route_part_it != (*route).edges.end(); ++route_part_it) {
        auto route_part_id = *route_part_it;
        if (route_part_id < stops.size()) {
            route_info.items.push_back(RouteInfo::WaitingOnStopItem{
                stops.at(route_part_id),
                static_cast<RouteTime>(routing_settings.bus_wait_time)
            });
        } else {
            auto [bus, span_count] = edge_to_bus_info_.at(route_part_id);
            route_info.items.push_back(RouteInfo::BusItem{
                bus,
                static_cast<RouteTime>(graph_->GetEdge(route_part_id).weight),
                span_count
            });
        }
    }
    return route_info;

}

void TransportRouter::InitGraph() {
    const auto& stops = db_.GetStops();

    // Количество вершин = остановки и точки ожидания автобуса на остановках
    graph_ = std::make_unique<graph::DirectedWeightedGraph<RouteTime>>(stops.size() * 2);
    
    const auto& routing_settings = db_.GetRoutingSettings();
    InitGraphVerteces(db_.GetStops(), static_cast<RouteTime>(routing_settings.bus_wait_time));
    InitGraphEdges(static_cast<RouteTime>(routing_settings.bus_velocity));
}

void TransportRouter::InitGraphVerteces(const std::vector<transport::StopPtr>& stops, RouteTime bus_wait_time) {
    for (size_t i = 0; i < stops.size(); ++i) {
        // Определяем идентификаторы вершин графа: точки остановок и доп. точки ожидания автобуса на остановках
        stop_to_vertex_info_[stops.at(i)] = StopGraphVertexInfo{ stops.size() + i, i };
        // Добавляем ребро: от точки ожидания автобуса до точки остановки
        graph_->AddEdge(graph::Edge<RouteTime>{ stops.size() + i, i, bus_wait_time });
    }
}

void TransportRouter::InitGraphEdges(RouteTime bus_velocity) {
    const auto& buses = db_.GetBuses();
    const auto& roundtrip_buses = db_.GetRoundtripBuses();
    for (auto bus : buses) {
        if (roundtrip_buses.count(bus)) {
            AddBusEdgesByStop(bus->stops.begin(), 
                              bus->stops.begin() + 1, 
                              bus->stops.end(),
                              bus,
                              bus_velocity);
        } else {
            AddBusEdgesByStop(bus->stops.begin(), 
                              bus->stops.begin() + 1, 
                              bus->stops.begin() + (bus->stops.size() + 1) / 2,
                              bus,
                              bus_velocity);
            AddBusEdgesByStop(bus->stops.begin() + bus->stops.size() / 2, 
                              bus->stops.begin() + bus->stops.size() / 2 + 1, 
                              bus->stops.end(),
                              bus,
                              bus_velocity);
        }
    }
}

void TransportRouter::AddBusEdgesByStop(StopPtrIt internal_from, StopPtrIt to_start, StopPtrIt to_end,
                                        transport::BusPtr bus,
                                        RouteTime bus_velocity) {
    RouteTime edge_weight{};
    auto from_vertex_id = stop_to_vertex_info_.at(*internal_from).stop_vertex_id;
    size_t span_count = 0;
    for (auto from = internal_from, to = to_start; to != to_end; ++from, ++to) {
        if (*to == *internal_from) {
            continue;
        }
        edge_weight += static_cast<RouteTime>(db_.GetStopDistance(*from, *to)) / bus_velocity;
        auto edgeId = graph_->AddEdge(graph::Edge<RouteTime>{ 
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
