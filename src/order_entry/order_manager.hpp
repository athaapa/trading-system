#pragma once

#include "book/book_types.hpp"
#include "strategy/imbalance_strategy.hpp"
#include <cstdint>
#include <unordered_map>
#include <variant>

using UserRefNum = uint32_t;
using Symbol = std::string;
enum class TimeInForce { DAY, IOC, GTX, GTT, AFTER_HOURS };
enum class Display { Y, N, A };
enum class Capacity { A, P, R, O };
enum class CrossType { N, O, C, H, S, R, E, A };

// TODO: appendages? see ouch docs
struct NewOrderRequest {
    UserRefNum user_ref_num;
    Symbol symbol;
    Side side;
    Quantity quantity;
    Price price;
};

enum class SubmitError {
    InvalidPrice,
    InvalidQuantity,
    OutstandingOrderLimitReached,
    OrderQuantityLimitReached,
    UserRefNumExhausted
};

enum class OrderStatus { PendingNew, Live, PendingCancel, Filled, Canceled, Rejected };

struct OrderRecord {
    UserRefNum id;
    Side side;
    Price price;

    Quantity original_quantity;
    Quantity remaining_quantity;

    OrderStatus status;
};

using SubmitResult = std::variant<NewOrderRequest, SubmitError>;

class OrderManager {
public:
    OrderManager(Symbol symbol, Quantity max_outstanding_quantity, size_t max_outstanding_orders);

    SubmitResult submit(const OrderIntent& intent);

    void on_order_accepted(UserRefNum order_num);
    void on_order_rejected(UserRefNum order_num);
    void on_order_executed(UserRefNum order_num, Quantity quantity);
    void on_order_cancelled(UserRefNum order_num, Quantity quantity);

private:
    UserRefNum next_user_ref_num_ = 1;
    Quantity max_outstanding_quantity_;
    Quantity current_outstanding_quantity_ = 0;
    size_t max_outstanding_orders_;
    Symbol symbol_;

    std::unordered_map<UserRefNum, OrderRecord> outstanding_orders_;
};
