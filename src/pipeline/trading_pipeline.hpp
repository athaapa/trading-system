#pragma once

#include "engine/trading_engine.hpp"
#include "market_data/itch/parser.hpp"
#include "order_entry/order_manager.hpp"
#include "order_entry/ouch/encoder.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <variant>

struct NoOrder { };

using PipelineResult = std::variant<EncodedOrder, NoOrder, ParseError, ApplyError, SubmitError>;

class TradingPipeline {
public:
    TradingPipeline(StockLocate stock_locate, Symbol symbol, Quantity max_outstanding_quantity,
        std::size_t max_outstanding_orders);

    [[nodiscard]] PipelineResult process(std::span<const std::uint8_t> itch_message);

private:
    TradingEngine engine_;
    OrderManager order_manager_;
};
