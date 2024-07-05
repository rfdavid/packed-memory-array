/* Inserts: 1.64294 */

#include "pma_btree_v2a.hpp"

namespace pma {

/* Initialize PMA structure */

PMA::PMA(size_t segmentSize) : segmentCapacity(hyperceil(segmentSize)) {
    capacity = segmentCapacity;
    height = 1;
    numElements = 0;

    // memory allocations
    allocSegments(1 /* num of segments */, segmentSize, &keys, &values, &segmentElementsCount);
}

// free resources
// TODO: use smart pointers instead
PMA::~PMA(){
    free(keys);
    free(values);
    free(segmentElementsCount);
    keys = nullptr;
    values = nullptr;
    segmentElementsCount = nullptr;
}

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
//        INDEX->insert(key, 0); // point to segment 0
        indexVec[0] = key;
    } else {
        // find the index less or equal than key
        auto segmentId = indexFindLeq(key);
        bool elementInsertedOnRebalance = false;

        // is this segment full?
        if (getSegmentCount(segmentId) == storage.segmentCapacity) {
            elementInsertedOnRebalance = rebalance(segmentId, key, value);


            // rebalanced but the element was not inserted
            // TODO: perhaps insert the element on resize to avoid another
            // search here
            if (!elementInsertedOnRebalance) {
                segmentId = indexFindLeq(key);
            }
        }

 //       std::cout << "Inserting on segment: " << segmentId << std::endl;
        if (!elementInsertedOnRebalance) {
            // return the key of the minimum element in the segment
            int64_t pivotOld = getMinimum(segmentId);
            bool minimumUpdated = insertCommon(segmentId, key, value);

            // minimum was updated, update the index
            if (minimumUpdated) {
                int64_t pivotNew = getMinimum(segmentId);
                // updateIndex(pivotOld, pivotNew);
                indexVec[segmentId] = pivotNew;
            }
        }
    }

//    dump();
//    dumpIndex();
}

// TODO: cache thresholds
void PackedMemoryArray::getThresholds(size_t height, double& lower, double& upper) const {
    double diff = (((double) storage.height) - height) / storage.height;
    lower = ph - 0.25 * diff;
    upper = th + 0.25 * diff;
}

bool PackedMemoryArray::rebalance(uint64_t segmentId, int64_t key, int64_t value) {
    size_t numElements = storage.segmentCapacity + 1;
    double rho = 0.0;
    double theta = 1.0;
    double density = static_cast<double>(numElements)/storage.segmentCapacity;
    size_t height = 1;

    int windowLength = 1;
    int windowId = segmentId;
    int windowStart = segmentId;
    int windowEnd = segmentId;

     if (storage.height > 1) {
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
 
         } while (density > theta  && height < storage.height);
     }

    if (density <= theta) {
//        std::cout << "+++++++++++++++++++++++++++++++++ Spread ++++++++++++++++++++++++++++++++" << std::endl;
        spread(numElements, windowStart, windowLength, key, value);
        storage.numElements += 1;
        return true;
    } else {
//        std::cout << "+++++++++++++++++++++++++++++++++ Resize ++++++++++++++++++++++++++++++++" << std::endl;
        // TODO: refactor, resize and insert
        resize();
        return false;
    }
}

// Approach
// 	1.	Allocate a new chunk of the same size as the segment.
// 	2.	Initialize this chunk to -1.
// 	3.	Spread non-null elements from the original segment into this new chunk.
// 	4.	Copy the new chunk back over the original segment.
void PackedMemoryArray::spread(size_t numElements, size_t windowStart, size_t numOfSegments, int64_t key, int64_t value) {
    int64_t* keys;
    int64_t* values;

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
    int64_t nextKey = currentKeys[indexNext];

    for (auto i = 0; i < numOfSegments; i++) {
        size_t segmentElements = elementsPerSegment + (i < oddSegments);
        size_t currentIndex = i * storage.segmentCapacity;
        segmentElementsCount[i] = segmentElements;

        if (nextKey > key) {
            indexVec[i + windowStart] = key;
        } else {
            indexVec[i + windowStart] = currentKeys[indexNext];
        }

        for(auto j = 0; j < segmentElements; j++) {
            if (nextKey > key || (key != INT_MAX && insertedElements+1 == numElements)) {
                // insert that key first
                // and do not increase indexNext
                keys[currentIndex] = key;
                values[currentIndex] = value;
                insertedElements++;
                if (insertedElements >= numElements)  break;
                key = INT_MAX;
            } else {
                // nextKey come first, key is greater
                keys[currentIndex] = nextKey;
                values[currentIndex] = currentValues[indexNext];
                insertedElements++;
                if (insertedElements >= numElements)  break;
                if (key != INT_MAX && insertedElements+1 < numElements) {
                    do { indexNext++; } while (currentKeys[indexNext] < 0);
                }
            }
            nextKey = currentKeys[indexNext];

            // next position to set
            currentIndex++;
        }

        segmentElementsCount[i] = segmentElements;
    }
    memcpy(currentKeys, keys, numOfSegments * storage.segmentCapacity * sizeof(keys[0]));
    memcpy(currentValues, values, numOfSegments * storage.segmentCapacity * sizeof(values[0]));
    free(keys);
    free(values);
}

void PackedMemoryArray::resize() {
    size_t numOfSegments, oddSegments, elementsPerSegment;
    size_t numElements = storage.numElements;

    // TODO: review this
//    do {
        storage.capacity *= 2;
//        storage.segmentCapacity = std::pow(2, std::ceil(log2(static_cast<double>(log2(storage.capacity)))));
        numOfSegments = storage.capacity / storage.segmentCapacity;
        elementsPerSegment = numElements / numOfSegments;
        oddSegments = numElements % numOfSegments;
        // always make room for one element more
 //   } while (elementsPerSegment + (oddSegments > 0) == storage.segmentCapacity);

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

        indexVec[i] = oldKeys[indexNext];

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
}

bool PackedMemoryArray::insertCommon(uint64_t segmentId, int64_t key, int64_t value){
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
    //   insertPos = -1 ;
    //   lastGap = 4;
    
    int lastGap = -1;
    int insertPos = -1;
    bool minimum = true;

    // first scan: find the first gap and where the element should be placed
    // second scan: shift the elements to insert the element
    for (int i = 0; i < storage.segmentCapacity; i++) {
        auto searchKey = keys[i];
        if (searchKey == -1) {
            lastGap = i;
            // check insertPos
            if (insertPos != -1) break;
        } else {
            if (searchKey < key) {
                minimum = false;
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
    // if there is no gap, we have to shift the elements
    if (insertPos == -1) {
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

void PackedMemoryArray::updateIndex(int64_t oldKey, int64_t newKey) {
    uint64_t segmentId = 0;
//    INDEX->remove_any(oldKey, &segmentId);
//    INDEX->insert(newKey, segmentId);
}

uint64_t PackedMemoryArray::getSegmentCount(uint64_t segmentId) const {
    return storage.segmentElementsCount[segmentId];
}

uint64_t PackedMemoryArray::indexFindLeq(int64_t key) const {
//    uint64_t value = 0;
//    bool found = INDEX->find_first(key, nullptr, &value);
//    return found ? value : 0;

    auto it = std::upper_bound(indexVec.begin(), indexVec.end(), key);

    if (it == indexVec.begin()) {
//        std::cout << "a ---> 0" << std::endl;
        return 0;
    } else if (it == indexVec.end()) {
//        std::cout << "b ---> " << (indexVec.size() - 1) << std::endl;
        return indexVec.size() - 1;
    } else {
 //       std::cout << "c ---> " << (it - indexVec.begin() - 1) << std::endl;
        return it - indexVec.begin() -1 ;
    }
}

void PackedMemoryArray::dumpIndex() {
    for (auto i = 0; i < indexVec.size(); i++) {
        std::cout << "Index: " << i << " Key: " << indexVec[i] << std::endl;
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
