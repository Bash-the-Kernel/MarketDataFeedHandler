#include <gtest/gtest.h>
#include "message_format.h"

using namespace mdfh;

TEST(MessageFormatTest, TradeSerializeDeserialize) {
    uint8_t buffer[64];
    size_t size = serialize_trade(buffer, 100, 1, 123456789, 150.5, 1000, 999);
    
    EXPECT_EQ(size, sizeof(TradeMessage));
    
    Message msg;
    EXPECT_TRUE(deserialize_message(buffer, size, msg));
    EXPECT_EQ(msg.header.type, MessageType::Trade);
    EXPECT_EQ(msg.header.symbol_id, 100);
    EXPECT_EQ(msg.header.sequence, 1);
    EXPECT_EQ(msg.trade.price, 150.5);
    EXPECT_EQ(msg.trade.quantity, 1000);
}

TEST(MessageFormatTest, QuoteSerializeDeserialize) {
    uint8_t buffer[64];
    size_t size = serialize_quote(buffer, 50, 2, 987654321, 100.0, 100.5, 500, 600);
    
    EXPECT_EQ(size, sizeof(QuoteMessage));
    
    Message msg;
    EXPECT_TRUE(deserialize_message(buffer, size, msg));
    EXPECT_EQ(msg.header.type, MessageType::Quote);
    EXPECT_EQ(msg.header.symbol_id, 50);
    EXPECT_EQ(msg.quote.bid_price, 100.0);
    EXPECT_EQ(msg.quote.ask_price, 100.5);
}
