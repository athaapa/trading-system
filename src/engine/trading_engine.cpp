#include "trading_engine.hpp"
#include "book/book_types.hpp"
#include "market_data/market_event.hpp"
#include <cassert>

namespace {

    ApplyResult apply_update(OrderBook& book, const AddOrder& update) {
        return book.add_order(update.order_num, update.side, update.quantity, update.price);
    }

    ApplyResult apply_update(OrderBook& book, const ExecuteOrder& update) {
        return book.execute_order(update.order_num, update.quantity);
    }

    ApplyResult apply_update(OrderBook& book, const CancelOrder& update) {
        return book.cancel_order(update.order_num, update.quantity);
    }

    ApplyResult apply_update(OrderBook& book, const DeleteOrder& update) {
        return book.delete_order(update.order_num);
    }

    ApplyResult apply_update(OrderBook& book, const ReplaceOrder& update) {
        return book.replace_order(
            update.old_order_num, update.new_order_num, update.quantity, update.price);
    }

    ApplyResult apply_update(OrderBook& book, const BookUpdate& update) {
        return std::visit(
            [&](const auto& concrete_update) { return apply_update(book, concrete_update); },
            update);
    }

} // namespace

EngineResult TradingEngine::on_market_event(const MarketEvent& event) {
    assert(stock_locate_ == event.stock_locate);

    const ApplyResult result = apply_update(book_, event.update);

    if (result.error != ApplyError::None) {
        return EventRejected { .error = result.error };
    }

    if (!result.top_of_book_changed) {
        return EventProcessed { .intent = std::nullopt };
    }

    const auto top = book_.top_of_book();
    if (!top.has_value()) {
        return EventProcessed { .intent = std::nullopt };
    }

    return EventProcessed { .intent = strategy_.evaluate(*top) };
};

TradingEngine::TradingEngine(StockLocate stock_locate)
    : stock_locate_ { stock_locate } { }
