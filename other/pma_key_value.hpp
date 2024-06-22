#pragma once

#include <vector>
#include <iostream>
#include <optional>
#include <cassert>

#ifdef DEBUG
#define DEBUG_PRINT std::cout
#define HIGHLIGHT_START std::cout << "\033[1;31m"
#define HIGHLIGHT_END std::cout << "\033[0m"
#else
#define DEBUG_PRINT if (false) std::cout
#define HIGHLIGHT_START if (false) std::cout
#define HIGHLIGHT_END if (false) std::cout
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
        data = std::vector<std::optional<std::pair<int64_t, int64_t>>>(capacity, std::nullopt);
        // from rma, 2^ceil(log2(log2(n)))
        // 2^ ceil(log2(log2(64))) = 2^ceil(log2(6)) = 2^ceil(3) = 8
        segmentSize = capacity;
    }

    void insertElement(int64_t key, int64_t value);
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
    uint64_t noOfSegments() const {
        return capacity / segmentSize;
    }
    int getTreeHeight();

public:
    uint64_t segmentSize;
    uint64_t capacity;
    uint64_t totalElements = 0;
    bool binarySearchPMA(uint64_t key, uint64_t *index);

private:
    double upperThresholdAtLevel(int level);
    double lowerThresholdAtLevel(int level);
    void rebalance(uint64_t left, uint64_t right);
    void insertElement(int key, int value, uint64_t index);
    void getSegmentOffset(int level, int index, uint64_t *start, uint64_t *end);
    double getDensity(uint64_t left, uint64_t right);
    void doubleCapacity();
    bool checkIfFullAndRebalance();
    void checkForRebalancing(int index);
    uint64_t findFirstGapFrom(uint64_t startingIndex);
    void deleteElement(int key);

private:
    std::vector<std::optional<std::pair<int64_t, int64_t>>> data;

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
