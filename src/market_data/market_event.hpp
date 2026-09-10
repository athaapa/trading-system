#pragma once

#include "book/book_types.hpp"
#include "book_update.hpp"

struct MarketEvent {
    StockLocate stock_locate;
    Timestamp timestamp;
    BookUpdate update;
};
