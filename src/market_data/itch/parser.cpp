#include "parser.hpp"
#include "book/book_types.hpp"
#include "market_data/book_update.hpp"
#include "market_data/market_event.hpp"

#include <concepts>
#include <cstdint>

namespace {
    template <std::unsigned_integral T> T read_big_endian(std::span<const std::uint8_t> bytes) {
        T value = 0;

        for (const std::uint8_t byte : bytes) {
            value = static_cast<T>((value << 8) | byte);
        }

        return value;
    }

    ParseResult parse_add_order(std::span<const uint8_t> message, size_t len) {
        if (message.size() != len) {
            return ParseError::IncorrectMessageLength;
        }

        StockLocate stock_locate = read_big_endian<std::uint16_t>(message.subspan(1, 2));
        // tracking number
        Timestamp timestamp = read_big_endian<std::uint64_t>(message.subspan(5, 6));

        OrderReferenceNumber order_num = read_big_endian<std::uint64_t>(message.subspan(11, 8));

        Side side;

        switch (static_cast<char>(message[19])) {
        case 'B':
            side = Side::Buy;
            break;
        case 'S':
            side = Side::Sell;
            break;
        default:
            return ParseError::InvalidSide;
        }

        Quantity quantity = read_big_endian<std::uint32_t>(message.subspan(20, 4));
        Price price = read_big_endian<std::uint32_t>(message.subspan(32, 4));

        AddOrder order {
            .order_num = order_num, .side = side, .quantity = quantity, .price = price
        };
        MarketEvent event { .stock_locate = stock_locate, .timestamp = timestamp, .update = order };

        return event;
    }

    ParseResult parse_execute_order(std::span<const uint8_t> message, size_t len) {
        if (message.size() != len) {
            return ParseError::IncorrectMessageLength;
        }

        StockLocate stock_locate = read_big_endian<std::uint16_t>(message.subspan(1, 2));
        // tracking number
        Timestamp timestamp = read_big_endian<std::uint64_t>(message.subspan(5, 6));

        OrderReferenceNumber order_num = read_big_endian<std::uint64_t>(message.subspan(11, 8));
        Quantity quantity = read_big_endian<std::uint64_t>(message.subspan(19, 4));
        // match number

        ExecuteOrder order { .order_num = order_num, .quantity = quantity };
        MarketEvent event { .stock_locate = stock_locate, .timestamp = timestamp, .update = order };

        return event;
    }

}

ParseResult parse_message(std::span<const uint8_t> message) {
    if (message.empty()) {
        return ParseError::EmptyMessage;
    }

    switch (static_cast<char>(message[0])) {
    case 'A': {
        // parse add order (no mpid)
        return parse_add_order(message, 36);
    }
    case 'F': {
        // parse add order (mpid)
        return parse_add_order(message, 40);
    }
    case 'E': {
        // execute order (no price, no printable)
        return parse_execute_order(message, 31);
    }
    case 'C': {
        // execute order (price, printable)
        return parse_execute_order(message, 36);
    }
    case 'X': {
        if (message.size() != 23) {
            return ParseError::IncorrectMessageLength;
        }

        StockLocate stock_locate = read_big_endian<std::uint16_t>(message.subspan(1, 2));
        // tracking number
        Timestamp timestamp = read_big_endian<std::uint64_t>(message.subspan(5, 6));

        OrderReferenceNumber order_num = read_big_endian<std::uint64_t>(message.subspan(11, 8));
        Quantity quantity = read_big_endian<std::uint64_t>(message.subspan(19, 4));

        CancelOrder order { .order_num = order_num, .quantity = quantity };
        MarketEvent event { .stock_locate = stock_locate, .timestamp = timestamp, .update = order };

        return event;
    }
    case 'D': {
        if (message.size() != 19) {
            return ParseError::IncorrectMessageLength;
        }

        StockLocate stock_locate = read_big_endian<std::uint16_t>(message.subspan(1, 2));
        // tracking number
        Timestamp timestamp = read_big_endian<std::uint64_t>(message.subspan(5, 6));

        OrderReferenceNumber order_num = read_big_endian<std::uint64_t>(message.subspan(11, 8));

        DeleteOrder order { .order_num = order_num };
        MarketEvent event { .stock_locate = stock_locate, .timestamp = timestamp, .update = order };

        return event;
    }
    case 'U': {
        if (message.size() != 35) {
            return ParseError::IncorrectMessageLength;
        }

        StockLocate stock_locate = read_big_endian<std::uint16_t>(message.subspan(1, 2));
        // tracking number
        Timestamp timestamp = read_big_endian<std::uint64_t>(message.subspan(5, 6));

        OrderReferenceNumber old_order_num = read_big_endian<std::uint64_t>(message.subspan(11, 8));
        OrderReferenceNumber new_order_num = read_big_endian<std::uint64_t>(message.subspan(19, 8));
        Quantity quantity = read_big_endian<std::uint32_t>(message.subspan(27, 4));
        Price price = read_big_endian<std::uint32_t>(message.subspan(31, 4));

        ReplaceOrder order {
            .old_order_num = old_order_num,
            .new_order_num = new_order_num,
            .quantity = quantity,
            .price = price,
        };
        MarketEvent event { .stock_locate = stock_locate, .timestamp = timestamp, .update = order };

        return event;
    }
    default:
        return ParseError::UnsupportedMessageType;
    }
}
