#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include <list>
#include <random>
#include <stdexcept>

#include "pma.hpp"

TEST_CASE("Insert sequential elements", "[pma]") {
    pma::PackedMemoryArray<int, int> pma(8);
    for (int i = 1; i <= 30; i++) {
        pma.insertElement(i, i*10);
    }

    REQUIRE(pma.isSorted() == true);
    REQUIRE(pma.checkInvariants() == true);
    REQUIRE(pma.getSize() == 64);
}

TEST_CASE("Inverse insertion", "[pma]") {
    pma::PackedMemoryArray<char, int> pma(64);
    for (int i = 100; i >= 0; i--) {
        pma.insertElement(static_cast<char>(i), i*10);
    }

    REQUIRE(pma.isSorted() == true);
    REQUIRE(pma.checkInvariants() == true);
    REQUIRE(pma.getSize() == 256);
}

TEST_CASE("Insert 10k elements", "[pma]") {
    pma::PackedMemoryArray<int, int> pma(64);
    for (int i = 10000; i > 0; i--) {
        pma.insertElement(i, i*10000);
    }

    REQUIRE(pma.isSorted() == true);
    REQUIRE(pma.checkInvariants() == true);
    REQUIRE(pma.getSize() == 16384);
    REQUIRE(pma.getSegmentSize() == 64);
    REQUIRE(pma.getNoOfSegments() == 256);
    REQUIRE(pma.getTreeHeight() == 9);
    REQUIRE(pma.getTotalElements() == 10000);
}

TEST_CASE("Insert 100k random big numbers", "[pma]") {
    pma::PackedMemoryArray<int, int> pma(64);
    std::random_device rd;
    std::mt19937 eng(rd());
    std::uniform_int_distribution<> distr(1, 100000);

    for (int i = 0; i < 100000; i++) {
        pma.insertElement(distr(eng), i);
    }

    REQUIRE(pma.isSorted() == true);
    REQUIRE(pma.checkInvariants() == true);
    REQUIRE(pma.getSize() == 131072);
    REQUIRE(pma.getSegmentSize() == 64);
    REQUIRE(pma.getNoOfSegments() == 2048);
    REQUIRE(pma.getTreeHeight() == 12);
    REQUIRE(pma.getTotalElements() > 63000);
}

TEST_CASE("Random insert", "[pma]") {
    pma::PackedMemoryArray<int, int> pma(8);
    std::list<int> keys = {5, 10, 6, 17, 1, 21, 9, 12, 8, 16, 20, 13, 7, 3, 15, 19, 14, 11, 22, 18, 4, 2};

    for (auto key : keys) {
        pma.insertElement(key, key*10);
    }

    REQUIRE(pma.isSorted() == true);
    REQUIRE(pma.checkInvariants() == true);
}

// Inserts n keys from nextKey, checking the PMA invariants along the way
template <typename KeyGen>
static void insertAndCheck(size_t segmentSize, int n, KeyGen nextKey) {
    pma::PackedMemoryArray<int, int> pma(segmentSize);
    for (int i = 0; i < n; i++) {
        pma.insertElement(nextKey(i), i);
        if (i % 500 == 0) {
            REQUIRE(pma.checkInvariants() == true);
        }
    }

    REQUIRE(pma.isSorted() == true);
    REQUIRE(pma.checkInvariants() == true);
    REQUIRE(pma.getTotalElements() == n);
}

TEST_CASE("Descending insert with small segments", "[pma]") {
    for (size_t segmentSize : {4, 8, 16}) {
        INFO("segment size " << segmentSize);
        insertAndCheck(segmentSize, 20000, [](int i) { return 20000 - i; });
    }
}

TEST_CASE("Random insert with small segments", "[pma]") {
    for (size_t segmentSize : {4, 8, 16}) {
        INFO("segment size " << segmentSize);
        std::mt19937 eng(42);
        insertAndCheck(segmentSize, 20000, [&](int) { return (int) (eng() % 1000000000); });
    }
}

TEST_CASE("Duplicate keys with small segments", "[pma]") {
    for (size_t segmentSize : {4, 8, 16}) {
        INFO("segment size " << segmentSize);
        std::mt19937 eng(42);
        insertAndCheck(segmentSize, 20000, [&](int) { return (int) (eng() % 50); });
    }
}

TEST_CASE("Smallest segment size", "[pma]") {
    std::mt19937 eng(42);
    insertAndCheck(2, 2000, [](int i) { return 2000 - i; });
    insertAndCheck(2, 2000, [&](int) { return (int) (eng() % 1000000000); });
    insertAndCheck(2, 2000, [&](int) { return (int) (eng() % 50); });
}

TEST_CASE("Invalid segment sizes are rejected", "[pma]") {
    using PMA = pma::PackedMemoryArray<int, int>;
    REQUIRE_THROWS_AS(PMA(0), std::invalid_argument);
    REQUIRE_THROWS_AS(PMA(1), std::invalid_argument);
    REQUIRE_THROWS_AS(PMA(32768), std::invalid_argument);
    REQUIRE_NOTHROW(PMA(2));
    REQUIRE_NOTHROW(PMA(32767));
}
