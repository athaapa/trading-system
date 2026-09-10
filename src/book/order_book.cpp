#include "order_book.hpp"
#include "book_types.hpp"
#include <cassert>

std::optional<TopOfBook> OrderBook::top_of_book() const {
    if (bids_.empty() || asks_.empty()) {
        return std::nullopt;
    }

    const auto& [bid_price, bid_quantity] = *bids_.begin();
    const auto& [ask_price, ask_quantity] = *asks_.begin();

    return TopOfBook {
        .bid_price = bid_price,
        .bid_quantity = bid_quantity,
        .ask_price = ask_price,
        .ask_quantity = ask_quantity,
    };
}

void OrderBook::add_level_quantity(Side side, Price price, Quantity quantity) {
    if (side == Side::Buy) {
        bids_[price] += quantity;
    } else {
        asks_[price] += quantity;
    }
}
void OrderBook::subtract_level_quantity(Side side, Price price, Quantity quantity) {
    if (side == Side::Buy) {
        auto it = bids_.find(price);
        assert(it != bids_.end());
        assert(it->second >= quantity);
        it->second -= quantity;
        if (it->second == 0) {
            bids_.erase(it);
        }
    } else {
        auto it = asks_.find(price);
        assert(it != asks_.end());
        assert(it->second >= quantity);
        it->second -= quantity;
        if (it->second == 0) {
            asks_.erase(it);
        }
    }
}

ApplyResult OrderBook::add_order(
    OrderReferenceNumber order_num, Side side, Quantity quantity, Price price) {

    if (quantity == 0) {
        return { ApplyError::InvalidQuantity, false };
    }

    if (price == 0) {
        return { ApplyError::InvalidPrice, false };
    }

    const auto before = top_of_book();

    if (orders_.find(order_num) != orders_.end()) {
        return { ApplyError::DuplicateOrder, false };
    }

    Order order = { .price = price, .quantity = quantity, .side = side };

    orders_.emplace(order_num, order);
    add_level_quantity(side, price, quantity);

    const bool top_of_book_changed = before != top_of_book();

    return { ApplyError::None, top_of_book_changed };
}

ApplyResult OrderBook::reduce_order(OrderReferenceNumber order_num, Quantity quantity) {
    if (quantity == 0) {
        return { ApplyError::InvalidQuantity, false };
    }

    const auto before = top_of_book();

    auto it = orders_.find(order_num);

    if (it == orders_.end()) {
        return { ApplyError::UnknownOrder, false };
    }

    if (quantity > it->second.quantity) {
        return { ApplyError::QuantityExceeded, false };
    }

    Order& order = it->second;

    order.quantity -= quantity;
    subtract_level_quantity(order.side, order.price, quantity);

    if (order.quantity == 0) {
        orders_.erase(it);
    }

    const bool top_of_book_changed = before != top_of_book();

    return { ApplyError::None, top_of_book_changed };
}

ApplyResult OrderBook::execute_order(OrderReferenceNumber order_num, Quantity quantity) {
    return reduce_order(order_num, quantity);
}

ApplyResult OrderBook::cancel_order(OrderReferenceNumber order_num, Quantity quantity) {
    return reduce_order(order_num, quantity);
}

ApplyResult OrderBook::delete_order(OrderReferenceNumber order_num) {
    const auto before = top_of_book();

    auto it = orders_.find(order_num);
    if (it == orders_.end()) {
        return { ApplyError::UnknownOrder, false };
    }

    Order& order = it->second;
    subtract_level_quantity(order.side, order.price, order.quantity);

    orders_.erase(it);

    const bool top_of_book_changed = before != top_of_book();

    return { ApplyError::None, top_of_book_changed };
}

ApplyResult OrderBook::replace_order(OrderReferenceNumber old_order_num,
    OrderReferenceNumber new_order_num, Quantity quantity, Price price) {
    if (quantity == 0) {
        return { ApplyError::InvalidQuantity, false };
    }

    if (price == 0) {
        return { ApplyError::InvalidPrice, false };
    }

    const auto before = top_of_book();

    auto it = orders_.find(old_order_num);
    if (it == orders_.end()) {
        return { ApplyError::UnknownOrder, false };
    }

    if (orders_.find(new_order_num) != orders_.end()) {
        return { ApplyError::DuplicateOrder, false };
    }

    const Order old_order = it->second;
    subtract_level_quantity(old_order.side, old_order.price, old_order.quantity);

    orders_.erase(it);

    Order replacement { .price = price, .quantity = quantity, .side = old_order.side };

    add_level_quantity(replacement.side, replacement.price, replacement.quantity);

    orders_.emplace(new_order_num, replacement);

    const bool top_of_book_changed = before != top_of_book();

    return { ApplyError::None, top_of_book_changed };
}
