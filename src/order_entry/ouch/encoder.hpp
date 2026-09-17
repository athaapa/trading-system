#pragma once

#include "order_entry/order_manager.hpp"
#include <array>
#include <cstddef>
#include <cstdint>

constexpr std::size_t kEnterOrderSize = 47;

using EncodedOrder = std::array<std::uint8_t, kEnterOrderSize>;

EncodedOrder encode_enter_order(const NewOrderRequest& request);
