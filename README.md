# Trading System

An event-driven trading system prototype written in C++20.

The project's end goal is a complete receive-to-send pipeline: decode exchange market data, reconstruct the order book, evaluate a strategy, and encode an outbound order message.

Currently implemented are the order book, strategy, and the engine connecting them.

The strategy uses top-of-book queue imbalance.[^1] An imbalance of at least `0.7` produces a buy intent at the best ask. An imbalance of at most `-0.7` produces a sell intent at the best bid.

The strategy provides a realistic decision workload for the system. It is not presented as a profitable trading strategy.

## Build

```bash
cmake -S . -B build
cmake --build build
```

## Test

```bash
ctest --test-dir build --output-on-failure
```

[^1]: Martin D. Gould and Julius Bonart, "[Queue Imbalance as a One-Tick-Ahead Price Predictor in a Limit Order Book](https://arxiv.org/abs/1512.03492)" (2015).
