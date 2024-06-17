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
    uint64_t minPos = binarySearchPMA(min);
    uint64_t maxPos = binarySearchPMA(max);

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
    return ph + 0.25 * diff;
}

double PackedMemoryArray::lowerThresholdAtLevel(int level) {
    int height = getTreeHeight();
    double diff = (((double) height) - level) / height;
    return ph - 0.25 * diff;
}

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

// Adapted Binary Search to handle gaps
uint64_t PackedMemoryArray::binarySearchPMA(uint64_t key) {
    uint64_t left = 0;
    uint64_t right = capacity - 1;
    uint64_t mid = 0;

    // binary search the key
    while (left <= right) {
        mid = left + (right - left) / 2;

        // key already exists, do nothing
        if (data[mid] && data[mid]->first == key) {
            DEBUG_PRINT << "key already exists" << std::endl;
            break;
        }

        if (data[mid] && data[mid]->first < key) {
            left = mid + 1;
        } else if (data[mid] && data[mid]->first > key) {
            // prevents underflow here
            if (mid == 0) {
                break;
            }
            right = mid - 1;
        } else {
            // data[mid] is a gap (std::nullopt), search nearest non-nullopt keys
            uint64_t nearestLeft = mid,  nearestRight = mid;

            while (nearestLeft > left && !data[nearestLeft]) nearestLeft--;
            while (nearestRight < right && !data[nearestRight]) nearestRight++;

            // no data between left and right
            if (!data[nearestLeft] && !data[nearestRight]) {
                break;
            }

            if (nearestLeft >= left && data[nearestLeft] && data[nearestLeft]->first >= key) {
                right = nearestLeft;
            } else if (nearestRight <= right && data[nearestRight] && data[nearestRight]->first <= key) {
                left = nearestRight;
            } else {
                // no valid entries around, or only gaps between left and right
                break;
            }
        }
    }
    return mid;
}

void PackedMemoryArray::rebalance(uint64_t left, uint64_t right) {
    // calculate gaps and elements for that segment
    // temporarily store the elements in a vector
    uint64_t numElements = 0;
    int segmentSizeToRebalance = right - left;

    // temporary copy elements here
    // idea: keep this vector globally and resize if necessary
    // instead of instantiating it every time
    std::vector<std::pair<int, int>> segmentElements;
    segmentElements.reserve(segmentSizeToRebalance);

    for (uint64_t i = left; i < right; i++) {
        if (data[i]) {
            segmentElements.push_back(data[i].value());
            numElements++;
            // clear the segment after copying
            data[i] = std::nullopt;
        }
    }

    DEBUG_PRINT << "Segment Size to Rebalance: " << segmentSizeToRebalance << std::endl;
    DEBUG_PRINT << "Num Elements: " << numElements << std::endl;

    double step = static_cast<double>(segmentSizeToRebalance) / numElements;

    // re-arrange data
    for (uint64_t i = 0; i < numElements; i++) {
        data[i * step + left] = segmentElements[i];
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
    DEBUG_PRINT << "Capacity: " << capacity << ", No of Segments: " << noOfSegments << ", Height: " << height << std::endl;
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
}

// it will usually be used in the beginning of insertions
bool PackedMemoryArray::checkIfFullAndRebalance() {
    if (totalElements == capacity) {
        DEBUG_PRINT << "Root level is full" << std::endl;
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

    DEBUG_PRINT << "Checking for rebalancing" << std::endl;
    DEBUG_PRINT << "Tree height: " << treeHeight << std::endl;

    bool triggerRebalance = false;
    // traverse the calibrator tree
    for (int level = 1; level <= treeHeight; level++) {
        // get the segment start and end for the current level
        getSegmentOffset(level, index, &start, &end);

        // calculate the density for that specific range
        double density = getDensity(start, end);

        DEBUG_PRINT << "> Level: " << level << ", Segment Left: " << start << ", Segment Right: " << end << std::endl;
        DEBUG_PRINT << "> Density: " << density << std::endl;
        DEBUG_PRINT << "> Total Elements: " << totalElements << ", Capacity: " << capacity << std::endl;

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
        DEBUG_PRINT << "> Upper Threshold: " << upperThreshold << std::endl;

        if (density > upperThreshold) {
            DEBUG_PRINT << "Segment is not balanced" << std::endl;
            // if level is root, double capacity and do full rebalancing
            if (level == treeHeight) {
                DEBUG_PRINT << "Root level is unbalanced" << std::endl;
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
            DEBUG_PRINT << "Segment is balanced" << std::endl;
            if (triggerRebalance) {
                rebalance(start, end);
            }
            break;
        }
    }
}

// found position will indicate if the gap was found to the left or right
// -1 for left, 1 for right
uint64_t PackedMemoryArray::findFirstGapFrom(uint64_t startingIndex) {
    uint64_t leftCursor = startingIndex;
    uint64_t rightCursor = startingIndex;
    uint64_t gapIndex = UINT64_MAX;

    while (leftCursor >= 0 || rightCursor < capacity) {
        if (rightCursor < capacity) {
            if (data[rightCursor] == std::nullopt) {
                gapIndex = rightCursor;
                break;
            }
            rightCursor++;
        }
        if (leftCursor >= 0) {
            if (data[leftCursor] == std::nullopt) {
                gapIndex = leftCursor;
                break;
            }
            if (leftCursor == 0) continue;
            leftCursor--;
        }
    }
    return gapIndex;
}

void PackedMemoryArray::deleteElement(int key) {
    uint64_t mid = binarySearchPMA(key);
    if (data[mid] && data[mid]->first == key) {
        data[mid] = std::nullopt;
        totalElements--;
        checkForRebalancing(mid);
    }
}

void PackedMemoryArray::insertElement(int key, int value) {
    assert(capacity > 0);

    uint64_t mid = binarySearchPMA(key);
    DEBUG_PRINT << "Key, Value to insert: " << key << "," << value << " | desired position: " << mid << std::endl;

    // key already exists
    if (data[mid] && key == data[mid]->first) return;

    // at this point, 'mid' is the most important value here
    // meaning where we want to insert the value
    // if there is a gap, insert the value and that's it 
    if (data[mid] == std::nullopt) {
        DEBUG_PRINT << "Inserting value: " << value << " at index: " << mid << std::endl;
        insertElement(key, value, mid);
        checkForRebalancing(mid);
        return;
    } 

    DEBUG_PRINT << "Searching for nearest gap" << std::endl;
    uint64_t nearestGap = findFirstGapFrom(mid);

    DEBUG_PRINT << "Neareast gap found at: " << nearestGap << ", position: " << std::endl;

    // 'mid' is where we want the element to be placed
    // if nearestGap is greater than mid, shift right
    // if nearestGap is less than mid, shift left
    if (nearestGap > mid) {
        // gap found at the right
        DEBUG_PRINT << "Shifting right" << std::endl;

        // bring the gap to the desired position
        if (key > data[mid]->first) {
            mid++;
        }

        // bring the gap to the left 
        for (uint64_t i = nearestGap; i > mid; i--) {
            data[i] = data[i - 1];
        }
    } else {
        DEBUG_PRINT << "Shifting left" << std::endl;

        if (key < data[mid]->first) {
            mid--;
        }
        for (uint64_t i = nearestGap; i < mid; i++) {
            data[i] = data[i + 1];
        }
    }

    // insert the value into mid position
    insertElement(key, value, mid);
    checkForRebalancing(mid);
}

} // namespace pma
