#include <catch2/catch_test_macros.hpp>
#include <vector>

#include "array_sequence.hpp"
#include "lazy_sequence.hpp"

TEST_CASE("From array") {
    int data[] = {1, 2, 3, 4, 5};
    auto seq = std::make_shared<LazySequence<int>>(data, 5);

    REQUIRE(seq->GetLength().IsFinite());
    REQUIRE(seq->GetLength().GetFinite() == 5);
    REQUIRE(seq->GetMaterializedCount() == 5);  // already materialized

    REQUIRE(seq->GetFirst() == 1);
    REQUIRE(seq->GetLast() == 5);
    REQUIRE(seq->GetIndex(2) == 3);
}

TEST_CASE("Generator") {
    auto start = std::make_shared<ArraySequence<long long>>();
    start->Append(1);
    start->Append(1);

    auto fib = std::make_shared<LazySequence<long long>>(
        [](SequencePtr<long long> last2) {
            return last2->Get(0) + last2->Get(1);
        },
        start, 2);

    REQUIRE(fib->GetLength().IsN0());
    REQUIRE(fib->GetMaterializedCount() == 2);

    // Access forces memoization.
    REQUIRE(fib->GetIndex(0) == 1);
    REQUIRE(fib->GetIndex(1) == 1);
    REQUIRE(fib->GetIndex(2) == 2);
    REQUIRE(fib->GetIndex(3) == 3);
    REQUIRE(fib->GetIndex(4) == 5);
    REQUIRE(fib->GetIndex(9) == 55);

    REQUIRE(fib->GetMaterializedCount() >= 10);
}

TEST_CASE("GetSubsequence") {
    auto base = std::make_shared<ArraySequence<int>>();
    for (int i = 0; i < 10; ++i) {
        base->Append(i);
    }
    auto seq = std::make_shared<LazySequence<int>>(base);

    auto sub = seq->GetSubsequence(3, 7);
    REQUIRE(sub->GetLength().IsFinite());
    REQUIRE(sub->GetLength().GetFinite() == 5);
    REQUIRE(sub->GetIndex(0) == 3);
    REQUIRE(sub->GetIndex(4) == 7);
}

TEST_CASE("Append/Prepend/InsertAt") {
    int data[] = {10, 20, 30};
    auto seq = std::make_shared<LazySequence<int>>(data, 3);

    auto appended = seq->Append(40);
    auto prepended = seq->Prepend(5);
    auto inserted = seq->InsertAt(15, 1);

    // Original
    REQUIRE(seq->GetLength().GetFinite() == 3);
    REQUIRE(seq->GetIndex(0) == 10);
    REQUIRE(seq->GetIndex(2) == 30);

    // Append
    REQUIRE(appended->GetLength().GetFinite() == 4);
    REQUIRE(appended->GetIndex(3) == 40);

    // Prepend
    REQUIRE(prepended->GetLength().GetFinite() == 4);
    REQUIRE(prepended->GetIndex(0) == 5);
    REQUIRE(prepended->GetIndex(1) == 10);

    // Insert
    REQUIRE(inserted->GetLength().GetFinite() == 4);
    REQUIRE(inserted->GetIndex(0) == 10);
    REQUIRE(inserted->GetIndex(1) == 15);
    REQUIRE(inserted->GetIndex(2) == 20);
}

TEST_CASE("Concat + Map + Reduce") {
    int a[] = {1, 2, 3};
    int b[] = {4, 5};
    auto s1 = std::make_shared<LazySequence<int>>(a, 3);
    auto s2 = std::make_shared<LazySequence<int>>(b, 2);

    auto c = s1->Concat(s2);
    REQUIRE(c->GetLength().IsFinite());
    REQUIRE(c->GetLength().GetFinite() == 5);
    REQUIRE(c->GetIndex(0) == 1);
    REQUIRE(c->GetIndex(4) == 5);

    auto sq = c->Map([](int x) {
        return x * x;
    });
    REQUIRE(sq->GetIndex(0) == 1);
    REQUIRE(sq->GetIndex(4) == 25);

    const int sum = c->Reduce(0, [](int acc, int x) {
        return acc + x;
    });
    REQUIRE(sum == 15);
}

TEST_CASE("Where") {
    int data[] = {1, 2, 3, 4, 5, 6};
    auto seq = std::make_shared<LazySequence<int>>(data, 6);
    auto evens = seq->Where([](int x) {
        return x % 2 == 0;
    });

    std::vector<int> got;
    for (auto it = evens->GetConstEnumerator(); !it->IsEnd(); it->MoveNext()) {
        got.push_back(it->ConstDereference());
    }

    REQUIRE(got == std::vector<int>({2, 4, 6}));
}

TEST_CASE("Zip") {
    int a[] = {1, 2, 3};
    int b[] = {10, 20};
    auto s1 = std::make_shared<LazySequence<int>>(a, 3);
    auto s2 = std::make_shared<LazySequence<int>>(b, 2);

    auto zipped = s1->Zip(s2);

    REQUIRE(zipped->GetLength().IsFinite());
    REQUIRE(zipped->GetLength().GetFinite() == 2);
    REQUIRE(zipped->GetIndex(0) == std::pair{1, 10});
    REQUIRE(zipped->GetIndex(1) == std::pair{2, 20});
}
