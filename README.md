# Trading System

A C++20 prototype that converts Nasdaq ITCH 5.0 order-book messages into an OUCH 5.0 Enter Order message.

The current version runs the complete decision path in one process:

```text
ITCH bytes
    -> decode market event
    -> update order book
    -> evaluate strategy
    -> apply order limits
    -> encode OUCH bytes
```

The executable runs a deterministic two-message demonstration. A bid initializes one side of the book. An ask completes an imbalanced top of book, which produces a 47-byte OUCH Enter Order message.

This version accepts one complete ITCH payload at a time. It does not include network transport, feed framing, recovery, or exchange-response handling.

## Strategy

The strategy uses top-of-book queue imbalance.[^1] An imbalance of at least `0.7` produces a buy intent at the best ask. An imbalance of at most `-0.7` produces a sell intent at the best bid.

The strategy provides a realistic decision workload for the system. It is not presented as a profitable trading strategy.

## Build

```bash
cmake -S . -B build
cmake --build build
```

## Run

```bash
./build/trading-system
```

## Test

```bash
ctest --test-dir build --output-on-failure
```

## Future work

The next milestone adds a replay sender and order receiver around the current pipeline. Later work can add exchange framing, order acknowledgements, and latency measurements.

[^1]: Martin D. Gould and Julius Bonart, "[Queue Imbalance as a One-Tick-Ahead Price Predictor in a Limit Order Book](https://arxiv.org/abs/1512.03492)" (2015).
