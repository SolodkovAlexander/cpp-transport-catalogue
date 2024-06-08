#include "domain.h"

namespace transport {

using StopPtr = const Stop*;

size_t StopPairHasher::operator() (const std::pair<StopPtr, StopPtr>& other) const {
    return stop_hasher(other.first) + stop_hasher(other.second) * n;
}

}  // namespace transport