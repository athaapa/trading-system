#include "book/book_types.hpp"
#include "order_entry/order_manager.hpp"
#include <cassert>
#include <cstddef>
#include <iostream>
#include <variant>

namespace {
    template <typename Test> void run_test(const char* name, Test test) {
        std::cerr << "RUN " << name << '\n';
        test();
    }

#define RUN_TEST(test) run_test(#test, test)

    constexpr Symbol kSymbol = "AAPL";
    constexpr Quantity kMaxQuantity = 100;
    constexpr size_t kMaxOrders = 2;

    OrderIntent make_intent(Quantity quantity = 10) {
        return OrderIntent {
            .side = Side::Buy,
            .price = 100,
            .quantity = quantity,
        };
    }

    void assert_error(const SubmitResult& result, SubmitError expected) {
        assert(std::holds_alternative<SubmitError>(result));
        assert(std::get<SubmitError>(result) == expected);
    }

    void test_valid_intent_returns_request() {
        OrderManager manager(kSymbol, kMaxQuantity, kMaxOrders);
        const OrderIntent intent = make_intent();

        SubmitResult result = manager.submit(intent);
        const auto* request = std::get_if<NewOrderRequest>(&result);
        assert(request != nullptr);
        assert(request->user_ref_num == 1);
        assert(request->symbol == kSymbol);
        assert(request->side == intent.side);
        assert(request->quantity == intent.quantity);
        assert(request->price == intent.price);
    }

    void test_excess_intent_returns_order_limit_reached() {
        OrderManager manager(kSymbol, kMaxQuantity, kMaxOrders);

        manager.submit(make_intent());
        manager.submit(make_intent());

        const SubmitResult result = manager.submit(make_intent());

        assert_error(result, SubmitError::OutstandingOrderLimitReached);
    }

    void test_excess_quantity_returns_quantity_limit_reached() {
        OrderManager manager(kSymbol, kMaxQuantity, kMaxOrders);

        manager.submit(make_intent(60));

        const SubmitResult result = manager.submit(make_intent(41));

        assert_error(result, SubmitError::OrderQuantityLimitReached);
    }
};

int main() {
    RUN_TEST(test_valid_intent_returns_request);
    RUN_TEST(test_excess_intent_returns_order_limit_reached);
    RUN_TEST(test_excess_quantity_returns_quantity_limit_reached);

    return 0;
}
