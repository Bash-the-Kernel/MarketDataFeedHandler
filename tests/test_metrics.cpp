#include <gtest/gtest.h>
#include "metrics.h"

using namespace mdfh;

TEST(MetricsTest, LatencyStats) {
    LatencyMetrics metrics;
    
    metrics.record(1000);
    metrics.record(2000);
    metrics.record(3000);
    metrics.record(4000);
    metrics.record(5000);
    
    auto stats = metrics.compute();
    EXPECT_EQ(stats.min_ns, 1000);
    EXPECT_EQ(stats.max_ns, 5000);
    EXPECT_EQ(stats.avg_ns, 3000);
    EXPECT_EQ(stats.count, 5);
}

TEST(MetricsTest, Throughput) {
    ThroughputMetrics metrics;
    metrics.start();
    
    for (int i = 0; i < 100; ++i) {
        metrics.record_message();
    }
    
    EXPECT_EQ(metrics.total_messages(), 100);
    EXPECT_GT(metrics.messages_per_second(), 0.0);
}
