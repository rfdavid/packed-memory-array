#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include <list>
#include <random>

#include "pma_btree.hpp"

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
    REQUIRE(pma.segmentSize() == 16);
    REQUIRE(pma.noOfSegments() == 1024);
    REQUIRE(pma.getTreeHeight() == 11);
    REQUIRE(pma.totalElements() == 10000);
}

TEST_CASE("Insert 100k random big numbers", "[pma]") {
    pma::PackedMemoryArray pma(8);
    std::random_device rd;
    std::mt19937 eng(rd());
    std::uniform_int_distribution<> distr(1, 100000);

    for (int i = 0; i < 100000; i++) {
        pma.insertElement(distr(eng), i*1000000);
    }

    REQUIRE(pma.isSorted() == true);
    REQUIRE(pma.size() == 131072);
    REQUIRE(pma.segmentSize() == 32);
    REQUIRE(pma.noOfSegments() == 4096);
    REQUIRE(pma.getTreeHeight() == 13);
    // elements are random and not unique
    REQUIRE(pma.totalElements() > 63000);
}

TEST_CASE("Random insert", "[pma]") {
    pma::PackedMemoryArray pma(8);
    std::list<int> keys = {5, 10, 6, 17, 1, 21, 9, 12, 8, 16, 20, 13, 7, 3, 15, 19, 14, 11, 22, 18, 4, 2};

    for (auto key : keys) {
        pma.insertElement(key, key*10);
    }

    REQUIRE(pma.isSorted() == true);
}

// TEST_CASE("Sum elements", "[pma]") {
//     SKIP();
// 
//     pma::PackedMemoryArray pma(8);
// 
//     for (int i = 1; i <= 30; i++) {
//         pma.insertElement(i, i*10);
//     }
// 
//     auto sum = pma.sum(5, 15);
//     REQUIRE(sum.m_first_key == 5);
//     REQUIRE(sum.m_last_key == 15);
//     REQUIRE(sum.m_num_elements == 11);
//     REQUIRE(sum.m_sum_keys == 110);
//     REQUIRE(sum.m_sum_values == 1100);
// }
// 
// TEST_CASE("Sum 10k elements", "[pma]") {
//     SKIP();
//     pma::PackedMemoryArray pma(64);
//     for (int i = 10000; i > 0; i--) {
//         pma.insertElement(i, i*10000);
//     }
// 
//     auto sum = pma.sum(5000, 10000);
//     REQUIRE(sum.m_first_key == 5000);
//     REQUIRE(sum.m_last_key == 10000);
//     REQUIRE(sum.m_num_elements == 5001);
//     REQUIRE(sum.m_sum_keys == 37507500);
//     REQUIRE(sum.m_sum_values == 375075000000);
// }
