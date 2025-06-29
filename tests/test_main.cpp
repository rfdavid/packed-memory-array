#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include <list>
#include <random>

#include "pma.hpp"

TEST_CASE("Insert sequential elements", "[pma]") {
    pma::PackedMemoryArray<int, int> pma(8);
    for (int i = 1; i <= 30; i++) {
        pma.insertElement(i, i*10);
    }

    REQUIRE(pma.isSorted() == true);
    REQUIRE(pma.getSize() == 64);
}

TEST_CASE("Inverse insertion", "[pma]") {
    pma::PackedMemoryArray<char, int> pma(64);
    for (int i = 100; i >= 0; i--) {
        pma.insertElement(static_cast<char>(i), i*10);
    }

    REQUIRE(pma.isSorted() == true);
    REQUIRE(pma.getSize() == 256);
}

TEST_CASE("Insert 10k elements", "[pma]") {
    pma::PackedMemoryArray<int, int> pma(64);
    for (int i = 10000; i > 0; i--) {
        pma.insertElement(i, i*10000);
    }

    REQUIRE(pma.isSorted() == true);
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
        pma.insertElement(distr(eng), i*1000000);
    }

    REQUIRE(pma.isSorted() == true);
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
}