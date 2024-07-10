/* use array as index */
#include "../include/pma_btree.hpp"

namespace pma {

/* Initialize PMA structure */

PMA::PMA(size_t segmentSize) : segmentCapacity(segmentSize) {
    // segmentCapacity = std::pow(2, std::ceil(log2(static_cast<double>(log2(segmentSize)))));
    capacity = segmentCapacity;
    height = 1;
    numElements = 0;

    // memory allocations
    allocSegments(1 /* num of segments */, segmentSize, &keys, &values, &segmentElementsCount);
}

// free resources
PMA::~PMA(){
    free(keys);
    free(values);
    free(segmentElementsCount);
    keys = nullptr;
    values = nullptr;
    segmentElementsCount = nullptr;
}

SumResult PackedMemoryArray::sum(int64_t min, int64_t max) {
    // find min
    auto segmentId = indexFindLeq(min);
    size_t start = segmentId * storage.segmentCapacity;
    size_t segmentEnd = start + storage.segmentCapacity;

    SumResult result;

    // find start position
    while (start < segmentEnd && storage.keys[start] < min) {
        start++;
    }

    result.m_first_key = storage.keys[start];

    // sum

    auto currentKey = storage.keys[start];
    while (start < storage.capacity && currentKey < max) {
        currentKey = storage.keys[start];
        if (currentKey > 0) {
            result.m_num_elements++;
            result.m_sum_keys += currentKey;
            result.m_sum_values += storage.values[start];
        }
        start++;
    }

    // find last
    do { start--; } while (storage.keys[start] < 0);
    result.m_last_key = storage.keys[start];

    return result;
}

// TODO: abstract the struct with alignas
void PMA::allocSegments(size_t numOfSegments, size_t segmentCapacity, 
        int64_t** keys, int64_t** values, decltype(segmentElementsCount)* segmentElementsCount){
    // reset the ptrs
    *keys = nullptr;
    *values = nullptr;
    *segmentElementsCount = nullptr;
    // TODO: check if memory was successfully acquired
    posix_memalign((void**) keys, 64 /* alignment */, numOfSegments * segmentCapacity * sizeof(keys[0]) /* size */ );
    posix_memalign((void**) values, 64, numOfSegments * segmentCapacity * sizeof(values[0]));
    posix_memalign((void**) segmentElementsCount, 64,  numOfSegments * sizeof(segmentElementsCount[0]));

    memset(*keys, -1, numOfSegments * segmentCapacity * sizeof(keys[0]));
    memset(*segmentElementsCount, 0, numOfSegments * sizeof(segmentElementsCount[0]));
}

/* Packed Memory Array Class */

// Initialize Packed Memory array:
// - create a dynamic (a,b)-tree index
// - initializa PMA struct with the initial segment capacity
PackedMemoryArray::PackedMemoryArray(uint64_t segmentCapacity) : 
     storage(segmentCapacity) {
    indexVec.resize(1, -1);
}

PackedMemoryArray::~PackedMemoryArray() {
    // TODO: use smart pointers
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
    } else {
        // find the segment which index is less or equal than the key
        auto segmentId = indexFindLeq(key);
        
        // is this segment full?
        if (getSegmentCount(segmentId) == storage.segmentCapacity) {
            segmentId = rebalance(segmentId, key, value);
        }

        int64_t newMinValue = insertCommon(segmentId, key, value);

        if (newMinValue > 0) {
            indexVec[segmentId] = newMinValue;
        }
    }

 //   dump();
//   dumpIndex();
}

// TODO: cache thresholds
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

    int windowLength = 1;
    int windowId = segmentId;
    int windowStart = segmentId;
    int windowEnd = segmentId;

    // [       abcd      ]  h = 3
    // [  ab   ] [  cd   ]  h = 2
    // [ aaa bbb ccc ddd ]  h = 1
    //   0    1   2   3
    //   ccc is full
    //
    //   storage.height = 3
    //   indexLeft = 1
    //   indexRight = 3
    //   windowId = 1
    //   windowLength = 2
    //   height = 2
    //   windowStart = 1 * 2 = 2 (inclusive)
    //   windowEnd = 2 + 2 = 4
    int indexLeft = segmentId - 1;
    int indexRight = segmentId + 1;

    do {
        height++;
        windowLength *= 2;
        windowId /= 2;
        windowStart = windowId * windowLength;
        windowEnd = windowStart + windowLength;
        getThresholds(height, rho, theta);

        // find the number of elements in the interval
        while (indexLeft >= windowStart) {
            numElements += getSegmentCount(indexLeft);
            indexLeft--;
        }
        while (indexRight < windowEnd){
            numElements += getSegmentCount(indexRight);
            indexRight++;
        }

        density = ((double) numElements) / (windowLength * storage.segmentCapacity);

      // while is not balanced and we have not reached the top level
    } while (density > theta && height < storage.height);

    if (density <= theta) {
        return spread(numElements - 1 /* havent inserted yet */, windowStart, windowLength, key);
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

    int64_t* __restrict currentKeys = storage.keys + windowStart * storage.segmentCapacity;
    int64_t* __restrict currentValues = storage.values + windowStart * storage.segmentCapacity;
    int16_t* __restrict segmentElementsCount = storage.segmentElementsCount + windowStart;

    // find the first non-empty element index
    while (currentKeys[indexNext] < 0) indexNext++;
    
    for (auto i = 0; i < numOfSegments; i++) {
        // remove all index from that segment

        size_t segmentElements = elementsPerSegment + (i < oddSegments);
        size_t currentIndex = i * storage.segmentCapacity;
        segmentElementsCount[i] = segmentElements;

        auto currentKey = currentKeys[indexNext];
        indexVec[i + windowStart] = currentKey;

        if (currentKey < key) {
            segmentToInsert = i + windowStart;
        }

        for(auto j = 0; j < segmentElements; j++) {
            keys[currentIndex] = currentKeys[indexNext];
            values[currentIndex] = currentValues[indexNext];
            insertedElements++;

            if (insertedElements >= numElements)  break;

            do { indexNext++; } while (currentKeys[indexNext] < 0);
            // next position to set
            currentIndex++;
        }
        segmentElementsCount[i] = segmentElements;
    }
    memcpy(currentKeys, keys, numOfSegments * storage.segmentCapacity * sizeof(keys[0]));
    memcpy(currentValues, values, numOfSegments * storage.segmentCapacity * sizeof(values[0]));
    free(keys);
    free(values);

    return segmentToInsert;
}

int PackedMemoryArray::resize(int64_t key) {
    size_t numOfSegments, oddSegments, elementsPerSegment;
    size_t numElements = storage.numElements;
    int segmentToInsert = 0;

    storage.capacity *= 2;
//    storage.segmentCapacity = std::pow(2, std::ceil(log2(static_cast<double>(log2(storage.capacity)))));
    numOfSegments = storage.capacity / storage.segmentCapacity;
    elementsPerSegment = numElements / numOfSegments;
    oddSegments = numElements % numOfSegments;

    storage.height = std::log2(storage.capacity / storage.segmentCapacity) + 1;

    indexVec.resize(numOfSegments, -1);

    int64_t* oldKeys;
    int64_t* oldValues;
    int16_t* oldElementsCount;

    // allocate where the new PMA will be
    // use 'old' because it will swap the pointers
    PMA::allocSegments(numOfSegments, storage.segmentCapacity, &oldKeys, &oldValues, &oldElementsCount);

    // oldKeys, oldValues and oldElementsCount will point to the old storage
    std::swap(oldKeys, storage.keys);
    std::swap(oldValues, storage.values);
    std::swap(oldElementsCount, storage.segmentElementsCount);

    // let unique_ptr to take responsibiltiy to free the old storage memory
    // using a custom deleter
    auto xFreePtr = [](void* ptr){ free(ptr); };
    std::unique_ptr<int64_t, decltype(xFreePtr)> oldKeysPtr { oldKeys, xFreePtr };
    std::unique_ptr<int64_t, decltype(xFreePtr)> oldValuesPtr { oldValues, xFreePtr };
    std::unique_ptr<int16_t, decltype(xFreePtr)> oldElementsCountPtr { oldElementsCount, xFreePtr };

    // initialize the key pointers for the new allocated space
    int64_t* __restrict keys = storage.keys;
    int64_t* __restrict values = storage.values;
    int16_t* __restrict segmentElementsCount = storage.segmentElementsCount;

    // fetch the first non-empty input segment
    size_t inputSegmentId = 0;
    size_t inputSize = oldElementsCount[0];

//    int64_t* inputKeys = oldKeys + storage.segmentCapacity - inputSize;
//    int64_t* inputValues = oldValues + storage.segmentCapacity - inputSize;

    // next non-empty value
    size_t indexNext = 0;

    // find first non-empty segment
    while (oldKeys[indexNext] < 0) indexNext++;

    size_t insertedElements = 0;

    for (auto i = 0; i < numOfSegments; i++) {
        size_t segmentElements = elementsPerSegment + (i < oddSegments);
        size_t currentIndex = i * storage.segmentCapacity;
        segmentElementsCount[i] = segmentElements;

        auto currentKey = oldKeys[indexNext];
        indexVec[i] = currentKey;

        // find the segment to insert
        if (currentKey < key) {
            segmentToInsert = i;
        }

        for(size_t j = 0; j < segmentElements; j++) {
            keys[currentIndex] = oldKeys[indexNext];
            values[currentIndex] = oldValues[indexNext];
            insertedElements++;

            if (insertedElements == numElements)  break;

            do { indexNext++; } while (oldKeys[indexNext] < 0);

            // next position to set
            currentIndex++;
        }
        segmentElementsCount[i] = segmentElements;
    }

    return segmentToInsert;
}

int64_t PackedMemoryArray::insertCommon(uint64_t segmentId, int64_t key, int64_t value) {
    // start pointers
    int64_t* __restrict keys = storage.keys + segmentId * storage.segmentCapacity;
    int64_t* __restrict values = storage.values + segmentId * storage.segmentCapacity;

    // check if the inserted key is the new minimum.
    // this will be used to update the index
//    size_t segmentElementsCount = storage.segmentElementsCount[segmentId];

    // find the position to insert the element
    // 
    //  3  4  8  _  _  9  10  11
    // insert 20:
    //
    // searchKey = 3
    // key = 20
    // 3 < 20
    
    int lastGap = -1;
    int insertPos = -1;
    int64_t minimum = key;

    // 
    //
    //   - keys[i] == gap?  gapIdx = i
    //   - keys[i] > key
    //         gapIdx == i-1?
    //            insert at i-1
    //         else 
    //            shift gapIdx to i-1
    //            insert at i-1
    //
    //         gapIdx == null?
    //            find gapIdx starting from i+1 (gapIdx is > i)
    //            shift gapIdx to i
    //
    //    i == segment size - 1
    //         keys[i] == null?
    //            insert at i
    //         else
    //            shift gapIdx to i
    //
    //     1  3  4  8  _  
    //     _  4  8  9  10 
    //     2  _  _  4  10 
    //
    //     key < keys[i]
    //       copy = keys[i]
    //       keys[i] = key
    //       key = copy
    //
    //    keys[i] == null
    //       keys[i] = key
    //       copy = null
    //
//
//    int64_t gapIndex = -1;
//    for (int i = 0; i < storage.segmentCapacity; i++) {
//        if (keys[i] == -1) {
//            // it is a gap
//            gapIndex = i;
//        } else {
//            // not a gap
//            // keys[i] > key?
//            if (keys[i] > key) {
//                // gap index is not defined?
//                if (gapIndex == -1) {
//                    // find the next gap
//                    for (gapIndex = i + 1; keys[gapIndex] > -1; gapIndex++);
//                    // shift the gap to i
//                    // gap is on the right, so shift elements to right
//                    while (gapIndex > i) {
//                        keys[gapIndex] = keys[gapIndex - 1];
//                        values[gapIndex] = values[gapIndex - 1];
//                        gapIndex--;
//                    }
//                    // insert at i
//                    keys[i] = key;
//                    values[i] = value;
//                } else {
//                    // gap index is already defined
//                    // shift gap index to the desired position if necessary
//                    // ex: insert 8
//                    //     _  4  6  9  10 
//                    //     g        i
//                    //     4  6  _  9  10 
//                    //           g       
//                    while(gapIndex != i - 1) {
//                        keys[gapIndex] = keys[gapIndex + 1];
//                        values[gapIndex] = values[gapIndex + 1];
//                        gapIndex++;
//                    }
//                    // insert at gapIndex
//                    keys[gapIndex] = key;
//                    values[gapIndex] = value;
//                }
//            }
//        }
//
//        if (storage.segmentCapacity - 1 == i) {
//            // last element
//            if (keys[i] == -1) {
//                // insert at i
//                keys[i] = key;
//                values[i] = value;
//            } else {
//                // shift gap index to i
//                while (gapIndex != i) {
//                    keys[gapIndex] = keys[gapIndex - 1];
//                    values[gapIndex] = values[gapIndex - 1];
//                    gapIndex--;
//                }
//                keys[gapIndex] = key;
//                values[gapIndex] = value;
//            }
//        }
//    }


    // first scan: find the first gap and where the element should be placed
    // second scan: shift the elements to insert the element
    for (int i = 0; i < storage.segmentCapacity; i++) {
        auto searchKey = keys[i];
        // check if it is a gap
        if (searchKey == -1) {
            // set last gap
            lastGap = i;
            // check if we already have a position to insert
            if (insertPos != -1) break;
        } else {
            // it is a valid element
            // is this element minimum?
            if (searchKey < key) {
                // no minimum to update
                minimum = -1;
            } else {
                // define the desired position
                if (insertPos == -1) {
                    insertPos = i;
                    // check lastGap 
                    if (lastGap != -1) break;
                }
            }
        }
    }

    // 3  4  8  _  _  _  10  11
    //                ^   ^
    // insert 9
    // last gap = 5
    // insertPos = 6
    //
    // 3  4  8  _  9  10  11  20
    //          ^             ^
    // insert 19
    // last gap = 3
    // insertPos = 7
    //
    //              |           |           |
    //  0  1  _  _  2  3  _  _  4  5  _  _  6  7  8  9
    //  insert 10
    //
    //
    
    // we have to insert at the end of the segment
    if (insertPos == -1) {
        // if there is no gap, we have to shift the elements
        if (lastGap != storage.segmentCapacity - 1) {
            // move gap to the right
            for (auto i = lastGap; i < storage.segmentCapacity - 1; i++) {
                keys[i] = keys[i + 1];
                values[i] = values[i + 1];
            }
        }
        insertPos = storage.segmentCapacity - 1;
    } else if (lastGap < insertPos) {
        // move gap to the right
        for (auto i = lastGap; i < insertPos; i++) {
            keys[i] = keys[i + 1];
            values[i] = values[i + 1];
        }
        insertPos--;
    } else if (lastGap > insertPos) {
        // shift right
        for (auto i = lastGap; i > insertPos; i--) {
            keys[i] = keys[i - 1];
            values[i] = values[i - 1];
        }
    }

    keys[insertPos] = key;
    values[insertPos] = value;

    // update the element count
    storage.segmentElementsCount[segmentId]++;
    storage.numElements++;

    return minimum;
}

int64_t PackedMemoryArray::getMinimum(uint64_t segmentId) const {
    auto segmentStart = segmentId * storage.segmentCapacity;
    int64_t* keys = storage.keys;

    // search for the first valid element
    while (keys[segmentStart] == -1) segmentStart++;

    return keys[segmentStart];
}

uint64_t PackedMemoryArray::getSegmentCount(uint64_t segmentId) const {
    return storage.segmentElementsCount[segmentId];
}

// binary serach the index
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

    if (ans == -1) return 0;

    return ans;
}

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


} // namespace pma



//    int lastGap = -1;
//    for (int i = 0; i < storage.segmentCapacity; i++) {
//        if (keys[i] == -1) {
//            // found a gap
//            lastGap = i;
//        } else {
//            // not a gap, is the key greater than the current key?
//            if (keys[i] > key) {
//                // element should be placed at the last found gap
//                if (gap == -1) {
//                    // no gaps, shift the elements
//                    for (auto j = storage.segmentCapacity - 1; j > i; j--) {
//                        if (keys[j] != -1) {
//                        keys[j] = keys[j - 1];
//                        values[j] = values[j - 1];
//                    }
//                }
//                break;
//            } else {
//                if (gap != -1) {
//                    // shift if there is a gap
//                    keys[i - 1] = keys[i];
//                    keys[i] = -1;
//                    // set the new last gap to insert
//                    lastGap = i;
//
//                    // shift values too
//                    values[i - 1] = values[i];
//                }
//            }
//        }
//    }
