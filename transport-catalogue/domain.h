#pragma once

#include "geo.h"

#include <string>
#include <vector>

/*
 * Классы/структуры, которые являются частью предметной области (domain)
 * вашего приложения и не зависят от транспортного справочника. Например Автобусные маршруты и Остановки. 
 */

namespace transport {

// ---------- Stop ------------------

struct Stop {
    std::string id;
    geo::Coordinates coordinates;
};
using StopPtr = const Stop*;

struct StopPairHasher {
    size_t operator() (const std::pair<StopPtr, StopPtr>& other) const;    
    private:
        inline static std::hash<StopPtr> stop_hasher;
        inline static const size_t n = 37;
};

// ---------- Bus ------------------

struct Bus {
    std::string id;
    std::vector<StopPtr> stops;
};

struct RoutingSettings {
    int bus_wait_time = 0;
    double bus_velocity = 0.0;
};

}  // namespace transport
