#include "geo.h"
#include "stat_reader.h"

#include <iomanip>
#include <set>
#include <unordered_set>

using namespace std::literals;
using namespace transport;

namespace {
    const int STAT_BUS_CURVATURE_PRECISION = 6;
}

RequestDescription ParseRequest(std::string_view request) {
    auto space_pos = request.find(' ');
    auto request_type = request.substr(0, space_pos);
    if (request_type != "Bus"s && request_type != "Stop"s) {
        return {};
    }

    auto not_space = request.find_first_not_of(' ', space_pos);
    return {request_type, 
            request.substr(not_space, request.size() - not_space)};
}

void ParseAndPrintStatBus(const TransportCatalogue& tansport_catalogue, 
                          const RequestDescription& request_description,
                          std::ostream& output) {
    auto bus = tansport_catalogue.GetBus(request_description.object_id);
    if (!bus) {
        output << "Bus "s << request_description.object_id << ": not found\n"s;
        return;
    }

    const size_t r_stop_count = bus->stops.size();
    const size_t u_stop_count = std::unordered_set(bus->stops.begin(), bus->stops.end()).size();
    double r_length = 0.0;
    int r_length_by_stop_distance = 0;
    for (auto start_stop = bus->stops.cbegin(), end_stop = bus->stops.cbegin() + 1;
         end_stop != bus->stops.cend(); ++start_stop, ++end_stop) {
        r_length += ComputeDistance((*start_stop)->coordinates, (*end_stop)->coordinates);
        r_length_by_stop_distance += tansport_catalogue.GetStopDistance(*start_stop, *end_stop);
    }

    output << "Bus "s << bus->id << ": "s 
           << r_stop_count << " stops on route, "s
           << u_stop_count << " unique stops, "s
           << r_length_by_stop_distance << " route length, "s
           << std::setprecision(STAT_BUS_CURVATURE_PRECISION) << static_cast<double>(r_length_by_stop_distance) / r_length  << " curvature\n";
}

struct BusCmp {
    bool operator() (BusPtr lhs, BusPtr rhs) const {
        return lhs->id < rhs->id;
    }
};

void ParseAndPrintStatBusStop(const TransportCatalogue& tansport_catalogue, 
                              const RequestDescription& request_description,
                              std::ostream& output) {
    auto stop = tansport_catalogue.GetStop(request_description.object_id);
    if (!stop) {
        output << "Stop "s << request_description.object_id << ": not found\n"s;
        return;
    }
    
    auto buses = tansport_catalogue.GetBuses(request_description.object_id);
    if (buses.empty()) {
        output << "Stop "s << request_description.object_id << ": no buses\n"s;
        return;
    }
    
    const std::set<BusPtr, BusCmp> sorted_buses{std::make_move_iterator(buses.begin()), std::make_move_iterator(buses.end())};
    output << "Stop "s << request_description.object_id << ": buses"s;
    for (auto bus : sorted_buses) {
        output << ' ' << bus->id;
    }
    output << '\n';
}

namespace transport {
void ParseAndPrintStat(const TransportCatalogue& tansport_catalogue, 
                       std::string_view request,
                       std::ostream& output) { 
    auto request_description = ParseRequest(request);
    if (request_description.request_type == "Bus"s) {
        ParseAndPrintStatBus(tansport_catalogue, request_description, output);
    } else if (request_description.request_type == "Stop"s) {
        ParseAndPrintStatBusStop(tansport_catalogue, request_description, output);
    }
}
} // namespace transport
