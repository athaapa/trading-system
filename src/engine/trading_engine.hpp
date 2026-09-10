#pragma once

#include "book/book_types.hpp"
#include "book/order_book.hpp"
#include "market_data/market_event.hpp"
#include "strategy/imbalance_strategy.hpp"
#include <optional>
#include <variant>

struct EventProcessed {
    std::optional<OrderIntent> intent;
};

struct EventRejected {
    ApplyError error;
};

using EngineResult = std::variant<EventProcessed, EventRejected>;

/**
 * @brief Maintains market state and runs a strategy for one instrument.
 *
 * A TradingEngine is bound to one StockLocate. For each market event,
 * it updates the order book first. It evaluates the strategy only when
 * the update successfully changes a complete top of book.
 *
 */
class TradingEngine {
public:
    /**
     * @brief Constructs an engine for one instrument.
     *
     * @param stock_locate Identifier of the instrument by this engine.
     */
    explicit TradingEngine(StockLocate stock_locate);

    /**
     * @brief Applies one market event and, when appropriate, evaluates
     * the strategy.
     *
     * @param event Decoded event for this engine's instrument.
     * @return EventRejected when the order book rejects the update.
     * Otherwise, returns EventProcessed containing an OrderIntent when
     * the strategy triggers, or std::nullopt when it does not.
     *
     * @pre event.stock_locate matches the instrument supplied to the
     * constructor.
     *
     * @post A rejected event leaves the order book unchanged and does
     * not evaluate the strategy.
     * @post A successfully applied event is reflected in the order book
     * before the strategy is evaluated.
     * @post The strategy is evaluated only when top_of_book_changed is
     * true and the book has both a bid and an ask.
     */
    EngineResult on_market_event(const MarketEvent& event);

private:
    StockLocate stock_locate_;
    OrderBook book_;
    ImbalanceStrategy strategy_;
};
