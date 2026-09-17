#include "encoder.hpp"

#include <algorithm>
#include <cassert>
#include <concepts>
#include <string_view>

namespace {

    template <std::unsigned_integral T>
    void write_big_endian(EncodedOrder& message, std::size_t offset, T data, std::size_t size) {
        for (size_t i = 0; i < size; ++i) {
            const std::size_t shift = 8 * (size - i - 1);
            message[offset + i] = static_cast<std::uint8_t>((data >> shift) & 0xFF);
        }
    }

    void write_alpha(
        EncodedOrder& message, std::size_t offset, std::string_view value, std::size_t width) {

        const auto field_begin = message.begin() + offset;
        const auto field_end = field_begin + width;

        std::fill(field_begin, field_end, static_cast<std::uint8_t>(' '));
        std::copy(value.begin(), value.end(), field_begin);
    }
}

EncodedOrder encode_enter_order(const NewOrderRequest& request) {
    EncodedOrder message;
    message.fill(static_cast<std::uint8_t>(' '));

    message[0] = 'O';
    write_big_endian(message, 1, request.user_ref_num, 4);
    message[5] = request.side == Side::Buy ? 'B' : 'S';
    write_big_endian(message, 6, request.quantity, 4);
    write_alpha(message, 10, request.symbol, 8);
    write_big_endian(message, 18, request.price, 8);

    message[26] = '0'; // Day
    message[27] = 'Y'; // Visible
    message[28] = 'A'; // Agency
    message[29] = 'N'; // Not sweep eligible
    message[30] = 'N'; // Continuous market

    // Bytes 31–44: space-padded ClOrdID
    message[45] = 0;
    message[46] = 0; // No appendages

    return message;
}
