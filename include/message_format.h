#pragma once

#include <cstdint>
#include <cstring>
#include <array>

namespace mdfh {

enum class MessageType : uint8_t {
    Trade = 1,
    Quote = 2
};

#pragma pack(push, 1)
struct MessageHeader {
    MessageType type;
    uint8_t reserved;
    uint16_t symbol_id;
    uint32_t sequence;
    uint64_t exchange_timestamp;
};

struct TradeMessage {
    MessageHeader header;
    double price;
    uint64_t quantity;
    uint64_t trade_id;
};

struct QuoteMessage {
    MessageHeader header;
    double bid_price;
    double ask_price;
    uint64_t bid_size;
    uint64_t ask_size;
};
#pragma pack(pop)

union Message {
    MessageHeader header;
    TradeMessage trade;
    QuoteMessage quote;
    std::array<uint8_t, 64> raw;
    
    Message() { std::memset(raw.data(), 0, raw.size()); }
};

size_t serialize_trade(uint8_t* buffer, uint16_t symbol_id, uint32_t sequence,
                       uint64_t timestamp, double price, uint64_t quantity, uint64_t trade_id);

size_t serialize_quote(uint8_t* buffer, uint16_t symbol_id, uint32_t sequence,
                       uint64_t timestamp, double bid_price, double ask_price,
                       uint64_t bid_size, uint64_t ask_size);

bool deserialize_message(const uint8_t* buffer, size_t length, Message& msg);

} // namespace mdfh
