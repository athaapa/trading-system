#pragma once

#include "book_types.hpp"

#include <functional>
#include <map>
#include <optional>
#include <unordered_map>

/**
 * @brief Maintains the resting orders and aggregated price levels for one instrument.
 *
 * Orders are addressed by their reference number. The book exposes a complete top of book only
 * when both a bid and an ask exist. Every mutating operation reports whether its successful
 * application changed the value returned by top_of_book(). Rejected operations leave the book
 * unchanged and report no top-of-book change.
 */
class OrderBook {
public:
    /**
     * @brief Adds a new resting order to the book.
     *
     * @param order_num Unique reference number for the order.
     * @param side Side on which the order rests.
     * @param quantity Number of shares in the order.
     * @param price Limit price of the order.
     * @return An application result containing ApplyError::None on success,
     * ApplyError::InvalidQuantity when @p quantity is zero, ApplyError::InvalidPrice when @p price
     * is zero, or ApplyError::DuplicateOrder when @p order_num already exists.
     * @post On success, @p order_num identifies the new resting order and its quantity contributes
     * to the corresponding price level.
     * @post On failure, the book is unchanged.
     */
    ApplyResult add_order(
        OrderReferenceNumber order_num, Side side, Quantity quantity, Price price);

    /**
     * @brief Applies an execution to part or all of a resting order.
     *
     * @param order_num Reference number of the resting order.
     * @param quantity Number of shares executed.
     * @return An application result containing ApplyError::None on success,
     * ApplyError::InvalidQuantity when @p quantity is zero, ApplyError::UnknownOrder when
     * @p order_num does not exist, or ApplyError::QuantityExceeded when @p quantity exceeds the
     * order's remaining quantity.
     * @post On success, the order and its price level are reduced by @p quantity. The order is
     * removed if its remaining quantity reaches zero.
     * @post On failure, the book is unchanged.
     */
    ApplyResult execute_order(OrderReferenceNumber order_num, Quantity quantity);

    /**
     * @brief Cancels part or all of a resting order.
     *
     * @param order_num Reference number of the resting order.
     * @param quantity Number of shares cancelled.
     * @return An application result containing ApplyError::None on success,
     * ApplyError::InvalidQuantity when @p quantity is zero, ApplyError::UnknownOrder when
     * @p order_num does not exist, or ApplyError::QuantityExceeded when @p quantity exceeds the
     * order's remaining quantity.
     * @post On success, the order and its price level are reduced by @p quantity. The order is
     * removed if its remaining quantity reaches zero.
     * @post On failure, the book is unchanged.
     */
    ApplyResult cancel_order(OrderReferenceNumber order_num, Quantity quantity);

    /**
     * @brief Deletes a resting order and all of its remaining quantity.
     *
     * @param order_num Reference number of the resting order.
     * @return An application result containing ApplyError::None on success or
     * ApplyError::UnknownOrder when @p order_num does not exist.
     * @post On success, @p order_num no longer identifies a resting order and none of its remaining
     * quantity contributes to its price level.
     * @post On failure, the book is unchanged.
     */
    ApplyResult delete_order(OrderReferenceNumber order_num);

    /**
     * @brief Replaces a resting order while preserving its side.
     *
     * The old order is removed and a new order is inserted with the supplied reference, quantity,
     * and price.
     *
     * @param old_order_num Reference number of the order being replaced.
     * @param new_order_num Unique reference number assigned to the replacement.
     * @param quantity Quantity of the replacement.
     * @param price Price of the replacement.
     * @return An application result containing ApplyError::None on success,
     * ApplyError::InvalidQuantity when @p quantity is zero, ApplyError::InvalidPrice when @p price
     * is zero, ApplyError::UnknownOrder when @p old_order_num does not exist, or
     * ApplyError::DuplicateOrder when @p new_order_num already exists.
     * @post On success, @p old_order_num no longer identifies a resting order and @p new_order_num
     * identifies the replacement on the original order's side.
     * @post On failure, the book is unchanged.
     */
    ApplyResult replace_order(OrderReferenceNumber old_order_num,
        OrderReferenceNumber new_order_num, Quantity quantity, Price price);

    /**
     * @brief Returns the best bid and ask with their aggregated quantities.
     *
     * @return The current top of book, or std::nullopt when either side of the book is empty.
     */
    std::optional<TopOfBook> top_of_book() const;

private:
    std::unordered_map<OrderReferenceNumber, Order> orders_;

    std::map<Price, Quantity, std::greater<Price>> bids_;
    std::map<Price, Quantity> asks_;

    ApplyResult reduce_order(OrderReferenceNumber order_num, Quantity quantity);

    void add_level_quantity(Side side, Price price, Quantity quantity);
    void subtract_level_quantity(Side side, Price price, Quantity quantity);
};
