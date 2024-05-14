#include "transport_catalogue.h"

#include <cassert>

namespace transport {
void TransportCatalogue::AddStop(std::string id, geo::Coordinates coordinates) {
    Stop* stop = &stops_.emplace_back(Stop{std::move(id), std::move(coordinates)});
    stop_links_[stop->id] = stop;
}

StopPtr TransportCatalogue::GetStop(std::string_view id) const {
    return stop_links_.count(id) ? stop_links_.at(id) : nullptr;
}

void TransportCatalogue::SetStopDistance(std::string_view id_from, std::string_view id_to, int distance) {
    auto stop_from = const_cast<Stop*>(GetStop(id_from));
    auto stop_to = GetStop(id_to);
    assert(stop_from && stop_to);

    stop_from->distance_to_stops[stop_to] = distance;
}

int TransportCatalogue::GetStopDistance(StopPtr stop_from, StopPtr stop_to) {
    assert(stop_from && stop_to);

    return (stop_from->distance_to_stops.count(stop_to)
            ? stop_from->distance_to_stops.at(stop_to)
            : stop_to->distance_to_stops.at(stop_from));
}

void TransportCatalogue::AddBus(std::string id, std::vector<StopPtr> route_stops) {
    Bus* bus = &buses_.emplace_back(Bus{std::move(id), std::move(route_stops)});
    bus_links_[bus->id] = bus;
    
    for (auto stop : bus->stops) {
        stop_to_buses_[stop].insert(bus);
    }
}

BusPtr TransportCatalogue::GetBus(std::string_view id) const {
    return bus_links_.count(id) ? bus_links_.at(id) : nullptr;
}

std::unordered_set<BusPtr> TransportCatalogue::GetBuses(std::string_view stop_id) const {
    auto stop = GetStop(stop_id);
    return stop_to_buses_.count(stop) ? stop_to_buses_.at(stop) : std::unordered_set<BusPtr>{};
}
} // namespace transport
