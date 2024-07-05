#include <cinttypes>
#include <cmath>
#include <iostream>
#include <memory> // unique_ptr
#include <vector>


namespace pma {

size_t hyperceil(size_t value);

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

    void initializeIndex(size_t btreeParams, size_t storageParams);
    void insertElement(int64_t key, int64_t value);
    void insertEmpty(int64_t key, int64_t value);
    int64_t insertCommon(uint64_t segmentId, int64_t key, int64_t value);
    bool rebalance(uint64_t segmentId, int64_t key, int64_t value);
    void spread(size_t numElements, size_t windowStart, size_t windowLength, int64_t key, int64_t value);
    void resize();
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

    void dump();

    void dumpIndex();

    /* index */
    void updateIndex(int64_t oldKey, int64_t newKey);

    static constexpr double ph = 0.50;

    static constexpr double th = 0.75;

    ~PackedMemoryArray();
};

} // namespace pma
