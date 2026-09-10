#pragma once

#include "book/book_types.hpp"
#include <optional>

struct OrderIntent {
    Side side;
    Price price;
    Quantity quantity;
};

/**
 * @brief Produces orders from top-of-book queue imbalance.
 *
 * Imbalance is:
 *
 *      bid_quantity - ask_quantity
 *     ----------------------------
 *      bid_quantity + ask_quantity
 *
 *
 * Each evaluation is independent:
 * - Imbalance >= 0.7 produces a buy at the best ask.
 * - Imbalance <= -0.7 produces a sell at the best bid.
 * - Otherwise, no order is produced.
 *
 * Every produced order has a quantity of 10.
 *
 */
class ImbalanceStrategy {
public:
    /**
     * @brief evaluates one complete top-of-book state.
     *
     * @param top Current best bid and ask.
     * @return The order to submit, or std::nullopt when the imbalance does not reach the threshold.
     *
     * @pre Both bid and ask quantities are greater than zero.
     */
    std::optional<OrderIntent> evaluate(const TopOfBook& top) const;

private:
    double calculate_imbalance(Quantity bid_quantity, Quantity ask_quantity) const;
};
