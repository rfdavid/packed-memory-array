#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include "pma_index.hpp"
#include <list>
#include <random>


// insert sequential elements
void insertSequentialElements(int total) {
    pma::PackedMemoryArray pma(8);
    for (int i = 0; i < total; i++) {
        pma.insertElement(i, i*10000);
    }
}

void insertRandomElements(int total) {
    pma::PackedMemoryArray pma(8);
    std::random_device rd;
    std::mt19937 eng(100);
    std::uniform_int_distribution<> distr(1, 100000);

    for (int i = 0; i < total; i++) {
        pma.insertElement(distr(eng), i*1000000);
    }
}

TEST_CASE("Insertions") {
    BENCHMARK("Insert 1m sequential elements") {
        return insertSequentialElements(10000000);
    };

//    BENCHMARK("Insert 100k random elements") {
//        return insertRandomElements(100000);
//    };

}
