#include "geo.h"
#include "stat_reader.h"

#include <iomanip>
#include <unordered_set>

using namespace std::literals;

namespace transport::requests {
void ParseAndPrintStat(const TransportCatalogue& tansport_catalogue, 
                       std::string_view request,
                       std::ostream& output) { 
    auto request_description = detail::ParseRequest(request);
    if (request_description.request_type == "Bus"s) {
        detail::ParseAndPrintStatBus(tansport_catalogue, request_description, output);
    } else if (request_description.request_type == "Stop"s) {
        detail::ParseAndPrintStatBusStop(tansport_catalogue, request_description, output);
    }
}

namespace detail {
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

    size_t r_stop_count = bus->stops.size();
    size_t u_stop_count = std::unordered_set(bus->stops.begin(), bus->stops.end()).size();
    double r_length = 0.0;
    for (auto start_stop = bus->stops.cbegin(), end_stop = bus->stops.cbegin() + 1;
         end_stop != bus->stops.cend(); ++start_stop, ++end_stop) {
        r_length += ComputeDistance((*start_stop)->coordinates, (*end_stop)->coordinates);
    }

    output << "Bus "s << bus->id << ": "s 
           << r_stop_count << " stops on route, "s
           << u_stop_count << " unique stops, "s
           << std::setprecision(6) << r_length << " route length\n"s;
}

void ParseAndPrintStatBusStop(const TransportCatalogue& tansport_catalogue, 
                              const RequestDescription& request_description,
                              std::ostream& output) {
    auto bus_stop = tansport_catalogue.GetBusStop(request_description.object_id);
    if (!bus_stop) {
        output << "Stop "s << request_description.object_id << ": not found\n"s;
        return;
    }
    
    auto bus_ids = tansport_catalogue.GetBuses(request_description.object_id);
    if (bus_ids.empty()) {
        output << "Stop "s << request_description.object_id << ": no buses\n"s;
        return;
    }

    output << "Stop "s << request_description.object_id << ": buses"s;
    for (auto bus_id : bus_ids) {
        output << ' ' << bus_id;
    }
    output << '\n';
}
}
}