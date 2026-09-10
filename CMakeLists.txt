cmake_minimum_required(VERSION 3.23)

project(trading-system LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(trading-system
    src/main.cpp
    src/book/order_book.cpp
)

target_include_directories(trading-system PRIVATE src)

include(CTest)

if(BUILD_TESTING)
    add_executable(order-book-tests
        tests/book/test_order_book.cpp
        src/book/order_book.cpp
    )

    add_executable(imbalance-strategy-tests
        tests/strategy/test_imbalance_strategy.cpp
        src/strategy/imbalance_strategy.cpp
    )

    add_executable(trading-engine-tests
        tests/engine/test_trading_engine.cpp
        src/engine/trading_engine.cpp
        src/book/order_book.cpp
        src/strategy/imbalance_strategy.cpp
    )

    target_include_directories(order-book-tests PRIVATE src)
    target_include_directories(imbalance-strategy-tests PRIVATE src)
    target_include_directories(trading-engine-tests PRIVATE src)

    add_test(
        NAME order-book-tests
        COMMAND order-book-tests
    )

    add_test(
        NAME imbalance-strategy-tests
        COMMAND imbalance-strategy-tests
    )

    add_test(
        NAME trading-engine-tests
        COMMAND trading-engine-tests
    )
endif()
