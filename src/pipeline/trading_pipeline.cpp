#include "trading_pipeline.hpp"
#include "market_data/itch/parser.hpp"
#include "market_data/market_event.hpp"
#include "order_entry/order_manager.hpp"
#include "order_entry/ouch/encoder.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <variant>

TradingPipeline::TradingPipeline(StockLocate stock_locate, Symbol symbol,
    Quantity max_outstanding_quantity, std::size_t max_outstanding_orders)
    : engine_ { stock_locate }
    , order_manager_ { symbol, max_outstanding_quantity, max_outstanding_orders } { }

PipelineResult TradingPipeline::process(std::span<const std::uint8_t> itch_message) {
    const ParseResult parse_result = parse_message(itch_message);
    if (const auto* error = std::get_if<ParseError>(&parse_result)) {
        return *error;
    }

    const auto& event = std::get<MarketEvent>(parse_result);

    const EngineResult engine_result = engine_.on_market_event(event);
    if (const auto* rejection = std::get_if<EventRejected>(&engine_result)) {
        return rejection->error;
    }

    const auto& event_processed = std::get<EventProcessed>(engine_result);
    if (!event_processed.intent.has_value()) {
        return NoOrder { };
    }

    const SubmitResult submit_result = order_manager_.submit(*event_processed.intent);
    if (const auto* error = std::get_if<SubmitError>(&submit_result)) {
        return *error;
    }

    return encode_enter_order(std::get<NewOrderRequest>(submit_result));
}
