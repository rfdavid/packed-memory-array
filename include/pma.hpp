/**
 * @file pma.hpp
 * @brief Header-only implementation of the Packed Memory Array (PMA)
 */

#ifndef PMA_HPP
#define PMA_HPP

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>
#include <cmath>
#include <type_traits>
#include <optional>

namespace pma {

/**
 * @brief Base storage structure for the Packed Memory Array
 */
template <typename KeyType, typename ValueType>
class PMA {
protected:
    size_t capacity;
    size_t height;
    size_t numElements;
    size_t segmentCapacity;
    KeyType* keys;
    ValueType* values;
    bool* occupied;
    int16_t* segmentElementsCount;

    void allocSegments(size_t numOfSegments, size_t segmentCapacity, 
            KeyType** keys, ValueType** values, bool** occupied, int16_t** segmentElementsCount) {
        *keys = nullptr;
        *values = nullptr;
        *occupied = nullptr;
        *segmentElementsCount = nullptr;

        // Allocate aligned memory for each array
        posix_memalign((void**) keys, 64, numOfSegments * segmentCapacity * sizeof(KeyType));
        posix_memalign((void**) values, 64, numOfSegments * segmentCapacity * sizeof(ValueType));
        posix_memalign((void**) occupied, 64, numOfSegments * segmentCapacity * sizeof(bool));
        posix_memalign((void**) segmentElementsCount, 64, numOfSegments * sizeof(int16_t));

        // Initialize arrays
        memset(*occupied, 0, numOfSegments * segmentCapacity * sizeof(bool));
        memset(*segmentElementsCount, 0, numOfSegments * sizeof(int16_t));
    }

    PMA(size_t segmentSize) : segmentCapacity(segmentSize) {
        capacity = segmentCapacity;
        height = 1;
        numElements = 0;
        allocSegments(1, segmentSize, &keys, &values, &occupied, &segmentElementsCount);
    }

    ~PMA() {
        free(keys);
        free(values);
        free(occupied);
        free(segmentElementsCount);
        keys = nullptr;
        values = nullptr;
        occupied = nullptr;
        segmentElementsCount = nullptr;
    }

public:
    size_t getSize() const {
        return capacity;
    }

    size_t getNoOfSegments() const {
        return capacity / segmentCapacity;
    }

    size_t getTotalElements() const {
        return numElements;
    }

    size_t getSegmentSize() const {
        return segmentCapacity;
    }

    size_t getTreeHeight() const {
        return height;
    }
};

/**
 * @brief Packed Memory Array implementation
 */
template <typename KeyType, typename ValueType>
class PackedMemoryArray : public PMA<KeyType, ValueType> {
private:
    std::vector<KeyType> indexVec;
    static constexpr double ph = 0.25;  // Lower density threshold
    static constexpr double th = 0.75;  // Upper density threshold

    inline void getThresholds(size_t height, double& lower, double& upper) const {
        double diff = (((double) this->height) - height) / this->height;
        lower = ph - 0.25 * diff;
        upper = th + 0.25 * diff;
    }

    inline KeyType getMinimum(uint64_t segmentId) const {
        auto segmentStart = segmentId * this->segmentCapacity;
        while (!this->occupied[segmentStart]) segmentStart++;
        return this->keys[segmentStart];
    }

    inline uint64_t getSegmentCount(uint64_t segmentId) const {
        return this->segmentElementsCount[segmentId];
    }

    inline uint64_t indexFindLeq(KeyType key) const {
        int64_t left = 0;
        int64_t right = indexVec.size() - 1;
        int64_t mid = 0;
        int64_t ans = -1;

        while (left <= right) {
            mid = left + (right - left) / 2;
            if (indexVec[mid] <= key) {
                ans = mid;
                left = mid + 1;
            } else {
                right = mid - 1;
            }
        }

        return (ans == -1) ? 0 : ans;
    }

    inline void insertEmpty(KeyType key, ValueType value) {
        this->keys[0] = key;
        this->values[0] = value;
        this->occupied[0] = true;
        this->segmentElementsCount[0] = 1;
        this->numElements = 1;
    }

    inline bool insertCommon(uint64_t segmentId, KeyType key, ValueType value) {
        // Get pointers to segment
        KeyType* __restrict keys = this->keys + segmentId * this->segmentCapacity;
        ValueType* __restrict values = this->values + segmentId * this->segmentCapacity;
        bool* __restrict occupied = this->occupied + segmentId * this->segmentCapacity;

        // Find insertion position and gap
        int lastGap = -1;
        int insertPos = -1;

        // Scan for gap and insertion position
        for (int i = 0; i < this->segmentCapacity; i++) {
            if (!occupied[i]) {
                lastGap = i;
                if (insertPos != -1) break;
            } else {
                if (insertPos == -1) {
                    if (keys[i] >= key) {
                        insertPos = i;
                        if (lastGap != -1) break;
                    }
                }
            }
        }
        
        // Handle insertion at end of segment
        if (insertPos == -1) {
            if (lastGap != this->segmentCapacity - 1) {
                size_t numElements = this->segmentCapacity - lastGap - 1;
                std::memmove(&keys[lastGap], &keys[lastGap + 1], numElements * sizeof(keys[0]));
                std::memmove(&values[lastGap], &values[lastGap + 1], numElements * sizeof(values[0]));
                std::memmove(&occupied[lastGap], &occupied[lastGap + 1], numElements * sizeof(occupied[0]));
            }
            insertPos = this->segmentCapacity - 1;
        } else if (lastGap < insertPos) {
            // Shift elements right
            size_t numElements = insertPos - lastGap;
            std::memmove(&keys[lastGap], &keys[lastGap + 1], numElements * sizeof(keys[0]));
            std::memmove(&values[lastGap], &values[lastGap + 1], numElements * sizeof(values[0]));
            std::memmove(&occupied[lastGap], &occupied[lastGap + 1], numElements * sizeof(occupied[0]));
            insertPos--;
        } else if (lastGap > insertPos) {
            // Shift elements left
            size_t numElements = lastGap - insertPos;
            std::memmove(&keys[insertPos + 1], &keys[insertPos], numElements * sizeof(keys[0]));
            std::memmove(&values[insertPos + 1], &values[insertPos], numElements * sizeof(values[0]));
            std::memmove(&occupied[insertPos + 1], &occupied[insertPos], numElements * sizeof(occupied[0]));
        }

        // Insert new element
        keys[insertPos] = key;
        values[insertPos] = value;
        occupied[insertPos] = true;

        // Update counts
        this->segmentElementsCount[segmentId]++;
        this->numElements++;

        // Return if new element is segment minimum
        return (insertPos == 0);
    }

    int rebalance(uint64_t segmentId, KeyType key, ValueType value);
    int spread(size_t numElements, size_t windowStart, size_t numOfSegments, KeyType key);
    int resize(KeyType key);

public:
    PackedMemoryArray(uint64_t segmentCapacity) : PMA<KeyType, ValueType>(segmentCapacity) {
        static_assert(std::is_trivially_copyable<KeyType>::value, "KeyType must be trivially copyable");
        static_assert(std::is_trivially_copyable<ValueType>::value, "ValueType must be trivially copyable");
        indexVec.resize(1);
    }

    ~PackedMemoryArray() = default;

    void insertElement(KeyType key, ValueType value) {
        if (this->numElements == 0) [[unlikely]] {
            insertEmpty(key, value);
            indexVec[0] = key;
            return;
        }

        // Find the appropriate segment for insertion
        auto segmentId = indexFindLeq(key);
        
        // Rebalance if segment is full
        if (getSegmentCount(segmentId) == this->segmentCapacity) {
            segmentId = rebalance(segmentId, key, value);
        }

        // Insert the element and update index if needed
        bool isNewMin = insertCommon(segmentId, key, value);
        if (isNewMin) {
            indexVec[segmentId] = key;
        }
    }

    bool isSorted() {
        KeyType previousKey;
        bool first = true;
        for (uint64_t i = 0; i < this->capacity; i++) {
            if (this->occupied[i]) {
                if (!first && this->keys[i] < previousKey) {
                    return false;
                }
                previousKey = this->keys[i];
                first = false;
            }
        }
        return true;
    }
};

// Implementation of rebalance
template <typename KeyType, typename ValueType>
int PackedMemoryArray<KeyType, ValueType>::rebalance(uint64_t segmentId, KeyType key, ValueType value) {
    size_t numElements = this->segmentCapacity + 1;
    double rho = 0.0;
    double theta = 1.0;
    double density = static_cast<double>(numElements)/this->segmentCapacity;
    size_t height = 1;

    // Initialize window parameters
    int windowLength = 1;
    int windowId = segmentId;
    int windowStart = segmentId;
    int windowEnd = segmentId;
    int indexLeft = segmentId - 1;
    int indexRight = segmentId + 1;

    // Find the smallest window that can accommodate the elements
    do {
        height++;
        windowLength *= 2;
        windowId /= 2;
        windowStart = windowId * windowLength;
        windowEnd = windowStart + windowLength;
        getThresholds(height, rho, theta);

        // Count elements in the window
        while (indexLeft >= windowStart) {
            numElements += getSegmentCount(indexLeft);
            indexLeft--;
        }
        while (indexRight < windowEnd) {
            numElements += getSegmentCount(indexRight);
            indexRight++;
        }

        density = ((double) numElements) / (windowLength * this->segmentCapacity);
    } while (density > theta && height < this->height);

    // Either spread elements or resize the array
    if (density <= theta) {
        return spread(numElements - 1, windowStart, windowLength, key);
    } else {
        return resize(key);
    }
}

// Implementation of spread
template <typename KeyType, typename ValueType>
int PackedMemoryArray<KeyType, ValueType>::spread(size_t numElements, size_t windowStart, size_t numOfSegments, KeyType key) {
    KeyType* newKeys;
    ValueType* newValues;
    bool* newOccupied;
    int segmentToInsert = windowStart;

    // Allocate temporary arrays
    posix_memalign((void**)&newKeys, 64, numOfSegments * this->segmentCapacity * sizeof(KeyType));
    posix_memalign((void**)&newValues, 64, numOfSegments * this->segmentCapacity * sizeof(ValueType));
    posix_memalign((void**)&newOccupied, 64, numOfSegments * this->segmentCapacity * sizeof(bool));
    memset(newOccupied, 0, numOfSegments * this->segmentCapacity * sizeof(bool));

    size_t oddSegments = numElements % numOfSegments;
    size_t elementsPerSegment = numElements / numOfSegments;
    size_t indexNext = windowStart * this->segmentCapacity;

    // Get pointers to the current window
    KeyType* __restrict currentKeys = this->keys;
    ValueType* __restrict currentValues = this->values;
    bool* __restrict currentOccupied = this->occupied;
    int16_t* __restrict segmentElementsCount = this->segmentElementsCount;

    // Find first non-empty element
    while (!currentOccupied[indexNext]) indexNext++;
    
    // Redistribute elements across segments
    size_t insertedElements = 0;
    for (auto i = 0; i < numOfSegments; i++) {
        size_t segmentElements = elementsPerSegment + (i < oddSegments);
        size_t currentIndex = i * this->segmentCapacity;
        segmentElementsCount[i + windowStart] = segmentElements;

        KeyType currentKey = currentKeys[indexNext];
        indexVec[i + windowStart] = currentKey;

        if (currentKey < key) {
            segmentToInsert = i + windowStart;
        }

        // Copy elements to new positions
        for(auto j = 0; j < segmentElements; j++) {
            newKeys[currentIndex] = currentKeys[indexNext];
            newValues[currentIndex] = currentValues[indexNext];
            newOccupied[currentIndex] = true;
            insertedElements++;

            if (insertedElements >= numElements) break;

            do { indexNext++; } while (!currentOccupied[indexNext]);
            currentIndex++;
        }
    }

    // Copy back to original arrays
    memcpy(currentKeys + windowStart * this->segmentCapacity, newKeys, numOfSegments * this->segmentCapacity * sizeof(KeyType));
    memcpy(currentValues + windowStart * this->segmentCapacity, newValues, numOfSegments * this->segmentCapacity * sizeof(ValueType));
    memcpy(currentOccupied + windowStart * this->segmentCapacity, newOccupied, numOfSegments * this->segmentCapacity * sizeof(bool));
    
    free(newKeys);
    free(newValues);
    free(newOccupied);

    return segmentToInsert;
}

// Implementation of resize
template <typename KeyType, typename ValueType>
int PackedMemoryArray<KeyType, ValueType>::resize(KeyType key) {
    // Calculate new capacity and parameters
    this->capacity *= 2;
    size_t numOfSegments = this->capacity / this->segmentCapacity;
    size_t numElements = this->numElements;
    size_t elementsPerSegment = numElements / numOfSegments;
    size_t oddSegments = numElements % numOfSegments;
    int segmentToInsert = 0;

    // Update height
    this->height = std::log2(this->capacity / this->segmentCapacity) + 1;
    indexVec.resize(numOfSegments);

    // Allocate new storage
    KeyType* oldKeys;
    ValueType* oldValues;
    bool* oldOccupied;
    int16_t* oldElementsCount;
    this->allocSegments(numOfSegments, this->segmentCapacity, &oldKeys, &oldValues, &oldOccupied, &oldElementsCount);

    // Swap old and new storage
    std::swap(oldKeys, this->keys);
    std::swap(oldValues, this->values);
    std::swap(oldOccupied, this->occupied);
    std::swap(oldElementsCount, this->segmentElementsCount);

    // Use smart pointers for automatic cleanup
    auto xFreePtr = [](void* ptr){ free(ptr); };
    std::unique_ptr<KeyType, decltype(xFreePtr)> oldKeysPtr { oldKeys, xFreePtr };
    std::unique_ptr<ValueType, decltype(xFreePtr)> oldValuesPtr { oldValues, xFreePtr };
    std::unique_ptr<bool, decltype(xFreePtr)> oldOccupiedPtr { oldOccupied, xFreePtr };
    std::unique_ptr<int16_t, decltype(xFreePtr)> oldElementsCountPtr { oldElementsCount, xFreePtr };

    // Get pointers to new storage
    KeyType* __restrict keys = this->keys;
    ValueType* __restrict values = this->values;
    bool* __restrict occupied = this->occupied;
    int16_t* __restrict segmentElementsCount = this->segmentElementsCount;

    // Find first non-empty element
    size_t indexNext = 0;
    while (!oldOccupied[indexNext]) indexNext++;

    // Redistribute elements
    size_t insertedElements = 0;
    for (auto i = 0; i < numOfSegments; i++) {
        size_t segmentElements = elementsPerSegment + (i < oddSegments);
        size_t currentIndex = i * this->segmentCapacity;
        segmentElementsCount[i] = segmentElements;

        KeyType currentKey = oldKeys[indexNext];
        indexVec[i] = currentKey;

        if (currentKey < key) {
            segmentToInsert = i;
        }

        // Copy elements to new positions
        for(size_t j = 0; j < segmentElements; j++) {
            keys[currentIndex] = oldKeys[indexNext];
            values[currentIndex] = oldValues[indexNext];
            occupied[currentIndex] = true;
            insertedElements++;

            if (insertedElements == numElements) break;

            do { indexNext++; } while (!oldOccupied[indexNext]);
            currentIndex++;
        }
    }

    return segmentToInsert;
}

} // namespace pma

#endif // PMA_HPP