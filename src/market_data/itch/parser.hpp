#pragma once

#include "market_data/market_event.hpp"

#include <cstdint>
#include <span>
#include <variant>

enum class ParseError { EmptyMessage, IncorrectMessageLength, UnsupportedMessageType, InvalidSide };

using ParseResult = std::variant<MarketEvent, ParseError>;

/**
 * @brief Parses an ITCH message into a MarketEvent or returns a ParseError on failure
 *
 * @param message The binary ITCH message to parse
 * @return ParseError if the message failed to parse or MarketEvent if it parsed successfully
 *
 */
[[nodiscard]]
ParseResult parse_message(std::span<const uint8_t> message);
