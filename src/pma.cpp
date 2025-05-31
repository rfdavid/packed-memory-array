/**
 * @file pma.cpp
 * @brief Implementation of the Packed Memory Array (PMA)
 */

#include "pma.hpp"

namespace pma {

/* Initialize PMA structure */
PMA::PMA(size_t segmentSize) : segmentCapacity(segmentSize) {
    capacity = segmentCapacity;
    height = 1;
    numElements = 0;

    // Allocate initial memory for one segment
    allocSegments(1, segmentSize, &keys, &values, &segmentElementsCount);
}

PMA::~PMA() {
    free(keys);
    free(values);
    free(segmentElementsCount);
    keys = nullptr;
    values = nullptr;
    segmentElementsCount = nullptr;
}

void PMA::allocSegments(size_t numOfSegments, size_t segmentCapacity, 
        int64_t** keys, int64_t** values, decltype(segmentElementsCount)* segmentElementsCount) {
    // Reset pointers
    *keys = nullptr;
    *values = nullptr;
    *segmentElementsCount = nullptr;

    // Allocate aligned memory for each array
    posix_memalign((void**) keys, 64, numOfSegments * segmentCapacity * sizeof(keys[0]));
    posix_memalign((void**) values, 64, numOfSegments * segmentCapacity * sizeof(values[0]));
    posix_memalign((void**) segmentElementsCount, 64, numOfSegments * sizeof(segmentElementsCount[0]));

    // Initialize arrays
    memset(*keys, -1, numOfSegments * segmentCapacity * sizeof(keys[0]));
    memset(*segmentElementsCount, 0, numOfSegments * sizeof(segmentElementsCount[0]));
}

// ============================================================================
// PackedMemoryArray Implementation
// ============================================================================

PackedMemoryArray::PackedMemoryArray(uint64_t segmentCapacity) : storage(segmentCapacity) {
    indexVec.resize(1, -1);
}

PackedMemoryArray::~PackedMemoryArray() {
    // Memory is managed by PMA destructor
}

void PackedMemoryArray::insertEmpty(int64_t key, int64_t value) {
    storage.keys[0] = key;
    storage.values[0] = value;
    storage.segmentElementsCount[0] = 1;
    storage.numElements = 1;
}

void PackedMemoryArray::insertElement(int64_t key, int64_t value) {
    if (storage.numElements == 0) [[unlikely]] {
        insertEmpty(key, value);
        indexVec[0] = key;
        return;
    }

    // Find the appropriate segment for insertion
    auto segmentId = indexFindLeq(key);
    
    // Rebalance if segment is full
    if (getSegmentCount(segmentId) == storage.segmentCapacity) {
        segmentId = rebalance(segmentId, key, value);
    }

    // Insert the element and update index if needed
    int64_t newMinValue = insertCommon(segmentId, key, value);
    if (newMinValue > 0) {
        indexVec[segmentId] = newMinValue;
    }
}

void PackedMemoryArray::getThresholds(size_t height, double& lower, double& upper) const {
    double diff = (((double) storage.height) - height) / storage.height;
    lower = ph - 0.25 * diff;
    upper = th + 0.25 * diff;
}

int PackedMemoryArray::rebalance(uint64_t segmentId, int64_t key, int64_t value) {
    size_t numElements = storage.segmentCapacity + 1;
    double rho = 0.0;
    double theta = 1.0;
    double density = static_cast<double>(numElements)/storage.segmentCapacity;
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

        density = ((double) numElements) / (windowLength * storage.segmentCapacity);
    } while (density > theta && height < storage.height);

    // Either spread elements or resize the array
    if (density <= theta) {
        return spread(numElements - 1, windowStart, windowLength, key);
    } else {
        return resize(key);
    }
}

// Approach
// 	1.	Allocate a new chunk of the same size as the segment.
// 	2.	Initialize this chunk to -1.
// 	3.	Spread non-null elements from the original segment into this new chunk.
// 	4.	Copy the new chunk back over the original segment.
int PackedMemoryArray::spread(size_t numElements, size_t windowStart, size_t numOfSegments, int64_t key) {
    int64_t* keys;
    int64_t* values;
    int segmentToInsert = windowStart;

    // allocate memory for the new segment
    posix_memalign((void**)&keys, 64, numOfSegments * storage.segmentCapacity * sizeof(keys[0]));
    posix_memalign((void**)&values, 64, numOfSegments * storage.segmentCapacity * sizeof(values[0]));
    memset(keys, -1, numOfSegments * storage.segmentCapacity * sizeof(keys[0]));

    // example:
    // 1301 elements to be distributed to 12 segments
    // 1301 / 12 = 108.4167
    // we fill 108 * 12 = 1296 elements
    // we still need to fill 1301 - 1296 = 5 elements = 1301 % 12
    //
    size_t oddSegments = numElements % numOfSegments;
    size_t elementsPerSegment = numElements / numOfSegments;
    size_t indexNext = 0;
    size_t insertedElements = 0;

    // Get pointers to the current window
    int64_t* __restrict currentKeys = storage.keys + windowStart * storage.segmentCapacity;
    int64_t* __restrict currentValues = storage.values + windowStart * storage.segmentCapacity;
    int16_t* __restrict segmentElementsCount = storage.segmentElementsCount + windowStart;

    // find the first non-empty element index
    while (currentKeys[indexNext] < 0) indexNext++;
    
    // Redistribute elements across segments
    for (auto i = 0; i < numOfSegments; i++) {
        size_t segmentElements = elementsPerSegment + (i < oddSegments);
        size_t currentIndex = i * storage.segmentCapacity;
        segmentElementsCount[i] = segmentElements;

        auto currentKey = currentKeys[indexNext];
        indexVec[i + windowStart] = currentKey;

        if (currentKey < key) {
            segmentToInsert = i + windowStart;
        }

        // Copy elements to new positions
        for(auto j = 0; j < segmentElements; j++) {
            keys[currentIndex] = currentKeys[indexNext];
            values[currentIndex] = currentValues[indexNext];
            insertedElements++;

            if (insertedElements >= numElements) break;

            do { indexNext++; } while (currentKeys[indexNext] < 0);
            currentIndex++;
        }
    }

    // Copy back to original arrays
    memcpy(currentKeys, keys, numOfSegments * storage.segmentCapacity * sizeof(keys[0]));
    memcpy(currentValues, values, numOfSegments * storage.segmentCapacity * sizeof(values[0]));
    
    // Cleanup
    free(keys);
    free(values);

    return segmentToInsert;
}

int PackedMemoryArray::resize(int64_t key) {
    // Calculate new capacity and parameters
    storage.capacity *= 2;
    size_t numOfSegments = storage.capacity / storage.segmentCapacity;
    size_t numElements = storage.numElements;
    size_t elementsPerSegment = numElements / numOfSegments;
    size_t oddSegments = numElements % numOfSegments;
    int segmentToInsert = 0;

    // Update height
    storage.height = std::log2(storage.capacity / storage.segmentCapacity) + 1;
    indexVec.resize(numOfSegments, -1);

    // Allocate new storage
    int64_t* oldKeys;
    int64_t* oldValues;
    int16_t* oldElementsCount;
    PMA::allocSegments(numOfSegments, storage.segmentCapacity, &oldKeys, &oldValues, &oldElementsCount);

    // Swap old and new storage
    std::swap(oldKeys, storage.keys);
    std::swap(oldValues, storage.values);
    std::swap(oldElementsCount, storage.segmentElementsCount);

    // Use smart pointers for automatic cleanup
    auto xFreePtr = [](void* ptr){ free(ptr); };
    std::unique_ptr<int64_t, decltype(xFreePtr)> oldKeysPtr { oldKeys, xFreePtr };
    std::unique_ptr<int64_t, decltype(xFreePtr)> oldValuesPtr { oldValues, xFreePtr };
    std::unique_ptr<int16_t, decltype(xFreePtr)> oldElementsCountPtr { oldElementsCount, xFreePtr };

    // Get pointers to new storage
    int64_t* __restrict keys = storage.keys;
    int64_t* __restrict values = storage.values;
    int16_t* __restrict segmentElementsCount = storage.segmentElementsCount;

    // Find first non-empty element
    size_t indexNext = 0;
    while (oldKeys[indexNext] < 0) indexNext++;

    // Redistribute elements
    size_t insertedElements = 0;
    for (auto i = 0; i < numOfSegments; i++) {
        size_t segmentElements = elementsPerSegment + (i < oddSegments);
        size_t currentIndex = i * storage.segmentCapacity;
        segmentElementsCount[i] = segmentElements;

        auto currentKey = oldKeys[indexNext];
        indexVec[i] = currentKey;

        if (currentKey < key) {
            segmentToInsert = i;
        }

        // Copy elements to new positions
        for(size_t j = 0; j < segmentElements; j++) {
            keys[currentIndex] = oldKeys[indexNext];
            values[currentIndex] = oldValues[indexNext];
            insertedElements++;

            if (insertedElements == numElements) break;

            do { indexNext++; } while (oldKeys[indexNext] < 0);
            currentIndex++;
        }
    }

    return segmentToInsert;
}

int64_t PackedMemoryArray::insertCommon(uint64_t segmentId, int64_t key, int64_t value) {
    // Get pointers to segment
    int64_t* __restrict keys = storage.keys + segmentId * storage.segmentCapacity;
    int64_t* __restrict values = storage.values + segmentId * storage.segmentCapacity;

    // Find insertion position and check if key is new minimum
    int lastGap = -1;
    int insertPos = -1;
    int64_t minimum = key;

    // First scan: find gap and insertion position
    for (int i = 0; i < storage.segmentCapacity; i++) {
        auto searchKey = keys[i];
        if (searchKey == -1) {
            lastGap = i;
            if (insertPos != -1) break;
        } else {
            if (searchKey < key) {
                minimum = -1;
            } else if (insertPos == -1) {
                insertPos = i;
                if (lastGap != -1) break;
            }
        }
    }
    
    // Handle insertion at end of segment
    if (insertPos == -1) {
        if (lastGap != storage.segmentCapacity - 1) {
            size_t numElements = storage.segmentCapacity - lastGap - 1;
            std::memmove(&keys[lastGap], &keys[lastGap + 1], numElements * sizeof(keys[0]));
            std::memmove(&values[lastGap], &values[lastGap + 1], numElements * sizeof(values[0]));
        }
        insertPos = storage.segmentCapacity - 1;
    } else if (lastGap < insertPos) {
        // Shift elements right
        size_t numElements = insertPos - lastGap;
        std::memmove(&keys[lastGap], &keys[lastGap + 1], numElements * sizeof(keys[0]));
        std::memmove(&values[lastGap], &values[lastGap + 1], numElements * sizeof(values[0]));
        insertPos--;
    } else if (lastGap > insertPos) {
        // Shift elements left
        size_t numElements = lastGap - insertPos;
        std::memmove(&keys[insertPos + 1], &keys[insertPos], numElements * sizeof(keys[0]));
        std::memmove(&values[insertPos + 1], &values[insertPos], numElements * sizeof(values[0]));
    }

    // Insert new element
    keys[insertPos] = key;
    values[insertPos] = value;

    // Update counts
    storage.segmentElementsCount[segmentId]++;
    storage.numElements++;

    return minimum;
}

// ============================================================================
// Helper Methods
// ============================================================================

int64_t PackedMemoryArray::getMinimum(uint64_t segmentId) const {
    auto segmentStart = segmentId * storage.segmentCapacity;
    int64_t* keys = storage.keys;
    while (keys[segmentStart] == -1) segmentStart++;
    return keys[segmentStart];
}

uint64_t PackedMemoryArray::getSegmentCount(uint64_t segmentId) const {
    return storage.segmentElementsCount[segmentId];
}

uint64_t PackedMemoryArray::indexFindLeq(int64_t key) const {
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

bool PackedMemoryArray::isSorted() {
    int64_t previousData = -1;
    for (uint64_t i = 0; i < storage.capacity - 1; i++) {
        if (storage.keys[i] > 0) {
            if (storage.keys[i] < previousData) {
                return false;
            }
            previousData = storage.keys[i];
        }
    }
    return true;
}

// ============================================================================
// Debug Methods
// ============================================================================

void PackedMemoryArray::dumpIndex() {
    for (auto i = 0; i < indexVec.size(); i++) {
        std::cout << "Index: " << i << " Key: " << indexVec[i] << std::endl;
    }
}

void PackedMemoryArray::dumpValues() {
    int64_t* keys = storage.keys;
    for (size_t i = 0; i < storage.capacity; i++) {
        if (keys[i] != -1) {
            std::cout << keys[i] << std::endl;
        }
    }
}

void PackedMemoryArray::dump() {
    int64_t* keys = storage.keys;
    for (size_t i = 0; i < storage.capacity; i++) {
        if (keys[i] == -1) {
            std::cout << " _ ";
        } else {
            std::cout << " " << keys[i] << " ";
        }
        if ((i + 1) % storage.segmentCapacity == 0) {
            std::cout << "|";
        }
    }
    std::cout << std::endl << std::endl;
}

} // namespace pma
