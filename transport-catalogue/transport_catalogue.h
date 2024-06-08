#pragma once
#include "domain.h"
#include "geo.h"

#include <deque>
#include <string>
#include <unordered_map>
#include <vector>
#include <unordered_set>

namespace transport {

using StopPtr = const Stop*;
using BusPtr = const Bus*;

class TransportCatalogue {
    public:
        void AddStop(std::string id, geo::Coordinates coordinates);
        StopPtr GetStop(std::string_view id) const;
        void SetStopDistance(StopPtr stop_from, StopPtr stop_to, int distance);
        int GetStopDistance(StopPtr stop_from, StopPtr stop_to) const;

        void AddBus(std::string id, std::vector<StopPtr> route_stops);
        BusPtr GetBus(std::string_view id) const;
        std::unordered_set<BusPtr> GetBuses(std::string_view stop_id) const;

    private:
    std::deque<Stop> stops_;
    std::unordered_map<std::string_view, StopPtr> stop_links_; 
    std::deque<Bus> buses_;
    std::unordered_map<std::string_view, BusPtr> bus_links_;
    std::unordered_map<StopPtr, std::unordered_set<BusPtr>> stop_to_buses_;
    std::unordered_map<std::pair<StopPtr, StopPtr>, int, StopPairHasher> distances_;
};

}  // namespace transport
