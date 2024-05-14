#pragma once

#include "transport_catalogue.h"

#include <iosfwd>
#include <string_view>

struct RequestDescription {
    std::string_view request_type;
    std::string_view object_id;
};

namespace transport {
void ParseAndPrintStat(const transport::TransportCatalogue& tansport_catalogue, 
                       std::string_view request,
                       std::ostream& output); 
} // namespace transport
