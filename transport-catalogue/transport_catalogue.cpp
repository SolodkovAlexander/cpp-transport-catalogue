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

void TransportCatalogue::SetStopDistance(StopPtr stop_from, StopPtr stop_to, int distance) {    
    distances_[{stop_from, stop_to}] = distance;
}

int TransportCatalogue::GetStopDistance(StopPtr stop_from, StopPtr stop_to) const {
    assert(stop_from && stop_to);

    return (distances_.count({stop_from, stop_to}) 
            ? distances_.at({stop_from, stop_to})
            : distances_.at({stop_to, stop_from}));
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

std::vector<BusPtr> TransportCatalogue::GetBuses() const { 
    std::vector<BusPtr> buses;
    buses.reserve(buses_.size());
    for (const auto& bus : buses_) {
        buses.push_back(&bus);
    }
    return buses; 
}

}  // namespace transport
