#include "domain.h"

namespace transport {

using StopPtr = const Stop*;

size_t StopPairHasher::operator() (const std::pair<StopPtr, StopPtr>& other) const {
    return stop_hasher(other.first) + stop_hasher(other.second) * n;
}

bool BusComparator::operator() (BusPtr lhs, BusPtr rhs) const {
    return lhs->id < rhs->id;
}

}  // namespace transport