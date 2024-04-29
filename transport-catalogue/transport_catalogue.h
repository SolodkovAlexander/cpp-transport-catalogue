#pragma once
#include "geo.h"

#include <deque>
#include <string>
#include <unordered_map>
#include <vector>
#include <unordered_set>

namespace transport {
struct Stop {
    std::string id;
    geo::Coordinates coordinates;
};
using StopPtr = const Stop*;

struct Bus {
    std::string id;
    std::vector<StopPtr> stops;
};
using BusPtr = const Bus*;

class TransportCatalogue {
    public:
        void AddStop(std::string id, geo::Coordinates coordinates);
        void AddBus(std::string id, std::vector<StopPtr> route_stops);
        StopPtr GetStop(std::string_view id) const;
        BusPtr GetBus(std::string_view id) const;
        std::unordered_set<BusPtr> GetBuses(std::string_view stop_id) const;

    private:
    std::deque<Stop> stops_;
    std::unordered_map<std::string_view, StopPtr> stop_links_; 
    std::deque<Bus> buses_;
    std::unordered_map<std::string_view, BusPtr> bus_links_;
    std::unordered_map<StopPtr, std::unordered_set<BusPtr>> stop_to_buses_;
};
}
