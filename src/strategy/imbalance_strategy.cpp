#include "imbalance_strategy.hpp"
#include "book/book_types.hpp"
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <optional>

namespace {
    constexpr double kImbalanceThreshold = 0.7;
    constexpr Quantity kOrderQuantity = 10;
}

double ImbalanceStrategy::calculate_imbalance(Quantity bid_quantity, Quantity ask_quantity) const {
    assert(bid_quantity > 0);
    assert(ask_quantity > 0);

    const double bid = static_cast<double>(bid_quantity);
    const double ask = static_cast<double>(ask_quantity);

    return (bid - ask) / (bid + ask);
}

std::optional<OrderIntent> ImbalanceStrategy::evaluate(const TopOfBook& top) const {
    const double imbalance = calculate_imbalance(top.bid_quantity, top.ask_quantity);

    if (imbalance >= kImbalanceThreshold) {
        return OrderIntent {
            .side = Side::Buy, .price = top.ask_price, .quantity = kOrderQuantity
        };
    }

    if (imbalance <= -kImbalanceThreshold) {
        return OrderIntent {
            .side = Side::Sell, .price = top.bid_price, .quantity = kOrderQuantity
        };
    }
    return std::nullopt;
}
