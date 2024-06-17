#pragma once

#include <vector>
#include <iostream>
#include <optional>
#include <cassert>

#ifdef DEBUG
#define DEBUG_PRINT std::cout
#else
#define DEBUG_PRINT if (false) std::cout
#endif

namespace pma {

    // from rma for benchmarking
    struct SumResult {
        int64_t m_first_key = 0; // the first key that qualifies inside the interval [min, max]. Undefined if m_num_elements == 0
        int64_t m_last_key = 0; // the last key that qualifies inside the interval [min, max]. Undefined if m_num_elements == 0
        uint64_t m_num_elements = 0; // the total number of elements inside the interval [min, max]
        int64_t m_sum_keys = 0; // the aggregate sum of all keys in the interval [min, max]
        int64_t m_sum_values = 0; // the aggregate sum of all values in the interval [min, max]
    };

class PackedMemoryArray {
public:
    PackedMemoryArray(uint64_t capacity) : capacity(capacity) {
        // from rma, 2^ceil(log2(log2(n)))
        // 2^ ceil(log2(log2(64))) = 2^ceil(log2(6)) = 2^ceil(3) = 8
        segmentSize = std::pow(2, std::ceil(log2(static_cast<double>(log2(capacity)))));
        data = std::vector<std::optional<std::pair<int, int>>>(capacity, std::nullopt);
    }

    void insertElement(int key, int value);
    // sum elements from a specific range
    SumResult sum(uint64_t min, uint64_t max);
    void print(int segmentSize = 0, bool printIndex = false);
    void printStats();
    bool isSorted();

    bool elemExistsAt(int index) const {
        return data[index] != std::nullopt;
    }
    int64_t elemAt(int index) const {
        return data[index]->first;
    }
    uint64_t size() const {
        return capacity;
    }

public:
    uint64_t segmentSize;

private:
    double upperThresholdAtLevel(int level);
    double lowerThresholdAtLevel(int level);
    void rebalance(uint64_t left, uint64_t right);
    void insertElement(int key, int value, uint64_t index);
    uint64_t binarySearchPMA(uint64_t key);
    int getTreeHeight();
    void getSegmentOffset(int level, int index, uint64_t *start, uint64_t *end);
    double getDensity(uint64_t left, uint64_t right);
    void doubleCapacity();
    bool checkIfFullAndRebalance();
    void checkForRebalancing(int index);
    uint64_t findFirstGapFrom(uint64_t startingIndex);
    void deleteElement(int key);

private:
    std::vector<std::optional<std::pair<int, int>>> data;
    uint64_t capacity;
    uint64_t totalElements = 0;

    // lower threshold at level 1
    static constexpr double p1 = 0.1;
    // upper threshold at level 1
    static constexpr double t1 = 1;
    // lower threshold at top level h
    static constexpr double ph = 0.30;
    // upper threshold at top level h
    static constexpr double th = 0.75;

};

} // namespace pma
