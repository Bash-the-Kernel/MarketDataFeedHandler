#include "message_format.h"

namespace mdfh {

size_t serialize_trade(uint8_t* buffer, uint16_t symbol_id, uint32_t sequence,
                       uint64_t timestamp, double price, uint64_t quantity, uint64_t trade_id) {
    TradeMessage msg;
    msg.header.type = MessageType::Trade;
    msg.header.reserved = 0;
    msg.header.symbol_id = symbol_id;
    msg.header.sequence = sequence;
    msg.header.exchange_timestamp = timestamp;
    msg.price = price;
    msg.quantity = quantity;
    msg.trade_id = trade_id;
    
    std::memcpy(buffer, &msg, sizeof(TradeMessage));
    return sizeof(TradeMessage);
}

size_t serialize_quote(uint8_t* buffer, uint16_t symbol_id, uint32_t sequence,
                       uint64_t timestamp, double bid_price, double ask_price,
                       uint64_t bid_size, uint64_t ask_size) {
    QuoteMessage msg;
    msg.header.type = MessageType::Quote;
    msg.header.reserved = 0;
    msg.header.symbol_id = symbol_id;
    msg.header.sequence = sequence;
    msg.header.exchange_timestamp = timestamp;
    msg.bid_price = bid_price;
    msg.ask_price = ask_price;
    msg.bid_size = bid_size;
    msg.ask_size = ask_size;
    
    std::memcpy(buffer, &msg, sizeof(QuoteMessage));
    return sizeof(QuoteMessage);
}

bool deserialize_message(const uint8_t* buffer, size_t length, Message& msg) {
    if (length < sizeof(MessageHeader)) {
        return false;
    }
    
    std::memcpy(&msg.header, buffer, sizeof(MessageHeader));
    
    if (msg.header.type == MessageType::Trade) {
        if (length < sizeof(TradeMessage)) return false;
        std::memcpy(&msg.trade, buffer, sizeof(TradeMessage));
        return true;
    } else if (msg.header.type == MessageType::Quote) {
        if (length < sizeof(QuoteMessage)) return false;
        std::memcpy(&msg.quote, buffer, sizeof(QuoteMessage));
        return true;
    }
    
    return false;
}

} // namespace mdfh
