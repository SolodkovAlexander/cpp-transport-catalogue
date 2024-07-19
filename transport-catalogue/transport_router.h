#pragma once

#include "graph.h"
#include "router.h"
#include "transport_catalogue.h"

#include <memory>
#include <unordered_map>
#include <variant>

using RouteTime = double;
using StopPtrIt = std::vector<transport::StopPtr>::const_iterator;

class TransportRouter {
public:
    TransportRouter(const transport::TransportCatalogue& db);

public:
    struct BusRouteInfo {
        transport::BusPtr bus = nullptr;
        int span_count = 0;
        RouteTime route_time = 0.0;
    };

    std::optional<graph::Router<RouteTime>::RouteInfo> GetMinTimeRoute(transport::StopPtr stop_from, transport::StopPtr stop_to) const;
    
    BusRouteInfo getBusRouteInfo(graph::VertexId bus_route_id) const;

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
    void InitGraph(const transport::TransportCatalogue& db);

    void InitGraphVerteces(const std::vector<transport::StopPtr>& stops, RouteTime bus_wait_time);

    void InitGraphEdges(const transport::TransportCatalogue& db, 
                        RouteTime bus_velocity);

    void AddBusEdgesByStop(StopPtrIt internal_from, StopPtrIt to_start, StopPtrIt to_end,
                           transport::BusPtr bus,
                           RouteTime bus_velocity,
                           const transport::TransportCatalogue& db);

private:
    std::unique_ptr<graph::DirectedWeightedGraph<RouteTime>> graph_;
    std::unique_ptr<graph::Router<RouteTime>> router_;
    std::unordered_map<transport::StopPtr, StopGraphVertexInfo> stop_to_vertex_info_;
    std::unordered_map<graph::EdgeId, GraphEdgeBusInfo> edge_to_bus_info_;
};
