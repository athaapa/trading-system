#include "order_entry/ouch/encoder.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>

namespace {
    template <typename Test> void run_test(const char* name, Test test) {
        std::cerr << "RUN " << name << '\n';
        test();
    }

#define RUN_TEST(test) run_test(#test, test)

    constexpr UserRefNum kUserRefNum = 0x01020304;
    constexpr Quantity kQuantity = 0x00010203;
    constexpr Price kPrice = 0x12345678;

    NewOrderRequest make_request(Side side = Side::Buy) {
        return NewOrderRequest {
            .user_ref_num = kUserRefNum,
            .symbol = "AAPL",
            .side = side,
            .quantity = kQuantity,
            .price = kPrice,
        };
    }

    template <std::size_t Size>
    void assert_bytes(const EncodedOrder& message, std::size_t offset,
        const std::array<std::uint8_t, Size>& expected) {
        assert(std::equal(expected.begin(), expected.end(), message.begin() + offset));
    }

    void test_enter_order_encodes_numeric_fields_big_endian() {
        const EncodedOrder message = encode_enter_order(make_request());

        assert_bytes(message, 1, std::array<std::uint8_t, 4> { 0x01, 0x02, 0x03, 0x04 });
        assert_bytes(message, 6, std::array<std::uint8_t, 4> { 0x00, 0x01, 0x02, 0x03 });
        assert_bytes(message, 18,
            std::array<std::uint8_t, 8> {
                0x00,
                0x00,
                0x00,
                0x00,
                0x12,
                0x34,
                0x56,
                0x78,
            });
    }

    void test_enter_order_encodes_alpha_and_default_fields() {
        const EncodedOrder message = encode_enter_order(make_request());

        assert(message[0] == static_cast<std::uint8_t>('O'));
        assert(message[5] == static_cast<std::uint8_t>('B'));
        assert_bytes(
            message, 10, std::array<std::uint8_t, 8> { 'A', 'A', 'P', 'L', ' ', ' ', ' ', ' ' });
        assert(message[26] == static_cast<std::uint8_t>('0'));
        assert(message[27] == static_cast<std::uint8_t>('Y'));
        assert(message[28] == static_cast<std::uint8_t>('A'));
        assert(message[29] == static_cast<std::uint8_t>('N'));
        assert(message[30] == static_cast<std::uint8_t>('N'));

        for (std::size_t offset = 31; offset < 45; ++offset) {
            assert(message[offset] == static_cast<std::uint8_t>(' '));
        }

        assert(message[45] == 0);
        assert(message[46] == 0);
    }

    void test_enter_order_encodes_sell_side() {
        const EncodedOrder message = encode_enter_order(make_request(Side::Sell));

        assert(message[5] == static_cast<std::uint8_t>('S'));
    }
}

int main() {
    RUN_TEST(test_enter_order_encodes_numeric_fields_big_endian);
    RUN_TEST(test_enter_order_encodes_alpha_and_default_fields);
    RUN_TEST(test_enter_order_encodes_sell_side);

    return 0;
}
