#include "transport_router.h"

TransportRouter::TransportRouter(const transport::TransportCatalogue& db) 
{
    InitGraph(db);
    router_ = std::make_unique<graph::Router<RouteTime>>(*graph_);
}

std::optional<graph::Router<RouteTime>::RouteInfo> TransportRouter::GetMinTimeRoute(transport::StopPtr stop_from, 
                                                                   transport::StopPtr stop_to) const {
    return router_->BuildRoute(stop_to_vertex_info_.at(stop_from).waiting_bus_vertex_id, 
                               stop_to_vertex_info_.at(stop_to).waiting_bus_vertex_id);
}

TransportRouter::BusRouteInfo TransportRouter::getBusRouteInfo(graph::VertexId bus_route_id) const {
    auto [bus, span_count] = edge_to_bus_info_.at(bus_route_id);
    return BusRouteInfo{
        bus,
        span_count,
        static_cast<RouteTime>(graph_->GetEdge(bus_route_id).weight)
    };
}

void TransportRouter::InitGraph(const transport::TransportCatalogue& db) {
    const auto& stops = db.GetStops();

    // Количество вершин = остановки и точки ожидания автобуса на остановках
    graph_ = std::make_unique<graph::DirectedWeightedGraph<RouteTime>>(stops.size() * 2);
    
    const auto& routing_settings = db.GetRoutingSettings();
    InitGraphVerteces(db.GetStops(), static_cast<RouteTime>(routing_settings.bus_wait_time));
    InitGraphEdges(db, static_cast<RouteTime>(routing_settings.bus_velocity));
}

void TransportRouter::InitGraphVerteces(const std::vector<transport::StopPtr>& stops, RouteTime bus_wait_time) {
    for (size_t i = 0; i < stops.size(); ++i) {
        // Определяем идентификаторы вершин графа: точки остановок и доп. точки ожидания автобуса на остановках
        stop_to_vertex_info_[stops.at(i)] = StopGraphVertexInfo{ stops.size() + i, i };
        // Добавляем ребро: от точки ожидания автобуса до точки остановки
        graph_->AddEdge(graph::Edge<RouteTime>{ stops.size() + i, i, bus_wait_time });
    }
}

void TransportRouter::InitGraphEdges(const transport::TransportCatalogue& db, 
                                     RouteTime bus_velocity) {
    const auto& buses = db.GetBuses();
    const auto& roundtrip_buses = db.GetRoundtripBuses();
    for (auto bus : buses) {
        if (roundtrip_buses.count(bus)) {
            AddBusEdgesByStop(bus->stops.begin(), 
                              bus->stops.begin() + 1, 
                              bus->stops.end(),
                              bus,
                              bus_velocity,
                              db);
        } else {
            AddBusEdgesByStop(bus->stops.begin(), 
                              bus->stops.begin() + 1, 
                              bus->stops.begin() + (bus->stops.size() + 1) / 2,
                              bus,
                              bus_velocity,
                              db);
            AddBusEdgesByStop(bus->stops.begin() + bus->stops.size() / 2, 
                              bus->stops.begin() + bus->stops.size() / 2 + 1, 
                              bus->stops.end(),
                              bus,
                              bus_velocity,
                              db);
        }
    }
}

void TransportRouter::AddBusEdgesByStop(StopPtrIt internal_from, StopPtrIt to_start, StopPtrIt to_end,
                                        transport::BusPtr bus,
                                        RouteTime bus_velocity,
                                        const transport::TransportCatalogue& db) {
    RouteTime edge_weight{};
    auto from_vertex_id = stop_to_vertex_info_.at(*internal_from).stop_vertex_id;
    int span_count = 0;
    for (auto from = internal_from, to = to_start; to != to_end; ++from, ++to) {
        if (*to == *internal_from) {
            continue;
        }
        edge_weight += static_cast<RouteTime>(db.GetStopDistance(*from, *to)) / bus_velocity;
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
                          bus_velocity,
                          db);
    }            
}