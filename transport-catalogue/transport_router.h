#pragma once

#include "graph.h"
#include "router.h"
#include "transport_catalogue.h"

#include <memory>
#include <unordered_map>
#include <variant>

using RouteTime = double;
using StopPtrIt = std::vector<transport::StopPtr>::const_iterator;

struct RouteInfo {
    struct WaitingOnStopItem {
        transport::StopPtr stop;
        RouteTime time = 0.0;
    };
    struct BusItem {
        transport::BusPtr bus;
        RouteTime time = 0.0;
        size_t span_count = 0;
    };

    RouteTime total_time = 0.0;

    using RouteStatItem = std::variant<RouteInfo::WaitingOnStopItem, RouteInfo::BusItem>;
    std::vector<RouteStatItem> items;
};

class TransportRouter {
public:
    TransportRouter(const transport::TransportCatalogue& db);

public:
    struct BusRouteInfo {
        transport::BusPtr bus = nullptr;
        RouteTime time = 0.0;
        size_t span_count = 0;
    };

    std::optional<RouteInfo> FindRoute(transport::StopPtr stop_from, transport::StopPtr stop_to) const;

private:
    struct StopGraphVertexInfo {
        graph::VertexId waiting_bus_vertex_id;
        graph::VertexId stop_vertex_id;
    };
    struct GraphEdgeBusInfo {
        transport::BusPtr bus;
        size_t span_count;
    };

private:
    void InitGraph();

    void InitGraphVerteces(const std::vector<transport::StopPtr>& stops, RouteTime bus_wait_time);

    void InitGraphEdges(RouteTime bus_velocity);

    void AddBusEdgesByStop(StopPtrIt internal_from, StopPtrIt to_start, StopPtrIt to_end,
                           transport::BusPtr bus,
                           RouteTime bus_velocity);

private:
    // TransportRouter использует агрегацию объектов "Транспортный Справочник" и "Граф" и "Роутер"
    const transport::TransportCatalogue& db_;
    std::unique_ptr<graph::DirectedWeightedGraph<RouteTime>> graph_;
    std::unique_ptr<graph::Router<RouteTime>> router_;
    std::unordered_map<transport::StopPtr, StopGraphVertexInfo> stop_to_vertex_info_;
    std::unordered_map<graph::EdgeId, GraphEdgeBusInfo> edge_to_bus_info_;
};
