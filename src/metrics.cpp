#include "metrics.h"
#include <numeric>

namespace mdfh {

LatencyMetrics::Stats LatencyMetrics::compute() const {
    if (latencies_.empty()) {
        return {0, 0, 0, 0, 0};
    }
    
    auto sorted = latencies_;
    std::sort(sorted.begin(), sorted.end());
    
    uint64_t sum = std::accumulate(sorted.begin(), sorted.end(), 0ULL);
    size_t p99_idx = static_cast<size_t>(sorted.size() * 0.99);
    
    return {
        sorted.front(),
        sum / sorted.size(),
        sorted[p99_idx],
        sorted.back(),
        sorted.size()
    };
}

void LatencyMetrics::reset() {
    latencies_.clear();
}

} // namespace mdfh
