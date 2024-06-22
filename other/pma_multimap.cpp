/* PMA Implementation 
 * - no index
 * - rebalance on each insert
 *   TODO: add tests, move to different files, use size_t, implemenet delete, 
 *   refactor to use more methods, separate public/private
 */
#include "pma_key_value.hpp"
#include <cmath>
#include <thread>
#include <algorithm>

namespace pma {

// this will be used to benchmark using rma implementation
// to simulate a range scan
// this implementation is the closest as possible to rma implementation
// TODO: create a specific find interval method: first find the first key
// (start), then do a binary search with left = start and right = capacity
SumResult PackedMemoryArray::sum(uint64_t min, uint64_t max) {
    uint64_t minPos = 0; 
    binarySearchPMA(min, &minPos);
    uint64_t maxPos = 0; 
    binarySearchPMA(max, &maxPos);

    // find the first non-null element 
    // this element will be the first key in the interval
    while (minPos < capacity && (data[minPos] == std::nullopt || data[minPos]->first < min)) {
        minPos++;
    }

    // find the last non-null element
    while (maxPos < capacity && (data[maxPos] == std::nullopt || data[maxPos]->first <= max)) {
        maxPos++;
    }

    // sum the values in the interval [min, max]
    SumResult result;
    result.m_first_key = data[minPos]->first;
    for (auto i = minPos; i < maxPos; i++) {
        if (data[i] != std::nullopt) {
            result.m_sum_keys += data[i]->first;
            result.m_sum_values += data[i]->second;
            result.m_num_elements++;
        }
    }

    // find the last non-null element
    result.m_last_key = std::numeric_limits<int64_t>::min();
    while (result.m_last_key == std::numeric_limits<int64_t>::min() && maxPos > minPos) {
        if (data[maxPos - 1] != std::nullopt) {
            result.m_last_key = data[maxPos -1]->first;
        } else {
            maxPos--;
        }
    }

    return result;
}

/* PMA Implementation */

// Get the upper threshold for a specific level
double PackedMemoryArray::upperThresholdAtLevel(int level) {
    int height = getTreeHeight();
    // the algorithm is very slow when using 0.75 threshold for the root level
    // pma from rewired paper uses ph + 0.25 * diff instead of defining th
    // that results in a lower level for the root node (~ 0.40) and it is
    // considerably faster
    // if (level == height) return th;
    // from rma implementation
    double diff = (((double) height) - level) / height;
    double threshold = th + 0.25 * diff;
    return threshold;
}

double PackedMemoryArray::lowerThresholdAtLevel(int level) {
    int height = getTreeHeight();
    double diff = (((double) height) - level) / height;
    return ph - 0.25 * diff;
}


// return true if the key already exists, false otherwise
// return the predecessor element
// return the last gap found, if there is any
bool PackedMemoryArray::binarySearchPMA(uint64_t key, uint64_t *index) {
    uint64_t left = 0;
    uint64_t right = capacity - 1;
    uint64_t mid = 0;

    // binary search the key
    while (left <= right) {
        mid = left + (right - left) / 2;

        // mid is a gap
        if (data[mid] == std::nullopt) {
            uint64_t nearestElement = mid;
            // search for the first valid element at the left
            while (nearestElement > left && !data[nearestElement]) nearestElement--;

            // found a valid element, adjust mid
            if (data[nearestElement]) {
                mid = nearestElement;
            } else {
                // no? search at the right
                nearestElement = mid;
                while (nearestElement < right && !data[nearestElement]) nearestElement++;
                mid = nearestElement;

                // is it still a gap? break
                if (!data[mid]) break;
            }
        }

        uint64_t currentKey = data[mid]->first;

        // key already exists
        if (currentKey == key) {
            *index = mid;
            return true;
        }

        // otherwise, adjust left and right
        if (currentKey < key) {
            left = mid + 1;
        } else if (currentKey > key) {
            // prevents underflow here
            if (mid == 0) break;
            right = mid - 1;
        }
    }

    *index = mid;
    return false;
}

void PackedMemoryArray::rebalance(uint64_t left, uint64_t right) {
    // calculate gaps and elements for that segment
    // temporarily store the elements in a vector
    uint64_t numElements = 0;
    int segmentSizeToRebalance = right - left;

    // temporary copy elements here
    // idea: keep this vector globally and resize if necessary
    // instead of instantiating it every time
    std::vector<std::pair<int, int>> elementsToResize;

    if (segmentSizeToRebalance > elementsToResize.capacity()) {
        elementsToResize.reserve(segmentSizeToRebalance);
    }

    for (auto i = left; i < right; i++) {
        if (data[i]) {
            // elementsToResize.push_back(data[i].value());
            elementsToResize[numElements] = data[i].value();
            numElements++;
            // clear the segment after copying
            data[i] = std::nullopt;
        }
    }

    double step = static_cast<double>(segmentSizeToRebalance) / numElements;

    // re-arrange data
    int64_t previousSegmentId = -1; 

    for (uint64_t i = 0; i < numElements; i++) {
        uint64_t pos = i * step + left;
        data[pos] = elementsToResize[i];

        // update index: is pos the first element of the segment?
        uint64_t segmentId = pos - (pos % segmentSize);

        // new segment is starting
        if (segmentId != previousSegmentId) {
            // find the index key for that specific segment
            auto indexKey = indexMap.find(segmentId);
            // no key, new segment is starting
            if (indexKey == indexMap.end()) {
                // add to the index
                index.insert(std::make_pair(elementsToResize[i].first, pos));
            } else {
                // update current index
                auto element = index.find(indexKey->second);
                // remove the old index
                index.erase(element);
                // insert the new index
                index.insert(std::make_pair(elementsToResize[i].first, pos));
            }
            // add or update the indexMap
            indexMap[segmentId] = elementsToResize[i].first;
        }
        previousSegmentId = segmentId;
    }
}

void PackedMemoryArray::insertElement(int key, int value, uint64_t index) {
    data[index] = std::make_pair(key, value);
    totalElements++;
}

// TODO: store after resizing
// return the height of the calibrator tree
int PackedMemoryArray::getTreeHeight() {
    int noOfSegments = capacity / segmentSize;
    if (noOfSegments == 1) return 1;
    int height = log2(noOfSegments) + 1;
    return height;
}

void PackedMemoryArray::getSegmentOffset(int level, int index, uint64_t *start, uint64_t *end) {
    // given the level of the calibrator tree and the segment size,
    // calculate the size of that window
    // the window size is 2^(level - 1) * segmentSize
    int windowSize = std::pow(2, level - 1) * segmentSize;
    // set start and end
    *start = index - (index % windowSize);
    *end = *start + windowSize;
}

// calculate how any elements are in this segment
double PackedMemoryArray::getDensity(uint64_t left, uint64_t right) {
    // if the segment is the root level, we don't
    // need to perform a full linear search
    if (left == 0 && right == capacity) {
        return static_cast<double>(totalElements) / capacity;
    }
    int segmentElements = 0;
    for (uint64_t i = left; i < right; i++) {
        if (data[i] != std::nullopt) {
            segmentElements++;
        }
    }
    return static_cast<double>(segmentElements) / (right - left);
}

// double capacity, resize and set the new segment size
void PackedMemoryArray::doubleCapacity() {
    capacity *= 2;
    data.resize(capacity, std::nullopt);
    // from rma, 2^ceil(log2(log2(n)))
    segmentSize = std::pow(2, std::ceil(log2(static_cast<double>(log2(capacity)))));
        index.clear();
        indexMap.clear();
}

// it will usually be used in the beginning of insertions
bool PackedMemoryArray::checkIfFullAndRebalance() {
    if (totalElements == capacity) {
        doubleCapacity();
        rebalance(0 /* start */, capacity - 1 /* end */);
        return true;
    }
    return false;
}

// in paper example
// p2, 11/12 = 0.91 (unbalanced) 10/12 = 0.83 (balanced)
// p3, 19/24 = 0.79 (unbalanced) 18/24 = 0.75 (balanced)
// 
// it is possible to have 20 elements in both window, and upper level is unbalanced
// check if the segment is balanced, if not, rebalance and check the upper level
// if its balanced, check the upper level. If upper level is not balanced, rebalance and check the upper level
void PackedMemoryArray::checkForRebalancing(int index) {
    if (checkIfFullAndRebalance()) return;

    uint64_t start = 0;
    uint64_t end = 0;
    int treeHeight = getTreeHeight();

    bool triggerRebalance = false;
    // traverse the calibrator tree
    for (int level = 1; level <= treeHeight; level++) {
        // get the segment start and end for the current level
        getSegmentOffset(level, index, &start, &end);

        // calculate the density for that specific range
        double density = getDensity(start, end);

        if (level == 1) {
            // only trigger rebalancing when the bottom segment is full
            if (density == 1) {
                continue;
            } else {
                // otherwise, don't check
                break;
            }
        }

        // get the upper threshold for the current level
        double upperThreshold = upperThresholdAtLevel(level);

        if (density > upperThreshold) {
            // if level is root, double capacity and do full rebalancing
            if (level == treeHeight) {
                doubleCapacity();
                rebalance(0, capacity - 1);
            } else {
                // do not rebalance yet, only mark for rebalancing
                // let the loop continue to check the upper level
                // once it find a balanced segment, it will trigger the rebalance
                triggerRebalance = true;
            }
        } else {
            // this level is balanced. check if we need to rebalance
            // because of the unbalanced lower levels
            if (triggerRebalance) {
                rebalance(start, end);
            }
            break;
        }
    }
}

void PackedMemoryArray::deleteElement(int key) {
    uint64_t mid = 0;
    if (binarySearchPMA(key, &mid)) {
        data[mid] = std::nullopt;
        totalElements--;
        checkForRebalancing(mid);
    }
}

uint64_t PackedMemoryArray::findGapWithinSegment(uint64_t pos) {
    uint64_t segmentStart = pos - (pos % segmentSize);
    for (uint64_t i = segmentStart; i < segmentStart + segmentSize; i++) {
        if (data[i] == std::nullopt) {
            return i;
        }
    }
}

// found position will indicate if the gap was found to the left or right
uint64_t PackedMemoryArray::findFirstGapFrom(uint64_t startingIndex) {
    int64_t leftCursor = static_cast<int64_t>(startingIndex);
    uint64_t rightCursor = startingIndex;
    while (leftCursor >= 0 || rightCursor < capacity) {
        // check right side
        if (rightCursor < capacity) {
            if (data[rightCursor] == std::nullopt) {
                return rightCursor;
            }
            rightCursor++;
        }
        // check left side
        if (leftCursor >= 0) {
            if (data[leftCursor] == std::nullopt) {
                return leftCursor;
            }
            leftCursor--;
        }
    }
    return UINT64_MAX;
}


// update index, start/end from underlying array
void PackedMemoryArray::updateIndex(int64_t start, int64_t end) {
    // iterate over segments
    for (auto i = start; i <= end; i += segmentSize) {
        // find the segment id for the start
        uint64_t segmentId = start - (start % segmentSize);

        // find the index that correspond to this segment
        // using indexMap
        auto idx = index.find(indexMap[segmentId]);

        // update
        updateIndex(idx);
    }
}

void PackedMemoryArray::updateIndex(std::multimap<int64_t, int64_t>::iterator& indexElement) {
    // we know the key from the index
    // find the position where this segment starts
    uint64_t segmentStartPos = indexElement->second - (indexElement->second % segmentSize);
    auto segmentId = segmentStartPos;

    // find the first valid element in this segment
    while (segmentStartPos < segmentStartPos + segmentSize && data[segmentStartPos] == std::nullopt) {
        segmentStartPos++;
    }

    // update the index if there is any change
    if (data[segmentStartPos]->first != indexElement->first || data[segmentStartPos]->second != indexElement->second) {
        index.erase(indexElement); 
        indexElement = index.insert(std::make_pair(data[segmentStartPos]->first, segmentStartPos));
        indexMap[segmentId] = data[segmentStartPos]->first;
    }
}

void PackedMemoryArray::insertElement(int64_t key, int64_t value) {
    assert(capacity > 0);

    if (totalElements == 0) {
        // first time inserting
        insertElement(key, value, 0);
        // add first index here
        index.insert(std::make_pair(key, 0));
        // map the segment in a hashmap
        indexMap[0] = key;
        return;
    }

    // first element that is greater than or equal the key
    // from the index
    auto element = index.upper_bound(key);
    
    // elements exists
    if (element->first == key) return;

    // where we want to place the element

    int64_t position = element->second;

    // if there is no first element greater than the key 
    // it will return the index.end()
    // search for the previous element
    if (element == index.end()) { 
        element--;
        position = element->second;
    }

    // perform a linear search to find the desired position
    // starting from the index
    uint64_t indexValue = element->first;
    uint64_t indexPos = element->second;

    if (indexValue > key) {
        // go left
        while (position >= 0) {
            if (data[position]) {
                // element already exists
                if (data[position]->first == key) {
                    return;
                }
                if (data[position]->first < key) break;
            }
            if (position == 0) break;
        }
    } else if (indexValue < key) {
        // we have to go right
        while (position < capacity) {
            if (data[position]) {
                if (data[position]->first == key) {
                    return;
                }
                if (data[position]->first > key) break;
            }
            position++;
        }
        position--;
    }

    DEBUG_PRINT << "old position: " << indexPos << ", new position: " << position << std::endl;
    print(segmentSize, true);

    // element found
    if (data[position] && data[position]->first == key) {
        return;
    }

    // insert if it's a gap
    if (data[position] == std::nullopt) {
        insertElement(key, value, position);
        checkForRebalancing(position);
        return;
    }

    uint64_t nearestGap = findFirstGapFrom(position);

    // 'position' is where we want the element to be placed
    if (nearestGap > position) {
        // gap found at the right
        // 2 4 6 8 _ 12
        //     p   ng

        // bring the gap to the desired position
        if (key > data[position]->first) position++;

        // bring the gap to the left 
        for (auto i = nearestGap; i > position; i--) {
            data[i] = data[i - 1];
        }
        // insert the value into the desired position
        insertElement(key, value, position);

        // update index from position to nearestGap
        updateIndex(position, nearestGap);
    } else {
        // bring gap to the right
        if (key < data[position]->first) position--;

        for (auto i = nearestGap; i < position; i++) {
            data[i] = data[i + 1];
        }

        // insert the value into the desired position
        insertElement(key, value, position);

        // update index from neareast gap to position 
        updateIndex(nearestGap, position);
    }

    insertElement(key, value, position);
    checkForRebalancing(position);

}

} // namespace pma




//    if (nearestGap > element->second) {
//        segmentsAffected = std::ceil(static_cast<double>(nearestGap - element->second) / segmentSize) + 20;
//        for (int i = 0; i < segmentsAffected; i++) {
//            if (element != index.end()) {
//                ++element;
//                if (element != index.end()) {
//                    updateIndex(element);
//                }
//            }
//        }
//    } else {
//        segmentsAffected = std::ceil(static_cast<double>(element->second - nearestGap) / segmentSize) + 20;
//        for (int i = 0; i < segmentsAffected; i++) {
//            if (element != index.begin()) {
//                --element;
//                updateIndex(element);
//            }
//        }
//    }


//    for (int i = 0; i < segmentsAffected && element != index.end(); i++) {
//        if (element != index.begin()) {
//            updateIndex(element);
//        }
//        if (element != index.begin()) {
//            --element;
//        }
//    }

    // finally check for rebalancing
// 
// 
// Adapted Binary Search to handle gaps
//uint64_t PackedMemoryArray::binarySearchPMA(uint64_t key) {
//     uint64_t left = 0;
//     uint64_t right = capacity - 1;
//     uint64_t mid = 0;
// 
//     // binary search the key
//     while (left <= right) {
//         mid = left + (right - left) / 2;
// 
//         // key already exists, do nothing
//         if (data[mid] && data[mid]->first == key) {
//             DEBUG_PRINT << "key already exists" << std::endl;
//             break;
//         }
// 
//         if (data[mid] && data[mid]->first < key) {
//             left = mid + 1;
//         } else if (data[mid] && data[mid]->first > key) {
//             // prevents underflow here
//             if (mid == 0) {
//                 break;
//             }
//             right = mid - 1;
//         } else {
//             // data[mid] is a gap (std::nullopt), search nearest non-nullopt keys
//             uint64_t nearestLeft = mid,  nearestRight = mid;
// 
//             while (nearestLeft > left && !data[nearestLeft]) nearestLeft--;
//             while (nearestRight < right && !data[nearestRight]) nearestRight++;
// 
//             // no data between left and right
//             if (!data[nearestLeft] && !data[nearestRight]) {
//                 break;
//             }
// 
//             if (nearestLeft >= left && data[nearestLeft] && data[nearestLeft]->first >= key) {
//                 right = nearestLeft;
//             } else if (nearestRight <= right && data[nearestRight] && data[nearestRight]->first <= key) {
//                 left = nearestRight;
//             } else {
//                 // no valid entries around, or only gaps between left and right
//                 break;
//             }
//         }
//     }
//     return mid;
// }
 

//	void rebalance(uint64_t from, uint64_t to) {
//		uint64_t capacity = to - from;
//		uint64_t n = 0;
//		for (uint64_t i = from; i < to; ++i) {
//			if (data[i] != std::nullopt) n++;
//		}
//
//		uint64_t frequency = (capacity << 8) / n;
//
//		uint64_t read_index = from + n - 1;
//		uint64_t write_index = (to << 8) - frequency;
//
//		while ((write_index >> 8) > read_index) {
//			data[write_index >> 8] = data[read_index];
//			data[read_index] = std::nullopt;
//			read_index--;
//			write_index -= frequency;
//		}
//	}

//        void halveCapacity() {
//            std::vector<std::pair<int, int>> segmentElements;
//            capacity /= 2;
//            data.resize(capacity, std::nullopt);
//            segmentSize = std::pow(2, std::ceil(log2(static_cast<double>(log2(capacity)))));
//
//            for (const auto& element : data) {
//                if (element) {
//                    segmentElements.push_back(element);
//                }
//            }
//            for (uint64_t i = left; i < right; i++) {
//                if (data[i]) {
//                    segmentElements.push_back(data[i].value());
//                    numElements++;
//                    // clear the segment after copying
//                    data[i] = std::nullopt;
//                }
//            }
//        }
//
// adapted binary search to handle gaps
// int binarySearchPMA2(int key) {
//     uint64_t left = 0;
//     uint64_t right = capacity - 1;
//     uint64_t mid = 0;
// 
//     // binary search the key
//     while (left <= right) {
//         mid = left + (right - left) / 2;
// 
//         // mid is a gap
//         if (data[mid] == std::nullopt) {
//             uint64_t nearestLeft = mid,  nearestRight = mid;
// 
//             // search nearest non-nullopt keys
//             while (nearestLeft > left && !data[nearestLeft]) nearestLeft--;
//             while (nearestRight < right && !data[nearestRight]) nearestRight++;
// 
//             if (nearestLeft >= left && data[nearestLeft] && data[nearestLeft]->first >= key) {
//                 right = nearestLeft;
//             } else if (nearestRight <= right && data[nearestRight] && data[nearestRight]->first <= key) {
//                 left = nearestRight;
//             } else {
//                 // no valid entries around, or only gaps between left and right
//                 break;
//             }
//             continue;
//         }
// 
//         // key already exists
//         if (data[mid]->first == key) {
//             DEBUG_PRINT << "key already exists" << std::endl;
//             break;
//         }
// 
//         if (data[mid]->first < key) {
//             left = mid + 1;
//         } else if (data[mid]->first > key) {
//             // prevents underflow here
//             if (mid == 0) break;
//             right = mid - 1;
//         }
//     }
//     return mid;
// }
//
// uint64_t PackedMemoryArray::findFirstGapFrom(uint64_t startingIndex) {
//     uint64_t leftCursor = startingIndex;
//     uint64_t rightCursor = startingIndex;
//     uint64_t gapIndex = UINT64_MAX;
// 
//     while (leftCursor >= 0 || rightCursor < capacity) {
//         if (rightCursor < capacity) {
//             if (data[rightCursor] == std::nullopt) {
//                 gapIndex = rightCursor;
//                 break;
//             }
//             rightCursor++;
//         }
//         if (leftCursor >= 0) {
//             if (data[leftCursor] == std::nullopt) {
//                 gapIndex = leftCursor;
//                 break;
//             }
//             if (leftCursor == 0) continue;
//             leftCursor--;
//         }
//     }
//     return gapIndex;
// }
//



// - we inserted the element at 'position' 
// void PackedMemoryArray::updateIndex(int64_t insertedKey, int64_t insertedPosition, std::multimap<int64_t, int64_t>::iterator& lowerBoundElement) {
//     // segment id of the inserted element
//     uint64_t segmentId = insertedPosition / segmentSize;
// 
//     // segment id of the lower bound element
//     uint64_t lowerBoundSegmentId = lowerBoundElement->second / lowerBoundElement->second;
// 
//     // find where the segment starts 
//     uint64_t segmentStart = insertedPosition - (insertedPosition % segmentSize);
// 
//     // go to the right until find the first valid element
//     // _ 2  6  8  12
//     while (segmentStart < segmentStart + segmentSize && data[segmentStart] == std::nullopt) {
//         segmentStart++;
//     }
// 
//     // inserted element and lower bound element are in the same segment
//     DEBUG_PRINT << "            -> Segment ID: " << segmentId << ", Lower Bound Segment ID: " << lowerBoundSegmentId << std::endl;
//     if (segmentId == lowerBoundSegmentId) {
//         // the first element in the segment is greater than what is in the index
//         // update
//         DEBUG_PRINT << "            -> Segment Start: " << segmentStart << ", Lower Bound Element: " << lowerBoundElement->first << std::endl;
//         if (data[segmentStart]->first <= lowerBoundElement->first) {
//             index.erase(lowerBoundElement); 
//             index.insert(std::make_pair(insertedKey, insertedPosition));
//             return;
//         }
//     }
// }
//
// void PackedMemoryArray::rebalance(uint64_t left, uint64_t right) {
//     // calculate gaps and elements for that segment
//     // temporarily store the elements in a vector
//     uint64_t numElements = 0;
//     int segmentSizeToRebalance = right - left;
// 
//     // temporary copy elements here
//     // idea: keep this vector globally and resize if necessary
//     // instead of instantiating it every time
//     std::vector<std::pair<int, int>> segmentElements;
//     segmentElements.reserve(segmentSizeToRebalance);
// 
//     for (uint64_t i = left; i < right; i++) {
//         if (data[i]) {
//             segmentElements.push_back(data[i].value());
//             numElements++;
//             // clear the segment after copying
//             data[i] = std::nullopt;
//         }
//     }
// 
//     DEBUG_PRINT << "Segment Size to Rebalance: " << segmentSizeToRebalance << std::endl;
//     DEBUG_PRINT << "Num Elements: " << numElements << std::endl;
// 
//     double step = static_cast<double>(segmentSizeToRebalance) / numElements;
// 
//     // re-arrange data
//     for (uint64_t i = 0; i < numElements; i++) {
//         data[i * step + left] = segmentElements[i];
//     }
// }


    // if there is a gap, insert the value and that's it 
//    if (data[position] == std::nullopt) {
//        DEBUG_PRINT << "Inserting value: " << value << " at index: " << position << std::endl;
//        insertElement(key, value, position);
//        // updateIndex(key, position, element);
//        updateIndex(element);
//        checkForRebalancing(position);
//        return;
//    } 

