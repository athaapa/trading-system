#include "book/book_types.hpp"
#include "strategy/imbalance_strategy.hpp"

#include <cassert>
#include <iostream>
#include <optional>

namespace {

    template <typename Test> void run_test(const char* name, Test test) {
        std::cerr << "RUN " << name << '\n';
        test();
    }

#define RUN_TEST(test) run_test(#test, test)

    constexpr Price kBidPrice = 10;
    constexpr Price kAskPrice = 15;
    constexpr Quantity kHighQuantity = 100;
    constexpr Quantity kLowQuantity = 1;

    constexpr Quantity kBoundaryHighQuantity = 17;
    constexpr Quantity kBoundaryLowQuantity = 3;

    void test_positive_threshold_produces_buy_at_best_ask() {
        ImbalanceStrategy strategy;

        const TopOfBook top { .bid_price = kBidPrice,
            .bid_quantity = kHighQuantity,
            .ask_price = kAskPrice,
            .ask_quantity = kLowQuantity };

        std::optional<OrderIntent> intent = strategy.evaluate(top);

        assert(intent.has_value());
        assert(intent.value().price == kAskPrice);
        assert(intent.value().side == Side::Buy);
        assert(intent.value().quantity == 10);
    }

    void test_negative_threshold_produces_sell_at_best_bid() {
        ImbalanceStrategy strategy;

        const TopOfBook top { .bid_price = kBidPrice,
            .bid_quantity = kLowQuantity,
            .ask_price = kAskPrice,
            .ask_quantity = kHighQuantity };

        std::optional<OrderIntent> intent = strategy.evaluate(top);

        assert(intent.has_value());
        assert(intent.value().price == kBidPrice);
        assert(intent.value().side == Side::Sell);
        assert(intent.value().quantity == 10);
    }

    void test_balanced_book_produces_no_order() {
        ImbalanceStrategy strategy;

        const TopOfBook top { .bid_price = kBidPrice,
            .bid_quantity = kHighQuantity,
            .ask_price = kAskPrice,
            .ask_quantity = kHighQuantity };

        std::optional<OrderIntent> intent = strategy.evaluate(top);

        assert(!intent.has_value());
    }

    void test_positive_boundary_threshold_produces_buy_at_best_ask() {
        ImbalanceStrategy strategy;

        const TopOfBook top { .bid_price = kBidPrice,
            .bid_quantity = kBoundaryHighQuantity,
            .ask_price = kAskPrice,
            .ask_quantity = kBoundaryLowQuantity };

        std::optional<OrderIntent> intent = strategy.evaluate(top);

        assert(intent.has_value());
        assert(intent.value().price == kAskPrice);
        assert(intent.value().side == Side::Buy);
        assert(intent.value().quantity == 10);
    }

    void test_negative_boundary_threshold_produces_sell_at_best_bid() {
        ImbalanceStrategy strategy;

        const TopOfBook top { .bid_price = kBidPrice,
            .bid_quantity = kBoundaryLowQuantity,
            .ask_price = kAskPrice,
            .ask_quantity = kBoundaryHighQuantity };

        std::optional<OrderIntent> intent = strategy.evaluate(top);

        assert(intent.has_value());
        assert(intent.value().price == kBidPrice);
        assert(intent.value().side == Side::Sell);
        assert(intent.value().quantity == 10);
    }

    void test_positive_below_threshold_produces_no_order() {
        ImbalanceStrategy strategy;

        const TopOfBook top { .bid_price = kBidPrice,
            .bid_quantity = kBoundaryHighQuantity,
            .ask_price = kAskPrice,
            .ask_quantity = kBoundaryHighQuantity - 1 };

        std::optional<OrderIntent> intent = strategy.evaluate(top);

        assert(!intent.has_value());
    }

    void test_negative_below_threshold_produces_no_order() {
        ImbalanceStrategy strategy;

        const TopOfBook top { .bid_price = kBidPrice,
            .bid_quantity = kBoundaryHighQuantity - 1,
            .ask_price = kAskPrice,
            .ask_quantity = kBoundaryHighQuantity };

        std::optional<OrderIntent> intent = strategy.evaluate(top);

        assert(!intent.has_value());
    }
} // namespace

int main() {
    RUN_TEST(test_positive_threshold_produces_buy_at_best_ask);
    RUN_TEST(test_negative_threshold_produces_sell_at_best_bid);

    RUN_TEST(test_balanced_book_produces_no_order);

    RUN_TEST(test_positive_boundary_threshold_produces_buy_at_best_ask);
    RUN_TEST(test_negative_boundary_threshold_produces_sell_at_best_bid);

    RUN_TEST(test_positive_below_threshold_produces_no_order);
    RUN_TEST(test_negative_below_threshold_produces_no_order);

    return 0;
}
