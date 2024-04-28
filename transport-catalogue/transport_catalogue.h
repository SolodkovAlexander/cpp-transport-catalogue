#pragma once
#include "geo.h"

#include <deque>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace transport {
struct BusStop {
    std::string id;
    geo::Coordinates coordinates;
};

struct Bus {
    std::string id;
    std::vector<BusStop*> stops;
};

class TransportCatalogue {
    public:
        void AddBusStop(std::string id, geo::Coordinates coordinates);
        void AddBus(std::string id, std::vector<std::string_view> route);
        BusStop* GetBusStop(std::string_view id) const;
        Bus* GetBus(std::string_view id) const;
        std::set<std::string_view> GetBuses(std::string_view bus_stop_id) const;

    private:
    std::deque<BusStop> bus_stops_;
    std::unordered_map<std::string_view, BusStop*> bus_stop_links_; 
    std::deque<Bus> buses_;
    std::unordered_map<std::string_view, Bus*> bus_links_;
    std::unordered_map<std::string_view, std::set<std::string_view>> bus_stop_to_buses_;
};
}
