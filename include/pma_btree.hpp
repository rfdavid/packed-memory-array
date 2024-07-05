#include <cinttypes>
#include <cmath>
#include <iostream>
#include <memory> // unique_ptr
#include <vector>

namespace pma {

struct SumResult {
    int64_t m_first_key = 0; // the first key that qualifies inside the interval [min, max]. Undefined if m_num_elements == 0
    int64_t m_last_key = 0; // the last key that qualifies inside the interval [min, max]. Undefined if m_num_elements == 0
    uint64_t m_num_elements = 0; // the total number of elements inside the interval [min, max]
    int64_t m_sum_keys = 0; // the aggregate sum of all keys in the interval [min, max]
    int64_t m_sum_values = 0; // the aggregate sum of all values in the interval [min, max]
};


struct PMA {
    int64_t* keys;
    int64_t* values;
    int16_t* segmentElementsCount;
    size_t capacity;
    size_t segmentCapacity;
    size_t height;
    size_t numElements;

    ~PMA();

    PMA(size_t segmentCapacity);
    static void allocSegments(size_t numOfSegments, size_t segmentCapacity, 
            int64_t** keys, int64_t** values, decltype(segmentElementsCount) * segmentElementsCount);
};

class PackedMemoryArray {
    public:
    PackedMemoryArray(uint64_t segmentCapacity);

    // pointer for the index
    PMA storage;

    std::vector<int64_t> indexVec;

    SumResult sum(int64_t min, int64_t max);

    void initializeIndex(size_t btreeParams, size_t storageParams);
    void insertElement(int64_t key, int64_t value);
    void insertEmpty(int64_t key, int64_t value);
    int64_t insertCommon(uint64_t segmentId, int64_t key, int64_t value);
    int rebalance(uint64_t segmentId, int64_t key, int64_t value);
    int spread(size_t numElements, size_t windowStart, size_t windowLength, int64_t key);
    int resize(int64_t key);
    bool isSorted();

    uint64_t getSegmentCount(uint64_t segmentId) const;
    int64_t getMinimum(uint64_t segmentId) const;
    uint64_t indexFindLeq(int64_t key) const;

    void getThresholds(size_t height, double& upper, double& lower) const;


    int getTreeHeight () const {
        return storage.height;
    }

    uint64_t totalElements() const {
        return storage.numElements;
    }

    uint64_t segmentSize() const {
        return storage.segmentCapacity;
    }

    uint64_t size() const {
        return storage.capacity;
    }

    uint64_t noOfSegments() const {
        return storage.capacity / storage.segmentCapacity;
    }

    bool elemExistsAt(int64_t index) const {
        return storage.keys[index] > 0;
    }

    void dump();
    void dumpValues();

    void dumpIndex();

    static constexpr double ph = 0.50;

    static constexpr double th = 0.75;

    ~PackedMemoryArray();
};

} // namespace pma
