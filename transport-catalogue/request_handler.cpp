#include "request_handler.h"

RequestHandler::RequestHandler(const transport::TransportCatalogue& db, 
                               const renderer::MapRenderer& renderer)
    : db_(db),
      renderer_(renderer)
{}

std::optional<BusStat> RequestHandler::GetBusStat(const std::string_view& bus_name) const {
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

std::optional<StopStat> RequestHandler::GetStopStat(const std::string_view& stop_name) const {
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
