#include "book/book_types.hpp"
#include "engine/trading_engine.hpp"
#include "market_data/book_update.hpp"
#include "market_data/market_event.hpp"
#include <cassert>
#include <iostream>
#include <variant>

namespace {
    template <typename Test> void run_test(const char* name, Test test) {
        std::cerr << "RUN " << name << '\n';
        test();
    }

#define RUN_TEST(test) run_test(#test, test)

    const EventProcessed& assert_processed(const EngineResult& result) {
        assert(std::holds_alternative<EventProcessed>(result));
        return std::get<EventProcessed>(result);
    }

    const EventRejected& assert_rejected(const EngineResult& result) {
        assert(std::holds_alternative<EventRejected>(result));
        return std::get<EventRejected>(result);
    }

    void assert_processed_without_intent(const EngineResult& result) {
        const auto& processed = assert_processed(result);
        assert(!processed.intent.has_value());
    }

    void assert_processed_with_intent(const EngineResult& result, Side expected_side,
        Price expected_price, Quantity expected_quantity) {
        const auto& processed = assert_processed(result);

        assert(processed.intent.has_value());
        assert(processed.intent->side == expected_side);
        assert(processed.intent->price == expected_price);
        assert(processed.intent->quantity == expected_quantity);
    }

    void test_add_that_completes_imbalanced_book_returns_buy_intent() {
        TradingEngine engine(0);

        const MarketEvent buy_order { .stock_locate = 0,
            .timestamp = 1,
            .update
            = AddOrder { .order_num = 1, .side = Side::Buy, .quantity = 17, .price = 100 } };

        const EngineResult buy_result = engine.on_market_event(buy_order);
        assert_processed_without_intent(buy_result);

        const MarketEvent sell_order { .stock_locate = 0,
            .timestamp = 2,
            .update
            = AddOrder { .order_num = 2, .side = Side::Sell, .quantity = 3, .price = 101 } };
        const EngineResult sell_result = engine.on_market_event(sell_order);

        assert_processed_with_intent(sell_result, Side::Buy, 101, 10);
    }

    void test_execute_that_changes_top_returns_strategy_intent() {
        TradingEngine engine(0);

        const MarketEvent buy_order { .stock_locate = 0,
            .timestamp = 1,
            .update
            = AddOrder { .order_num = 1, .side = Side::Buy, .quantity = 20, .price = 100 } };

        const MarketEvent sell_order { .stock_locate = 0,
            .timestamp = 3,
            .update
            = AddOrder { .order_num = 2, .side = Side::Sell, .quantity = 10, .price = 101 } };

        const MarketEvent execute_order { .stock_locate = 0,
            .timestamp = 4,
            .update = ExecuteOrder { .order_num = 2, .quantity = 7 } };

        const EngineResult buy_result = engine.on_market_event(buy_order);
        assert_processed_without_intent(buy_result);

        const EngineResult sell_result = engine.on_market_event(sell_order);
        assert_processed_without_intent(sell_result);

        const EngineResult execute_result = engine.on_market_event(execute_order);
        assert_processed_with_intent(execute_result, Side::Buy, 101, 10);
    }

    void test_cancel_at_non_best_price_does_not_retrigger_strategy() {
        TradingEngine engine(0);

        const MarketEvent buy_order { .stock_locate = 0,
            .timestamp = 1,
            .update
            = AddOrder { .order_num = 1, .side = Side::Buy, .quantity = 17, .price = 100 } };

        const EngineResult buy_result = engine.on_market_event(buy_order);
        assert_processed_without_intent(buy_result);

        const MarketEvent sell_order { .stock_locate = 0,
            .timestamp = 2,
            .update
            = AddOrder { .order_num = 2, .side = Side::Sell, .quantity = 3, .price = 101 } };
        const EngineResult sell_result = engine.on_market_event(sell_order);

        assert_processed_with_intent(sell_result, Side::Buy, 101, 10);

        const MarketEvent buy_order2 { .stock_locate = 0,
            .timestamp = 3,
            .update = AddOrder { .order_num = 3, .side = Side::Buy, .quantity = 5, .price = 99 } };

        const EngineResult buy_result2 = engine.on_market_event(buy_order2);
        assert_processed_without_intent(buy_result2);

        const MarketEvent cancel_order { .stock_locate = 0,
            .timestamp = 4,
            .update = CancelOrder { .order_num = 3, .quantity = 1 } };

        const EngineResult cancel_result = engine.on_market_event(cancel_order);

        assert_processed_without_intent(cancel_result);
    }

    void test_delete_last_order_on_one_side_returns_no_intent() {
        TradingEngine engine(0);

        const MarketEvent buy_order { .stock_locate = 0,
            .timestamp = 1,
            .update
            = AddOrder { .order_num = 1, .side = Side::Buy, .quantity = 17, .price = 100 } };

        const EngineResult buy_result = engine.on_market_event(buy_order);
        assert_processed_without_intent(buy_result);

        const MarketEvent sell_order { .stock_locate = 0,
            .timestamp = 2,
            .update
            = AddOrder { .order_num = 2, .side = Side::Sell, .quantity = 3, .price = 101 } };
        const EngineResult sell_result = engine.on_market_event(sell_order);

        assert_processed_with_intent(sell_result, Side::Buy, 101, 10);

        const MarketEvent delete_order {
            .stock_locate = 0, .timestamp = 3, .update = DeleteOrder { .order_num = 2 }
        };
        const EngineResult delete_result = engine.on_market_event(delete_order);
        assert_processed_without_intent(delete_result);
    }

    void test_replace_that_changes_top_to_balanced_returns_no_intent() {
        TradingEngine engine(0);

        const MarketEvent buy_order { .stock_locate = 0,
            .timestamp = 1,
            .update
            = AddOrder { .order_num = 1, .side = Side::Buy, .quantity = 17, .price = 100 } };

        const EngineResult buy_result = engine.on_market_event(buy_order);
        assert_processed_without_intent(buy_result);

        const MarketEvent sell_order { .stock_locate = 0,
            .timestamp = 2,
            .update
            = AddOrder { .order_num = 2, .side = Side::Sell, .quantity = 3, .price = 101 } };
        const EngineResult sell_result = engine.on_market_event(sell_order);

        assert_processed_with_intent(sell_result, Side::Buy, 101, 10);

        const MarketEvent replace_order { .stock_locate = 0,
            .timestamp = 3,
            .update = ReplaceOrder {
                .old_order_num = 1, .new_order_num = 3, .quantity = 16, .price = 100 } };
        const EngineResult replace_result = engine.on_market_event(replace_order);
        assert_processed_without_intent(replace_result);
    }
    void test_rejected_book_update_returns_event_rejected() {
        TradingEngine engine(0);

        const MarketEvent buy_order {
            .stock_locate = 0, .timestamp = 1, .update = DeleteOrder { .order_num = 1 }
        };

        const EngineResult delete_result = engine.on_market_event(buy_order);
        EventRejected rejection = assert_rejected(delete_result);
        assert(rejection.error == ApplyError::UnknownOrder);
    }
}

int main() {
    RUN_TEST(test_add_that_completes_imbalanced_book_returns_buy_intent);
    RUN_TEST(test_rejected_book_update_returns_event_rejected);
    RUN_TEST(test_delete_last_order_on_one_side_returns_no_intent);
    RUN_TEST(test_execute_that_changes_top_returns_strategy_intent);
    RUN_TEST(test_replace_that_changes_top_to_balanced_returns_no_intent);
    RUN_TEST(test_cancel_at_non_best_price_does_not_retrigger_strategy);

    return 0;
}
