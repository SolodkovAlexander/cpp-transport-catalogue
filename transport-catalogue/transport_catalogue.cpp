#include "transport_catalogue.h"

namespace transport {
void TransportCatalogue::AddBusStop(std::string id, geo::Coordinates coordinates) {
    BusStop* bus_stop = &bus_stops_.emplace_back(BusStop{std::move(id), std::move(coordinates)});
    bus_stop_links_[bus_stop->id] = bus_stop;
}

void TransportCatalogue::AddBus(std::string id, std::vector<std::string_view> route) {
    Bus* bus = &buses_.emplace_back(Bus{std::move(id), {}});
    bus_links_[bus->id] = bus;
    
    for (auto stop_id : route) {
        auto bus_stop = GetBusStop(stop_id);
        bus->stops.push_back(bus_stop);
        bus_stop_to_buses_[bus_stop->id].insert(bus->id);
    }
}

BusStop* TransportCatalogue::GetBusStop(std::string_view id) const {
    if (!bus_stop_links_.count(id)) {
        return nullptr;
    }
    return bus_stop_links_.at(id);
}

Bus* TransportCatalogue::GetBus(std::string_view id) const {
    if (!bus_links_.count(id)) {
        return nullptr;
    }
    return bus_links_.at(id);
}

std::set<std::string_view> TransportCatalogue::GetBuses(std::string_view bus_stop_id) const {
    if (!bus_stop_to_buses_.count(bus_stop_id)) {
        return {};
    }
    return bus_stop_to_buses_.at(bus_stop_id);
}
}
