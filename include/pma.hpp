/**
 * @file pma.hpp
 * @brief Packed Memory Array (PMA) implementation
 * 
 * This file implements a Packed Memory Array, which is a data structure that maintains
 * a sorted array with efficient insertions and range queries.
 */

#include <cinttypes>
#include <cmath>
#include <iostream>
#include <memory> // unique_ptr
#include <vector>

namespace pma {

/**
 * @brief Core storage structure for the Packed Memory Array
 */
struct PMA {
    // Core data arrays
    int64_t* keys;              ///< Array of keys
    int64_t* values;            ///< Array of values
    int16_t* segmentElementsCount; ///< Count of elements in each segment
    
    // Configuration
    size_t capacity;            ///< Total capacity of the array
    size_t segmentCapacity;     ///< Capacity of each segment
    size_t height;              ///< Height of the PMA tree
    size_t numElements;         ///< Total number of elements stored

    // Constructors and destructors
    PMA(size_t segmentCapacity);
    ~PMA();

    // Memory management
    static void allocSegments(size_t numOfSegments, size_t segmentCapacity, 
            int64_t** keys, int64_t** values, decltype(segmentElementsCount) * segmentElementsCount);
};

/**
 * @brief Main Packed Memory Array class
 * 
 * Implements a Packed Memory Array with efficient insertions and range queries.
 * The structure maintains a sorted array with O(log n) insertions and O(log n) range queries.
 */
class PackedMemoryArray {
public:
    // Constants
    static constexpr double ph = 0.50;  ///< Lower density threshold
    static constexpr double th = 0.75;  ///< Upper density threshold

    // Constructors and destructors
    PackedMemoryArray(uint64_t segmentCapacity);
    ~PackedMemoryArray();

    // Core operations
    void insertElement(int64_t key, int64_t value);
    bool isSorted();

    // Getters
    int getTreeHeight() const { return storage.height; }
    uint64_t totalElements() const { return storage.numElements; }
    uint64_t segmentSize() const { return storage.segmentCapacity; }
    uint64_t size() const { return storage.capacity; }
    uint64_t noOfSegments() const { return storage.capacity / storage.segmentCapacity; }
    bool elemExistsAt(int64_t index) const { return storage.keys[index] > 0; }
    uint64_t getSegmentCount(uint64_t segmentId) const;
    int64_t getMinimum(uint64_t segmentId) const;
    uint64_t indexFindLeq(int64_t key) const;

    // Debug operations
    void dump();
    void dumpValues();
    void dumpIndex();

private:
    // Internal operations
    void initializeIndex(size_t btreeParams, size_t storageParams);
    void insertEmpty(int64_t key, int64_t value);
    int64_t insertCommon(uint64_t segmentId, int64_t key, int64_t value);
    int rebalance(uint64_t segmentId, int64_t key, int64_t value);
    int spread(size_t numElements, size_t windowStart, size_t windowLength, int64_t key);
    int resize(int64_t key);
    void getThresholds(size_t height, double& upper, double& lower) const;

    // Member variables
    PMA storage;                ///< Core storage structure
    std::vector<int64_t> indexVec; ///< Index vector for efficient lookups
};

} // namespace pma
