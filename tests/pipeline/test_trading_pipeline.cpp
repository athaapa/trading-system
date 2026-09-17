#include "pipeline/trading_pipeline.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <variant>
#include <vector>

namespace {
    template <typename Test> void run_test(const char* name, Test test) {
        std::cerr << "RUN " << name << '\n';
        test();
    }

#define RUN_TEST(test) run_test(#test, test)

    constexpr StockLocate kStockLocate = 0x1234;
    constexpr Timestamp kTimestamp = 123456789012ULL;
    constexpr Price kBidPrice = 1000000;
    constexpr Price kAskPrice = 1000100;

    void write_big_endian(std::span<std::uint8_t> destination, std::uint64_t value) {
        for (std::size_t i = destination.size(); i-- > 0;) {
            destination[i] = static_cast<std::uint8_t>(value & 0xFF);
            value >>= 8;
        }
    }

    std::uint64_t read_big_endian(std::span<const std::uint8_t> bytes) {
        std::uint64_t value = 0;
        for (const std::uint8_t byte : bytes) {
            value = (value << 8) | byte;
        }
        return value;
    }

    std::vector<std::uint8_t> make_add_order_message(OrderReferenceNumber order_num,
        Side side,
        Quantity quantity,
        Price price) {
        std::vector<std::uint8_t> message(36, 0);
        std::span<std::uint8_t> bytes { message };

        message[0] = static_cast<std::uint8_t>('A');
        write_big_endian(bytes.subspan(1, 2), kStockLocate);
        write_big_endian(bytes.subspan(5, 6), kTimestamp);
        write_big_endian(bytes.subspan(11, 8), order_num);
        message[19] = static_cast<std::uint8_t>(side == Side::Buy ? 'B' : 'S');
        write_big_endian(bytes.subspan(20, 4), quantity);
        write_big_endian(bytes.subspan(32, 4), price);

        return message;
    }

    void test_raw_itch_messages_produce_encoded_ouch_order() {
        TradingPipeline pipeline(kStockLocate, "AAPL", 100, 2);

        const auto bid = make_add_order_message(1, Side::Buy, 100, kBidPrice);
        const PipelineResult bid_result = pipeline.process(bid);
        assert(std::holds_alternative<NoOrder>(bid_result));

        const auto ask = make_add_order_message(2, Side::Sell, 10, kAskPrice);
        const PipelineResult ask_result = pipeline.process(ask);
        const auto* encoded = std::get_if<EncodedOrder>(&ask_result);
        assert(encoded != nullptr);

        const std::span<const std::uint8_t> bytes { *encoded };
        assert(bytes[0] == static_cast<std::uint8_t>('O'));
        assert(read_big_endian(bytes.subspan(1, 4)) == 1);
        assert(bytes[5] == static_cast<std::uint8_t>('B'));
        assert(read_big_endian(bytes.subspan(6, 4)) == 10);

        constexpr std::array<std::uint8_t, 8> kExpectedSymbol {
            'A', 'A', 'P', 'L', ' ', ' ', ' ', ' ',
        };
        assert(std::equal(kExpectedSymbol.begin(), kExpectedSymbol.end(), bytes.begin() + 10));
        assert(read_big_endian(bytes.subspan(18, 8)) == kAskPrice);
    }
}

int main() {
    RUN_TEST(test_raw_itch_messages_produce_encoded_ouch_order);
    return 0;
}
