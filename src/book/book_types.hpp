#pragma once

#include <cstdint>
#include <string>

using Price = std::uint64_t;
using StockLocate = std::uint64_t;
using Quantity = std::uint32_t;
using Timestamp = std::uint64_t;
using OrderReferenceNumber = std::uint64_t;
using MPID = std::string;

enum class Side { Buy, Sell };

struct Order {
    Price price;
    Quantity quantity;
    Side side;
};

enum class ApplyError {
    None,
    UnknownOrder,
    QuantityExceeded,
    DuplicateOrder,
    InvalidQuantity,
    InvalidPrice
};

struct ApplyResult {
    ApplyError error;
    bool top_of_book_changed;
};

struct TopOfBook {
    Price bid_price;
    Quantity bid_quantity;
    Price ask_price;
    Quantity ask_quantity;

    bool operator==(const TopOfBook&) const = default;
};
