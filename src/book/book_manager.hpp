#pragma once

#include "order_book.hpp"

#include <map>

class BookManager {
public:
    // ApplyResult apply(const ItchMessage& message);
    const OrderBook* find_book(StockLocate stock) const;

private:
    std::map<StockLocate, OrderBook> books_;
};
