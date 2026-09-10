#include "book/book_types.hpp"
#include "book/order_book.hpp"

#include <array>
#include <cassert>
#include <iostream>

namespace {

    constexpr Price kBestBid = 100;
    constexpr Price kBestAsk = 200;
    constexpr Quantity kQuantity = 10;
    constexpr OrderReferenceNumber kBidOrder = 1;
    constexpr OrderReferenceNumber kAskOrder = 2;

    using ReductionOperation = ApplyResult (OrderBook::*)(OrderReferenceNumber, Quantity);

    struct NamedReductionOperation {
        const char* name;
        ReductionOperation operation;
    };

    constexpr std::array<NamedReductionOperation, 2> kReductionOperations {
        NamedReductionOperation { "execute", &OrderBook::execute_order },
        NamedReductionOperation { "cancel", &OrderBook::cancel_order },
    };

    template <typename Test> void run_test(const char* name, Test test) {
        std::cerr << "RUN " << name << '\n';
        test();
    }

    template <typename Test>
    void run_reduction_test(const char* name, Test test, const NamedReductionOperation& reduction) {
        std::cerr << "RUN " << name << " [" << reduction.name << "]\n";
        test(reduction.operation);
    }

#define RUN_TEST(test) run_test(#test, test)
#define RUN_REDUCTION_TEST(test, reduction) run_reduction_test(#test, test, reduction)

    void add_complete_book(OrderBook& book) {
        const auto bid_result = book.add_order(kBidOrder, Side::Buy, kQuantity, kBestBid);
        const auto ask_result = book.add_order(kAskOrder, Side::Sell, kQuantity, kBestAsk);

        assert(bid_result.error == ApplyError::None);
        assert(ask_result.error == ApplyError::None);
    }

    void assert_top(const OrderBook& book, Price bid_price, Quantity bid_quantity, Price ask_price,
        Quantity ask_quantity) {
        const auto top = book.top_of_book();

        assert(top.has_value());
        assert(top->bid_price == bid_price);
        assert(top->bid_quantity == bid_quantity);
        assert(top->ask_price == ask_price);
        assert(top->ask_quantity == ask_quantity);
    }

    void assert_rejected_without_top_change(const ApplyResult& result, ApplyError expected_error) {
        assert(result.error == expected_error);
        assert(!result.top_of_book_changed);
    }

    void test_empty_book_has_no_top() {
        OrderBook book;

        assert(!book.top_of_book().has_value());
    }

    void test_bid_only_book_has_no_top() {
        OrderBook book;

        const auto result = book.add_order(1, Side::Buy, 100, 5);

        assert(result.error == ApplyError::None);
        assert(!result.top_of_book_changed);
        assert(!book.top_of_book().has_value());
    }

    void test_ask_only_book_has_no_top() {
        OrderBook book;

        const auto result = book.add_order(1, Side::Sell, 100, 5);

        assert(result.error == ApplyError::None);
        assert(!result.top_of_book_changed);
        assert(!book.top_of_book().has_value());
    }

    void test_add_ask_to_bid_only_book_creates_top() {
        OrderBook book;
        book.add_order(kBidOrder, Side::Buy, kQuantity, kBestBid);

        const auto result = book.add_order(kAskOrder, Side::Sell, kQuantity, kBestAsk);

        assert(result.error == ApplyError::None);
        assert(result.top_of_book_changed);
        assert_top(book, kBestBid, kQuantity, kBestAsk, kQuantity);
    }

    void test_add_rejects_zero_quantity() {
        OrderBook book;
        add_complete_book(book);
        const auto before = book.top_of_book();

        const auto result = book.add_order(3, Side::Buy, 0, 110);

        assert_rejected_without_top_change(result, ApplyError::InvalidQuantity);
        assert(book.top_of_book() == before);

        const auto reuse_result = book.add_order(3, Side::Buy, 4, 90);
        assert(reuse_result.error == ApplyError::None);
        assert(!reuse_result.top_of_book_changed);
        assert(book.top_of_book() == before);
    }

    void test_add_rejects_zero_price() {
        OrderBook book;
        add_complete_book(book);
        const auto before = book.top_of_book();

        const auto result = book.add_order(3, Side::Sell, 100, 0);

        assert_rejected_without_top_change(result, ApplyError::InvalidPrice);
        assert(book.top_of_book() == before);

        const auto reuse_result = book.add_order(3, Side::Buy, 4, 90);
        assert(reuse_result.error == ApplyError::None);
        assert(!reuse_result.top_of_book_changed);
        assert(book.top_of_book() == before);
    }

    void test_add_rejects_duplicate_reference_without_changing_book() {
        OrderBook book;
        add_complete_book(book);
        const auto before = book.top_of_book();

        const auto result = book.add_order(kBidOrder, Side::Sell, 50, 150);

        assert_rejected_without_top_change(result, ApplyError::DuplicateOrder);
        assert(book.top_of_book() == before);

        const auto reduction = book.execute_order(kBidOrder, 4);
        assert(reduction.error == ApplyError::None);
        assert(reduction.top_of_book_changed);
        assert_top(book, kBestBid, 6, kBestAsk, kQuantity);
    }

    void test_add_second_order_at_best_bid_increases_bid_quantity() {
        OrderBook book;
        add_complete_book(book);

        const auto result = book.add_order(3, Side::Buy, 7, kBestBid);

        assert(result.error == ApplyError::None);
        assert(result.top_of_book_changed);
        assert_top(book, kBestBid, 17, kBestAsk, kQuantity);
    }

    void test_add_second_order_at_best_ask_increases_ask_quantity() {
        OrderBook book;
        add_complete_book(book);

        const auto result = book.add_order(3, Side::Sell, 7, kBestAsk);

        assert(result.error == ApplyError::None);
        assert(result.top_of_book_changed);
        assert_top(book, kBestBid, kQuantity, kBestAsk, 17);
    }

    void test_add_better_bid_changes_top() {
        OrderBook book;
        add_complete_book(book);

        const auto result = book.add_order(3, Side::Buy, 7, 110);

        assert(result.error == ApplyError::None);
        assert(result.top_of_book_changed);
        assert_top(book, 110, 7, kBestAsk, kQuantity);
    }

    void test_add_lower_ask_changes_top() {
        OrderBook book;
        add_complete_book(book);

        const auto result = book.add_order(3, Side::Sell, 7, 190);

        assert(result.error == ApplyError::None);
        assert(result.top_of_book_changed);
        assert_top(book, kBestBid, kQuantity, 190, 7);
    }

    void test_add_worse_bid_does_not_change_top() {
        OrderBook book;
        add_complete_book(book);
        const auto before = book.top_of_book();

        const auto result = book.add_order(3, Side::Buy, 7, 90);

        assert(result.error == ApplyError::None);
        assert(!result.top_of_book_changed);
        assert(book.top_of_book() == before);
    }

    void test_add_higher_ask_does_not_change_top() {
        OrderBook book;
        add_complete_book(book);
        const auto before = book.top_of_book();

        const auto result = book.add_order(3, Side::Sell, 7, 210);

        assert(result.error == ApplyError::None);
        assert(!result.top_of_book_changed);
        assert(book.top_of_book() == before);
    }

    void test_reduction_rejects_zero_quantity(ReductionOperation operation) {
        OrderBook book;
        add_complete_book(book);
        const auto before = book.top_of_book();

        const auto result = (book.*operation)(kBidOrder, 0);

        assert_rejected_without_top_change(result, ApplyError::InvalidQuantity);
        assert(book.top_of_book() == before);
    }

    void test_reduction_rejects_unknown_order(ReductionOperation operation) {
        OrderBook book;
        add_complete_book(book);
        const auto before = book.top_of_book();

        const auto result = (book.*operation)(999, 1);

        assert_rejected_without_top_change(result, ApplyError::UnknownOrder);
        assert(book.top_of_book() == before);
    }

    void test_reduction_rejects_excessive_quantity_without_changing_book(
        ReductionOperation operation) {
        OrderBook book;
        add_complete_book(book);
        const auto before = book.top_of_book();

        const auto result = (book.*operation)(kBidOrder, kQuantity + 1);

        assert_rejected_without_top_change(result, ApplyError::QuantityExceeded);
        assert(book.top_of_book() == before);
    }

    void test_partial_bid_reduction_reduces_best_bid_quantity(ReductionOperation operation) {
        OrderBook book;
        add_complete_book(book);

        const auto result = (book.*operation)(kBidOrder, 4);

        assert(result.error == ApplyError::None);
        assert(result.top_of_book_changed);
        assert_top(book, kBestBid, 6, kBestAsk, kQuantity);
    }

    void test_partial_ask_reduction_reduces_best_ask_quantity(ReductionOperation operation) {
        OrderBook book;
        add_complete_book(book);

        const auto result = (book.*operation)(kAskOrder, 4);

        assert(result.error == ApplyError::None);
        assert(result.top_of_book_changed);
        assert_top(book, kBestBid, kQuantity, kBestAsk, 6);
    }

    void test_full_bid_reduction_reveals_next_bid(ReductionOperation operation) {
        OrderBook book;
        add_complete_book(book);
        book.add_order(3, Side::Buy, 6, 90);

        const auto result = (book.*operation)(kBidOrder, kQuantity);

        assert(result.error == ApplyError::None);
        assert(result.top_of_book_changed);
        assert_top(book, 90, 6, kBestAsk, kQuantity);
    }

    void test_full_bid_reduction_removes_order_reference(ReductionOperation operation) {
        OrderBook book;
        add_complete_book(book);

        const auto reduction = (book.*operation)(kBidOrder, kQuantity);
        assert(reduction.error == ApplyError::None);
        assert(reduction.top_of_book_changed);
        assert(!book.top_of_book().has_value());

        const auto result = (book.*operation)(kBidOrder, 1);

        assert_rejected_without_top_change(result, ApplyError::UnknownOrder);
    }

    void test_full_reduction_of_one_bid_order_preserves_others_at_same_price(
        ReductionOperation operation) {
        OrderBook book;
        add_complete_book(book);
        book.add_order(3, Side::Buy, 7, kBestBid);

        const auto result = (book.*operation)(kBidOrder, kQuantity);

        assert(result.error == ApplyError::None);
        assert(result.top_of_book_changed);
        assert_top(book, kBestBid, 7, kBestAsk, kQuantity);
    }

    void test_partial_reduction_at_non_best_bid_updates_hidden_quantity(
        ReductionOperation operation) {
        OrderBook book;
        add_complete_book(book);
        book.add_order(3, Side::Buy, 10, 90);

        const auto reduction = (book.*operation)(3, 4);
        assert(reduction.error == ApplyError::None);
        assert(!reduction.top_of_book_changed);
        assert_top(book, kBestBid, kQuantity, kBestAsk, kQuantity);

        book.delete_order(kBidOrder);
        assert_top(book, 90, 6, kBestAsk, kQuantity);
    }

    void test_delete_rejects_unknown_order() {
        OrderBook book;
        add_complete_book(book);
        const auto before = book.top_of_book();

        const auto result = book.delete_order(999);

        assert_rejected_without_top_change(result, ApplyError::UnknownOrder);
        assert(book.top_of_book() == before);
    }

    void test_delete_one_bid_order_preserves_others_at_same_price() {
        OrderBook book;
        add_complete_book(book);
        book.add_order(3, Side::Buy, 7, kBestBid);

        const auto result = book.delete_order(kBidOrder);

        assert(result.error == ApplyError::None);
        assert(result.top_of_book_changed);
        assert_top(book, kBestBid, 7, kBestAsk, kQuantity);
    }

    void test_delete_best_bid_reveals_next_bid() {
        OrderBook book;
        add_complete_book(book);
        book.add_order(3, Side::Buy, 6, 90);

        const auto result = book.delete_order(kBidOrder);

        assert(result.error == ApplyError::None);
        assert(result.top_of_book_changed);
        assert_top(book, 90, 6, kBestAsk, kQuantity);
    }

    void test_delete_non_best_bid_does_not_change_top() {
        OrderBook book;
        add_complete_book(book);
        book.add_order(3, Side::Buy, 6, 90);
        const auto before = book.top_of_book();

        const auto result = book.delete_order(3);

        assert(result.error == ApplyError::None);
        assert(!result.top_of_book_changed);
        assert(book.top_of_book() == before);
    }

    void test_delete_last_ask_clears_top() {
        OrderBook book;
        add_complete_book(book);

        const auto result = book.delete_order(kAskOrder);

        assert(result.error == ApplyError::None);
        assert(result.top_of_book_changed);
        assert(!book.top_of_book().has_value());
    }

    void test_replace_rejects_zero_quantity() {
        OrderBook book;
        add_complete_book(book);
        const auto before = book.top_of_book();

        const auto result = book.replace_order(kBidOrder, 3, 0, 110);

        assert_rejected_without_top_change(result, ApplyError::InvalidQuantity);
        assert(book.top_of_book() == before);
    }

    void test_replace_rejects_zero_price() {
        OrderBook book;
        add_complete_book(book);
        const auto before = book.top_of_book();

        const auto result = book.replace_order(kBidOrder, 3, 5, 0);

        assert_rejected_without_top_change(result, ApplyError::InvalidPrice);
        assert(book.top_of_book() == before);
    }

    void test_replace_rejects_unknown_old_reference() {
        OrderBook book;
        add_complete_book(book);
        const auto before = book.top_of_book();

        const auto result = book.replace_order(999, 3, 5, 110);

        assert_rejected_without_top_change(result, ApplyError::UnknownOrder);
        assert(book.top_of_book() == before);
    }

    void test_replace_rejects_duplicate_new_reference() {
        OrderBook book;
        add_complete_book(book);
        const auto before = book.top_of_book();

        const auto result = book.replace_order(kBidOrder, kAskOrder, 5, 110);

        assert_rejected_without_top_change(result, ApplyError::DuplicateOrder);
        assert(book.top_of_book() == before);
    }

    void test_replace_removes_old_and_creates_new_reference() {
        OrderBook book;
        add_complete_book(book);

        const auto result = book.replace_order(kBidOrder, 3, kQuantity, kBestBid);

        assert(result.error == ApplyError::None);
        assert(!result.top_of_book_changed);
        assert_top(book, kBestBid, kQuantity, kBestAsk, kQuantity);

        const auto old_result = book.execute_order(kBidOrder, 1);
        assert_rejected_without_top_change(old_result, ApplyError::UnknownOrder);

        const auto new_result = book.execute_order(3, 1);
        assert(new_result.error == ApplyError::None);
        assert(new_result.top_of_book_changed);
        assert_top(book, kBestBid, 9, kBestAsk, kQuantity);
    }

    void test_replace_sell_order_keeps_original_side_and_moves_price() {
        OrderBook book;
        add_complete_book(book);

        const auto result = book.replace_order(kAskOrder, 3, kQuantity, 190);

        assert(result.error == ApplyError::None);
        assert(result.top_of_book_changed);
        assert_top(book, kBestBid, kQuantity, 190, kQuantity);
    }

    void test_replace_buy_order_keeps_original_side_and_moves_price() {
        OrderBook book;
        add_complete_book(book);

        const auto result = book.replace_order(kBidOrder, 3, kQuantity, 110);

        assert(result.error == ApplyError::None);
        assert(result.top_of_book_changed);
        assert_top(book, 110, kQuantity, kBestAsk, kQuantity);
    }

    void test_replace_at_same_bid_price_updates_level_quantity() {
        OrderBook book;
        add_complete_book(book);

        const auto result = book.replace_order(kBidOrder, 3, 15, kBestBid);

        assert(result.error == ApplyError::None);
        assert(result.top_of_book_changed);
        assert_top(book, kBestBid, 15, kBestAsk, kQuantity);
    }

    void test_replace_best_bid_with_lower_price_reveals_next_bid() {
        OrderBook book;
        add_complete_book(book);
        book.add_order(3, Side::Buy, 6, 95);

        const auto result = book.replace_order(kBidOrder, 4, 7, 90);

        assert(result.error == ApplyError::None);
        assert(result.top_of_book_changed);
        assert_top(book, 95, 6, kBestAsk, kQuantity);
    }

    void test_replace_non_best_ask_with_lower_price_changes_top() {
        OrderBook book;
        add_complete_book(book);
        book.add_order(3, Side::Sell, 6, 210);

        const auto result = book.replace_order(3, 4, 7, 190);

        assert(result.error == ApplyError::None);
        assert(result.top_of_book_changed);
        assert_top(book, kBestBid, kQuantity, 190, 7);
    }

} // namespace

int main() {
    RUN_TEST(test_empty_book_has_no_top);
    RUN_TEST(test_bid_only_book_has_no_top);
    RUN_TEST(test_ask_only_book_has_no_top);
    RUN_TEST(test_add_ask_to_bid_only_book_creates_top);

    RUN_TEST(test_add_rejects_zero_quantity);
    RUN_TEST(test_add_rejects_zero_price);
    RUN_TEST(test_add_rejects_duplicate_reference_without_changing_book);
    RUN_TEST(test_add_second_order_at_best_bid_increases_bid_quantity);
    RUN_TEST(test_add_second_order_at_best_ask_increases_ask_quantity);
    RUN_TEST(test_add_better_bid_changes_top);
    RUN_TEST(test_add_lower_ask_changes_top);
    RUN_TEST(test_add_worse_bid_does_not_change_top);
    RUN_TEST(test_add_higher_ask_does_not_change_top);

    for (const auto& reduction : kReductionOperations) {
        RUN_REDUCTION_TEST(test_reduction_rejects_zero_quantity, reduction);
        RUN_REDUCTION_TEST(test_reduction_rejects_unknown_order, reduction);
        RUN_REDUCTION_TEST(
            test_reduction_rejects_excessive_quantity_without_changing_book, reduction);
        RUN_REDUCTION_TEST(test_partial_bid_reduction_reduces_best_bid_quantity, reduction);
        RUN_REDUCTION_TEST(test_partial_ask_reduction_reduces_best_ask_quantity, reduction);
        RUN_REDUCTION_TEST(test_full_bid_reduction_reveals_next_bid, reduction);
        RUN_REDUCTION_TEST(test_full_bid_reduction_removes_order_reference, reduction);
        RUN_REDUCTION_TEST(
            test_full_reduction_of_one_bid_order_preserves_others_at_same_price, reduction);
        RUN_REDUCTION_TEST(
            test_partial_reduction_at_non_best_bid_updates_hidden_quantity, reduction);
    }

    RUN_TEST(test_delete_rejects_unknown_order);
    RUN_TEST(test_delete_one_bid_order_preserves_others_at_same_price);
    RUN_TEST(test_delete_best_bid_reveals_next_bid);
    RUN_TEST(test_delete_non_best_bid_does_not_change_top);
    RUN_TEST(test_delete_last_ask_clears_top);

    RUN_TEST(test_replace_rejects_zero_quantity);
    RUN_TEST(test_replace_rejects_zero_price);
    RUN_TEST(test_replace_rejects_unknown_old_reference);
    RUN_TEST(test_replace_rejects_duplicate_new_reference);
    RUN_TEST(test_replace_removes_old_and_creates_new_reference);
    RUN_TEST(test_replace_sell_order_keeps_original_side_and_moves_price);
    RUN_TEST(test_replace_buy_order_keeps_original_side_and_moves_price);
    RUN_TEST(test_replace_at_same_bid_price_updates_level_quantity);
    RUN_TEST(test_replace_best_bid_with_lower_price_reveals_next_bid);
    RUN_TEST(test_replace_non_best_ask_with_lower_price_changes_top);

    return 0;
}
