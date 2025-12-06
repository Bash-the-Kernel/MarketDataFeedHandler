#pragma once

#include <vector>
#include <algorithm>
#include <cstdint>
#include <chrono>

namespace mdfh {

class LatencyMetrics {
public:
    void record(uint64_t latency_ns) {
        latencies_.push_back(latency_ns);
    }
    
    struct Stats {
        uint64_t min_ns;
        uint64_t avg_ns;
        uint64_t p99_ns;
        uint64_t max_ns;
        size_t count;
    };
    
    Stats compute() const;
    void reset();
    
private:
    std::vector<uint64_t> latencies_;
};

class ThroughputMetrics {
public:
    void start() {
        start_time_ = std::chrono::steady_clock::now();
        message_count_ = 0;
    }
    
    void record_message() {
        ++message_count_;
    }
    
    double messages_per_second() const {
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - start_time_).count();
        if (duration == 0) return 0.0;
        return (message_count_ * 1'000'000.0) / duration;
    }
    
    uint64_t total_messages() const { return message_count_; }
    
private:
    std::chrono::steady_clock::time_point start_time_;
    uint64_t message_count_ = 0;
};

inline uint64_t get_timestamp_ns() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
}

} // namespace mdfh
