#include "request_handler.h"

using namespace std::literals;

RequestHandler::RequestHandler(const transport::TransportCatalogue& db, 
                               const renderer::MapRenderer& renderer)
    : db_(db),
      renderer_(renderer),
      db_router_(TransportRouter(db))
{}

std::optional<BusStat> RequestHandler::GetBusStat(std::string_view bus_name) const {
    auto bus(db_.GetBus(bus_name));
    if (!bus) {
        return std::nullopt;
    }

    const int r_stop_count = static_cast<int>(bus->stops.size());
    const int u_stop_count = static_cast<int>(std::unordered_set(bus->stops.begin(), bus->stops.end()).size());
    double r_length = 0.0;
    int r_length_by_stop_distance = 0;
    for (auto start_stop = bus->stops.cbegin(), end_stop = bus->stops.cbegin() + 1;
         end_stop != bus->stops.cend(); ++start_stop, ++end_stop) {
        r_length += ComputeDistance((*start_stop)->coordinates, (*end_stop)->coordinates);
        r_length_by_stop_distance += db_.GetStopDistance(*start_stop, *end_stop);
    }

    return BusStat{
        static_cast<double>(r_length_by_stop_distance) / r_length,
        r_length_by_stop_distance,
        r_stop_count,
        u_stop_count
    };
}

std::optional<StopStat> RequestHandler::GetStopStat(std::string_view stop_name) const {
    auto stop = db_.GetStop(stop_name);
    if (!stop) {
        return std::nullopt;
    }
    
    StopStat stop_stat;
    for (auto bus : db_.GetBuses(stop_name)) {
        stop_stat.bus_names.insert(bus->id);
    }
    return stop_stat;
}

std::optional<RouteStat> RequestHandler::GetRouteStat(std::string_view stop_name_from, std::string_view stop_name_to) const {
    auto route = db_router_.GetMinTimeRoute(db_.GetStop(stop_name_from), 
                                            db_.GetStop(stop_name_to));
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
    for (auto route_part_it = (*route).edges.begin(); route_part_it != (*route).edges.end(); ++route_part_it) {
        auto route_part_id = *route_part_it;
        if (route_part_id < stops.size()) {// Ожидаем автобуса
            route_stat.items.push_back(RouteStat::WaitingOnStopItem{stops.at(route_part_id)->id});
        } else {
            auto [bus, span_count, route_part_time] = db_router_.getBusRouteInfo(route_part_id);
            route_stat.items.push_back(RouteStat::BusItem{
                bus->id,
                span_count,
                route_part_time
            });
        }
    }
    return route_stat;
}

svg::Document RequestHandler::RenderMap() const { 
    return renderer_.RenderMap(db_.GetBuses(),
                               db_.GetRoundtripBuses());
}
