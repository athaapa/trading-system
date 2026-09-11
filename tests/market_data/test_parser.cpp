#include "market_data/itch/parser.hpp"

#include <array>
#include <cassert>
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

    struct MessageType {
        const char* name;
        char type_code;
        std::size_t length;
    };

    constexpr std::array<MessageType, 2> kAddOrderTypes {
        MessageType { "A", 'A', 36 },
        MessageType { "F", 'F', 40 },
    };

    constexpr std::array<MessageType, 2> kExecuteMessageTypes {
        MessageType { "E", 'E', 31 },
        MessageType { "C", 'C', 36 },
    };

    constexpr MessageType kCancelMessage { "X", 'X', 23 };
    constexpr MessageType kDeleteMessage { "D", 'D', 19 };
    constexpr MessageType kReplaceMessage { "U", 'U', 35 };

    template <typename Test>
    void run_typed_test(const char* name, Test test, const MessageType& message_type) {
        std::cerr << "RUN " << name << " [" << message_type.name << "]\n";
        test(message_type);
    }

#define RUN_TEST(test) run_test(#test, test)
#define RUN_TYPED_TEST(test, message_type) run_typed_test(#test, test, message_type)

    constexpr StockLocate kStockLocate = 0x1A2B;
    constexpr Timestamp kTimestamp = 123456789012ULL;
    constexpr OrderReferenceNumber kOrderNum = 9876543210ULL;
    constexpr OrderReferenceNumber kNewOrderNum = 111222333ULL;
    constexpr Quantity kQuantity = 0x01020304;
    constexpr Price kPrice = 1502500;

    void write_be(std::span<std::uint8_t> dest, std::uint64_t value) {
        for (std::size_t i = dest.size(); i-- > 0;) {
            dest[i] = static_cast<std::uint8_t>(value & 0xFF);
            value >>= 8;
        }
    }

    std::vector<std::uint8_t> make_add_order_message(const MessageType& add_type, char side) {
        std::vector<std::uint8_t> message(add_type.length, 0);

        message[0] = static_cast<std::uint8_t>(add_type.type_code);
        write_be(std::span<std::uint8_t>(message).subspan(1, 2), kStockLocate);
        write_be(std::span<std::uint8_t>(message).subspan(5, 6), kTimestamp);
        write_be(std::span<std::uint8_t>(message).subspan(11, 8), kOrderNum);
        message[19] = static_cast<std::uint8_t>(side);
        write_be(std::span<std::uint8_t>(message).subspan(20, 4), kQuantity);
        write_be(std::span<std::uint8_t>(message).subspan(32, 4), kPrice);

        return message;
    }

    std::vector<std::uint8_t> make_execute_message(const MessageType& message_type) {
        std::vector<std::uint8_t> message(message_type.length, 0);

        message[0] = static_cast<std::uint8_t>(message_type.type_code);
        write_be(std::span<std::uint8_t>(message).subspan(1, 2), kStockLocate);
        write_be(std::span<std::uint8_t>(message).subspan(5, 6), kTimestamp);
        write_be(std::span<std::uint8_t>(message).subspan(11, 8), kOrderNum);
        write_be(std::span<std::uint8_t>(message).subspan(19, 4), kQuantity);

        return message;
    }

    std::vector<std::uint8_t> make_cancel_message() {
        return make_execute_message(kCancelMessage);
    }

    std::vector<std::uint8_t> make_delete_message() {
        std::vector<std::uint8_t> message(kDeleteMessage.length, 0);

        message[0] = static_cast<std::uint8_t>(kDeleteMessage.type_code);
        write_be(std::span<std::uint8_t>(message).subspan(1, 2), kStockLocate);
        write_be(std::span<std::uint8_t>(message).subspan(5, 6), kTimestamp);
        write_be(std::span<std::uint8_t>(message).subspan(11, 8), kOrderNum);

        return message;
    }

    std::vector<std::uint8_t> make_replace_message() {
        std::vector<std::uint8_t> message(kReplaceMessage.length, 0);

        message[0] = static_cast<std::uint8_t>(kReplaceMessage.type_code);
        write_be(std::span<std::uint8_t>(message).subspan(1, 2), kStockLocate);
        write_be(std::span<std::uint8_t>(message).subspan(5, 6), kTimestamp);
        write_be(std::span<std::uint8_t>(message).subspan(11, 8), kOrderNum);
        write_be(std::span<std::uint8_t>(message).subspan(19, 8), kNewOrderNum);
        write_be(std::span<std::uint8_t>(message).subspan(27, 4), kQuantity);
        write_be(std::span<std::uint8_t>(message).subspan(31, 4), kPrice);

        return message;
    }

    const MarketEvent& assert_event(const ParseResult& result) {
        assert(std::holds_alternative<MarketEvent>(result));
        return std::get<MarketEvent>(result);
    }

    void assert_add_order(const MarketEvent& event, Side expected_side) {
        assert(event.stock_locate == kStockLocate);
        assert(event.timestamp == kTimestamp);

        const auto* add = std::get_if<AddOrder>(&event.update);
        assert(add != nullptr);
        assert(add->order_num == kOrderNum);
        assert(add->side == expected_side);
        assert(add->quantity == kQuantity);
        assert(add->price == kPrice);
    }

    void assert_execute_order(const MarketEvent& event) {
        assert(event.stock_locate == kStockLocate);
        assert(event.timestamp == kTimestamp);

        const auto* execute = std::get_if<ExecuteOrder>(&event.update);
        assert(execute != nullptr);
        assert(execute->order_num == kOrderNum);
        assert(execute->quantity == kQuantity);
    }

    void assert_cancel_order(const MarketEvent& event) {
        assert(event.stock_locate == kStockLocate);
        assert(event.timestamp == kTimestamp);

        const auto* cancel = std::get_if<CancelOrder>(&event.update);
        assert(cancel != nullptr);
        assert(cancel->order_num == kOrderNum);
        assert(cancel->quantity == kQuantity);
    }

    void assert_delete_order(const MarketEvent& event) {
        assert(event.stock_locate == kStockLocate);
        assert(event.timestamp == kTimestamp);

        const auto* deleted = std::get_if<DeleteOrder>(&event.update);
        assert(deleted != nullptr);
        assert(deleted->order_num == kOrderNum);
    }

    void assert_replace_order(const MarketEvent& event) {
        assert(event.stock_locate == kStockLocate);
        assert(event.timestamp == kTimestamp);

        const auto* replace = std::get_if<ReplaceOrder>(&event.update);
        assert(replace != nullptr);
        assert(replace->old_order_num == kOrderNum);
        assert(replace->new_order_num == kNewOrderNum);
        assert(replace->quantity == kQuantity);
        assert(replace->price == kPrice);
    }

    void assert_error(const ParseResult& result, ParseError expected) {
        assert(std::holds_alternative<ParseError>(result));
        assert(std::get<ParseError>(result) == expected);
    }

    void assert_rejects_incorrect_lengths(const std::vector<std::uint8_t>& message) {
        assert_error(parse_message(std::span<const std::uint8_t>(message).first(message.size() - 1)),
            ParseError::IncorrectMessageLength);

        auto oversized = message;
        oversized.push_back(0xFF);
        assert_error(parse_message(oversized), ParseError::IncorrectMessageLength);
    }

    void test_valid_add_message_buy_returns_event(const MessageType& add_type) {
        const auto message = make_add_order_message(add_type, 'B');

        const ParseResult result = parse_message(message);

        assert_add_order(assert_event(result), Side::Buy);
    }

    void test_valid_add_message_sell_returns_event(const MessageType& add_type) {
        const auto message = make_add_order_message(add_type, 'S');

        const ParseResult result = parse_message(message);

        assert_add_order(assert_event(result), Side::Sell);
    }

    void test_empty_add_message_rejects() {
        const ParseResult result = parse_message({ });

        assert_error(result, ParseError::EmptyMessage);
    }

    void test_invalid_size_add_message_rejects(const MessageType& add_type) {
        assert_rejects_incorrect_lengths(make_add_order_message(add_type, 'B'));
    }

    void test_invalid_side_add_message_rejects(const MessageType& add_type) {
        const auto message = make_add_order_message(add_type, 'X');

        const ParseResult result = parse_message(message);

        assert_error(result, ParseError::InvalidSide);
    }

    void test_unsupported_message_type_rejects() {
        auto message = make_add_order_message(kAddOrderTypes[0], 'B');
        message[0] = static_cast<std::uint8_t>('Z');

        const ParseResult result = parse_message(message);

        assert_error(result, ParseError::UnsupportedMessageType);
    }

    void test_invalid_size_execute_message_rejects(const MessageType& message_type) {
        assert_rejects_incorrect_lengths(make_execute_message(message_type));
    }

    void test_valid_execute_message_returns_event(const MessageType& message_type) {
        const auto message = make_execute_message(message_type);

        const ParseResult result = parse_message(message);

        assert_execute_order(assert_event(result));
    }

    void test_invalid_size_cancel_message_rejects() {
        assert_rejects_incorrect_lengths(make_cancel_message());
    }

    void test_valid_cancel_message_return_event() {
        const auto message = make_cancel_message();

        const ParseResult result = parse_message(message);

        assert_cancel_order(assert_event(result));
    }

    void test_invalid_size_delete_message_rejects() {
        assert_rejects_incorrect_lengths(make_delete_message());
    }

    void test_valid_delete_message_return_event() {
        const auto message = make_delete_message();

        const ParseResult result = parse_message(message);

        assert_delete_order(assert_event(result));
    }

    void test_invalid_size_replace_message_rejects() {
        assert_rejects_incorrect_lengths(make_replace_message());
    }

    void test_valid_replace_message_return_event() {
        const auto message = make_replace_message();

        const ParseResult result = parse_message(message);

        assert_replace_order(assert_event(result));
    }
}

int main() {
    for (const auto& add_type : kAddOrderTypes) {
        RUN_TYPED_TEST(test_valid_add_message_buy_returns_event, add_type);
        RUN_TYPED_TEST(test_valid_add_message_sell_returns_event, add_type);
        RUN_TYPED_TEST(test_invalid_size_add_message_rejects, add_type);
        RUN_TYPED_TEST(test_invalid_side_add_message_rejects, add_type);
    }

    for (const auto& execute_type : kExecuteMessageTypes) {
        RUN_TYPED_TEST(test_valid_execute_message_returns_event, execute_type);
        RUN_TYPED_TEST(test_invalid_size_execute_message_rejects, execute_type);
    }

    RUN_TEST(test_invalid_size_cancel_message_rejects);
    RUN_TEST(test_valid_cancel_message_return_event);
    RUN_TEST(test_invalid_size_delete_message_rejects);
    RUN_TEST(test_valid_delete_message_return_event);
    RUN_TEST(test_invalid_size_replace_message_rejects);
    RUN_TEST(test_valid_replace_message_return_event);

    RUN_TEST(test_empty_add_message_rejects);
    RUN_TEST(test_unsupported_message_type_rejects);

    return 0;
}
