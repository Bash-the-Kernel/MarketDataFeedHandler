#include <gtest/gtest.h>
#include "ring_buffer.h"

using namespace mdfh;

TEST(RingBufferTest, PushPop) {
    RingBuffer<int, 8> buffer;
    
    EXPECT_TRUE(buffer.empty());
    EXPECT_TRUE(buffer.try_push(42));
    EXPECT_FALSE(buffer.empty());
    
    auto val = buffer.try_pop();
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, 42);
    EXPECT_TRUE(buffer.empty());
}

TEST(RingBufferTest, FullBuffer) {
    RingBuffer<int, 4> buffer;
    
    EXPECT_TRUE(buffer.try_push(1));
    EXPECT_TRUE(buffer.try_push(2));
    EXPECT_TRUE(buffer.try_push(3));
    EXPECT_FALSE(buffer.try_push(4));
    
    EXPECT_EQ(*buffer.try_pop(), 1);
    EXPECT_TRUE(buffer.try_push(4));
}

TEST(RingBufferTest, EmptyPop) {
    RingBuffer<int, 4> buffer;
    EXPECT_FALSE(buffer.try_pop().has_value());
}
