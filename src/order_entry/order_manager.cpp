#include "order_manager.hpp"
#include "strategy/imbalance_strategy.hpp"

OrderManager::OrderManager(
    Symbol symbol, Quantity max_outstanding_quantity, size_t max_outstanding_orders)
    : symbol_ { symbol }
    , max_outstanding_orders_ { max_outstanding_orders }
    , max_outstanding_quantity_ { max_outstanding_quantity } { }

SubmitResult OrderManager::submit(const OrderIntent& intent) {
    if (outstanding_orders_.size() >= max_outstanding_orders_) {
        return SubmitError::OutstandingOrderLimitReached;
    }

    if (current_outstanding_quantity_ + intent.quantity > max_outstanding_quantity_) {
        return SubmitError::OrderQuantityLimitReached;
    }

    const UserRefNum id = next_user_ref_num_;

    OrderRecord record {
        .id = id,
        .side = intent.side,
        .price = intent.price,
        .original_quantity = intent.quantity,
        .remaining_quantity = intent.quantity,
        .status = OrderStatus::PendingNew,
    };

    NewOrderRequest request {
        .user_ref_num = id,
        .symbol = symbol_,
        .side = intent.side,
        .quantity = intent.quantity,
        .price = intent.price,
    };

    outstanding_orders_.emplace(id, record);
    current_outstanding_quantity_ += intent.quantity;
    ++next_user_ref_num_;
    return request;
}
