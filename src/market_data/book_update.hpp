#pragma once

#include "book/book_types.hpp"
#include <variant>

struct AddOrder {
    OrderReferenceNumber order_num;
    Side side;
    Quantity quantity;
    Price price;
};

struct ExecuteOrder {
    OrderReferenceNumber order_num;
    Quantity quantity;
};

struct CancelOrder {
    OrderReferenceNumber order_num;
    Quantity quantity;
};

struct DeleteOrder {
    OrderReferenceNumber order_num;
};

struct ReplaceOrder {
    OrderReferenceNumber old_order_num;
    OrderReferenceNumber new_order_num;
    Quantity quantity;
    Price price;
};

using BookUpdate = std::variant<AddOrder, ExecuteOrder, CancelOrder, DeleteOrder, ReplaceOrder>;
