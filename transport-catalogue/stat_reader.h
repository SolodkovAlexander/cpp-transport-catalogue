#pragma once

#include "transport_catalogue.h"

#include <iosfwd>
#include <string_view>

namespace transport::requests {
void ParseAndPrintStat(const TransportCatalogue& tansport_catalogue, 
                       std::string_view request,
                       std::ostream& output); 

namespace detail {
struct RequestDescription {
    std::string_view request_type;
    std::string_view object_id;
};

RequestDescription ParseRequest(std::string_view request);

void ParseAndPrintStatBus(const TransportCatalogue& tansport_catalogue, 
                          const RequestDescription& request_description,
                          std::ostream& output);

void ParseAndPrintStatBusStop(const TransportCatalogue& tansport_catalogue, 
                              const RequestDescription& request_description,
                              std::ostream& output);
}
}
