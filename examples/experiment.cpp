#include <random>
#include <chrono>
#include <map>
#include <iostream>

#include "pma.hpp"

void benchmarkPMA_random() {
    pma::PackedMemoryArray<int, int> pma(128);
    std::vector<int64_t> keys(10000000);
    
    std::mt19937_64 rng(42); // fixed seed for reproducibility
    std::uniform_int_distribution<int64_t> dist(0, 1LL << 40);

    for (auto& k : keys) k = dist(rng);
    
    auto start = std::chrono::high_resolution_clock::now();

    for (auto k : keys) {
        pma.insertElement(k, k * 10);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "PMA random: " << duration.count() << " us" << std::endl;
}

void benchmarkMAP_random() {
    std::map<int, int> map;
    std::vector<int> keys(10000000);

    std::mt19937_64 rng(42); // fixed seed for reproducibility
    std::uniform_int_distribution<int64_t> dist(0, 1LL << 40);

    for (auto& k : keys) k = dist(rng);

    auto start = std::chrono::high_resolution_clock::now();

    for (auto k : keys) {
        map.insert({k, k * 10});
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "std::map random: " << duration.count() << " us" << std::endl;
}

void benchmarkPMA() {
    uint64_t mid = 0;
    pma::PackedMemoryArray<int, int> pma(64 /* initial capacity */);

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 10000000; i++) {
        pma.insertElement(i, i*10);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "Time taken by function: "
        << duration.count() << " microseconds" << std::endl;
}

void benchmarkMAP() {
    std::map<int, int> map;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 10000000; i++) {
        map.insert({i, i*10});
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "Time taken by function: "
        << duration.count() << " microseconds" << std::endl;
}

int main() {
    benchmarkMAP_random();
//    benchmarkPMA_random();
    return 0;
}
