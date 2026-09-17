cmake_minimum_required(VERSION 3.23)

project(trading-system LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(trading-system
    src/main.cpp
    src/book/order_book.cpp
    src/engine/trading_engine.cpp
    src/market_data/itch/parser.cpp
    src/order_entry/order_manager.cpp
    src/order_entry/ouch/encoder.cpp
    src/pipeline/trading_pipeline.cpp
    src/strategy/imbalance_strategy.cpp
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

    add_executable(itch-parser-tests
        tests/market_data/test_parser.cpp
        src/market_data/itch/parser.cpp
    )

    add_executable(order-manager-tests
        tests/order_entry/test_order_manager.cpp
        src/order_entry/order_manager.cpp
    )

    add_executable(ouch-encoder-tests
        tests/order_entry/test_ouch_encoder.cpp
        src/order_entry/ouch/encoder.cpp
    )

    add_executable(trading-pipeline-tests
        tests/pipeline/test_trading_pipeline.cpp
        src/pipeline/trading_pipeline.cpp
        src/market_data/itch/parser.cpp
        src/engine/trading_engine.cpp
        src/book/order_book.cpp
        src/strategy/imbalance_strategy.cpp
        src/order_entry/order_manager.cpp
        src/order_entry/ouch/encoder.cpp
    )

    target_include_directories(order-book-tests PRIVATE src)
    target_include_directories(imbalance-strategy-tests PRIVATE src)
    target_include_directories(trading-engine-tests PRIVATE src)
    target_include_directories(itch-parser-tests PRIVATE src)
    target_include_directories(order-manager-tests PRIVATE src)
    target_include_directories(ouch-encoder-tests PRIVATE src)
    target_include_directories(trading-pipeline-tests PRIVATE src)

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

    add_test(
        NAME itch-parser-tests
        COMMAND itch-parser-tests
    )

    add_test(
        NAME order-manager-tests
        COMMAND order-manager-tests
    )

    add_test(
        NAME ouch-encoder-tests
        COMMAND ouch-encoder-tests
    )

    add_test(
        NAME trading-pipeline-tests
        COMMAND trading-pipeline-tests
    )
endif()
