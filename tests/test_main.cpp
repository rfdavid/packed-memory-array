#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include "pma_key_value.hpp"
#include <list>

TEST_CASE("Insert sequential elements", "[pma]") {
    pma::PackedMemoryArray pma(8);
    for (int i = 1; i <= 30; i++) {
        pma.insertElement(i, i*10);
    }

    REQUIRE(pma.isSorted() == true);
    REQUIRE(pma.size() == 64);
}

TEST_CASE("Inverse insertion", "[pma]") {
    pma::PackedMemoryArray pma(64);
    for (int i = 100; i >= 0; i--) {
        pma.insertElement(i, i*10);
    }

    REQUIRE(pma.isSorted() == true);
    REQUIRE(pma.size() == 256);
}

TEST_CASE("Insert 10k elements", "[pma]") {
    pma::PackedMemoryArray pma(64);
    for (int i = 10000; i > 0; i--) {
        pma.insertElement(i, i*10000);
    }

    REQUIRE(pma.isSorted() == true);
    REQUIRE(pma.size() == 16384);
    REQUIRE(pma.segmentSize == 16);
    REQUIRE(pma.noOfSegments() == 1024);
    REQUIRE(pma.getTreeHeight() == 11);
    REQUIRE(pma.totalElements == 10000);
}


TEST_CASE("Random insert", "[pma]") {
    pma::PackedMemoryArray pma(8);
    std::list<int> keys = {5, 10, 6, 17, 1, 21, 9, 12, 8, 16, 20, 13, 7, 3, 15, 19, 14, 11, 22, 18, 4, 2};

    for (auto key : keys) {
        pma.insertElement(key, key*10);
    }

    REQUIRE(pma.isSorted() == true);
}
