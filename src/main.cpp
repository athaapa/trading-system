
#include "pipeline/trading_pipeline.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <span>
#include <variant>

namespace {
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

    std::array<std::uint8_t, 36> make_add_order_message(OrderReferenceNumber order_num,
        Side side,
        Quantity quantity,
        Price price) {
        std::array<std::uint8_t, 36> message { };
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

    void print_bytes(const EncodedOrder& order) {
        for (const std::uint8_t byte : order) {
            std::cout << std::hex << std::setw(2) << std::setfill('0')
                      << static_cast<unsigned int>(byte) << ' ';
        }
        std::cout << '\n';
    }
}

int main() {
    TradingPipeline pipeline(kStockLocate, "AAPL", 100, 2);

    const auto bid = make_add_order_message(1, Side::Buy, 100, kBidPrice);
    const PipelineResult bid_result = pipeline.process(bid);
    if (!std::holds_alternative<NoOrder>(bid_result)) {
        std::cerr << "The bid message produced an unexpected result.\n";
        return 1;
    }

    const auto ask = make_add_order_message(2, Side::Sell, 10, kAskPrice);
    const PipelineResult ask_result = pipeline.process(ask);
    const auto* encoded_order = std::get_if<EncodedOrder>(&ask_result);
    if (encoded_order == nullptr) {
        std::cerr << "The ask message did not produce an order.\n";
        return 1;
    }

    std::cout << "Encoded OUCH Enter Order (" << encoded_order->size() << " bytes):\n";
    print_bytes(*encoded_order);
    return 0;
}
