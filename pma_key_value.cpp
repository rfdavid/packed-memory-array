/* PMA Implementation 
 * - no index
 * - rebalance on each insert
 *   TODO: move to different files, use size_t, implemenet delete, 
 *   refactor to use more methods, separate public/private
 */
#include <iostream>
#include <vector>
#include <optional>
#include <random>
#include <cmath>
#include <thread>
#include <chrono>
#include <algorithm>
#include <list>
#include "timer.hpp" // from reddragon

#ifdef DEBUG
#define DEBUG_PRINT std::cout
#else
#define DEBUG_PRINT if (false) std::cout
#endif

class PackedMemoryArray {
    public:
        // from rma for benchmarking
        struct SumResult {
            int64_t m_first_key = 0; // the first key that qualifies inside the interval [min, max]. Undefined if m_num_elements == 0
            int64_t m_last_key = 0; // the last key that qualifies inside the interval [min, max]. Undefined if m_num_elements == 0
            uint64_t m_num_elements = 0; // the total number of elements inside the interval [min, max]
            int64_t m_sum_keys = 0; // the aggregate sum of all keys in the interval [min, max]
            int64_t m_sum_values = 0; // the aggregate sum of all values in the interval [min, max]
        };

        std::vector<std::optional<std::pair<int, int>>> data;
        uint64_t segmentSize;
        uint64_t capacity;
        uint64_t totalElements = 0;

        // lower threshold at level 1
        static constexpr double p1 = 0.1;
        // upper threshold at level 1
        static constexpr double t1 = 1;
        // lower threshold at top level h
        static constexpr double ph = 0.30;
        // upper threshold at top level h
        static constexpr double th = 0.75;

        PackedMemoryArray(uint64_t capacity) : capacity(capacity) {
            segmentSize = std::pow(2, std::ceil(log2(static_cast<double>(log2(capacity)))));
            data = std::vector<std::optional<std::pair<int, int>>>(capacity, std::nullopt);
        }

        /* Helper functions */
        void checkIfSorted() {
            for (uint64_t i = 0; i < capacity - 1; i++) {
                if (data[i] && data[i + 1] && data[i]->first > data[i + 1]->first) {
                    std::cerr << "Array is not sorted" << std::endl;
                    exit(1);
                }
            }
            std::cout << "Array is sorted" << std::endl;
        }

        void printStats() {
            std::cout << "Capacity: " << capacity << std::endl;
            std::cout << "Segment Size: " << segmentSize << std::endl;
            std::cout << "Total Elements: " << totalElements << std::endl;
        }

        void print(int segmentSize = 0, bool printIndex = false) {
            for (uint64_t i = 0; i < capacity; i++) {
                if (i > 0 && segmentSize > 0 && i % segmentSize == 0) {
                    std::cout << " | ";
                }
                if (data[i] != std::nullopt) {
                    if (printIndex) {
                        std::cout << i << ": ";
                    }
                    std::cout << data[i]->first << " (" << data[i]->second << ") ";
                } else {
                    std::cout << "_ ";
                }
            }
            std::cout << std::endl << std::endl;
        }

        // this will be used to benchmark using rma implementation
        // to simulate a range scan
        // this implementation is the closest as possible to rma implementation
        SumResult sum(int64_t min, int64_t max) {
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

        bool elemExistsAt(int index) const {
            return data[index] != std::nullopt;
        }

        int64_t elemAt(int index) const {
            return data[index]->first;
        }

        uint64_t size() const {
            return capacity;
        }

        double upperThresholdAtLevel(int level) {
            int height = getTreeHeight();
            //            if (level == height) return th;

            // from rma implementation:
            double diff = (((double) height) - level) / height;

            //            DEBUG_PRINT << " -------------------------------------------  height " << height << ", level " << level << "  >   th (slow): " << th << ", calculated (fast): " << ph + 0.25 * diff << std::endl;

            return ph + 0.25 * diff;
            // return th + (t1 - th) * (height - level) / height;
        }

        double lowerThresholdAtLevel(int level) {
            int height = getTreeHeight();
            return ph - (ph - p1) * (height - level) / (height);
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

        void rebalance(uint64_t left, uint64_t right) {
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

        void insertElement(int key, int value, uint64_t index) {
            data[index] = std::make_pair(key, value);
            totalElements++;
        }

        int binarySearchPMA(int key) {
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

        // adapted binary search to handle gaps
        int binarySearchPMA2(int key) {
            uint64_t left = 0;
            uint64_t right = capacity - 1;
            uint64_t mid = 0;

            // binary search the key
            while (left <= right) {
                mid = left + (right - left) / 2;

                // mid is a gap
                if (data[mid] == std::nullopt) {
                    uint64_t nearestLeft = mid,  nearestRight = mid;

                    // search nearest non-nullopt keys
                    while (nearestLeft > left && !data[nearestLeft]) nearestLeft--;
                    while (nearestRight < right && !data[nearestRight]) nearestRight++;

                    if (nearestLeft >= left && data[nearestLeft] && data[nearestLeft]->first >= key) {
                        right = nearestLeft;
                    } else if (nearestRight <= right && data[nearestRight] && data[nearestRight]->first <= key) {
                        left = nearestRight;
                    } else {
                        // no valid entries around, or only gaps between left and right
                        break;
                    }
                    continue;
                }

                // key already exists
                if (data[mid]->first == key) {
                    DEBUG_PRINT << "key already exists" << std::endl;
                    break;
                }

                if (data[mid]->first < key) {
                    left = mid + 1;
                } else if (data[mid]->first > key) {
                    // prevents underflow here
                    if (mid == 0) break;
                    right = mid - 1;
                }
            }
            return mid;
        }

        // TODO: store after resizing
        int getTreeHeight() {
            int noOfSegments = capacity / segmentSize;
            if (noOfSegments == 1) return 1;
            int height = log2(noOfSegments) + 1;
            DEBUG_PRINT << "Capacity: " << capacity << ", No of Segments: " << noOfSegments << ", Height: " << height << std::endl;
            return height;
        }

        // segment size for a segment is given by 2^l * segmentSize, starting from level 0
        // we know the position
        //  left = index - (index % segmentSize);
        //  right = left + segmentSize;
        //
        //                                                     s = 16
        //                    |                                s = 8
        //  0 1 2 3 | 4 5 6 7 | 8 9 10 11 | 12 13 14 15        s = 4
        //  seg size = 4
        //  el = 10
        //  level = 1
        //  size = 2^1 * 4 = 8
        //  left = 10 - (10 % 8) = 8
        //  right = 8 + 8 = 16
        //
        //  el = 12
        //  left = 12 - (12 % 8) = 8
        //  right = 8 + 8 = 16
        //
        //  el = 5
        //  left = 5 - (5 % 8) = 0
        //  right = 0 + 8 = 8
        //
        //

        void getSegmentOffset(int level, int index, uint64_t &segmentLeft, uint64_t &segmentRight) {
            // TODO: improve naming (windowSize / segmentSize)
            int windowSize = std::pow(2, level-1) * segmentSize;
            segmentLeft = index - (index % windowSize);
            segmentRight = segmentLeft + windowSize;
        }

        double getDensity(uint64_t left, uint64_t right) {
            // if the segment is the root level, we don't
            // need to perform a full linear search

            if (left == 0 && right == capacity) {
                return static_cast<double>(totalElements) / capacity;
            }
            int segmentElements = 0;
            for (uint64_t i = left; i < right; i++) {
                if (data[i] != std::nullopt) {
                    // DEBUG_PRINT << "[" << i << "]: " << data[i].value() << ", ";
                    segmentElements++;
                }
            }
            return static_cast<double>(segmentElements) / (right - left);
        }

        void doubleCapacity() {
            capacity *= 2;
            data.resize(capacity, std::nullopt);
            // from rma, 2^ceil(log2(log2(n)))
            segmentSize = std::pow(2, std::ceil(log2(static_cast<double>(log2(capacity)))));
        }

        bool checkIfFullAndRebalance() {
            if (totalElements == capacity) {
                DEBUG_PRINT << "Root level is full" << std::endl;
                doubleCapacity();
                rebalance(0, capacity - 1);
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
        void checkForRebalancing(int index) {
            if (checkIfFullAndRebalance()) return;

            uint64_t segmentLeft = 0;
            uint64_t segmentRight = 0;
            int treeHeight = getTreeHeight();

            DEBUG_PRINT << "Checking for rebalancing" << std::endl;
            DEBUG_PRINT << "Tree height: " << treeHeight << std::endl;

            bool triggerRebalance = false;
            for (int level = 1; level <= treeHeight; level++) {
                getSegmentOffset(level, index, segmentLeft, segmentRight);
                double density = getDensity(segmentLeft, segmentRight);

                DEBUG_PRINT << "> Level: " << level << ", Segment Left: " << segmentLeft << ", Segment Right: " << segmentRight << std::endl;
                DEBUG_PRINT << "> Density: " << density << std::endl;
                DEBUG_PRINT << "> Total Elements: " << totalElements << ", Capacity: " << capacity << std::endl;

                if (level == 1) {
                    // only trigger rebalancing when the bottom segment is full
                    if (density == 1) {
                        continue;
                    } else {
                        // otherwise, we don't need to check
                        break;
                    }
                }

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
                        // otherwise, rebalance the upper segment and continue checking
                        // the upper levels through the loop
                        // getSegmentOffset(level+1, index, segmentLeft, segmentRight);
                        // rebalance(segmentLeft, segmentRight);
                        triggerRebalance = true;
                    }
                } else {
                    DEBUG_PRINT << "Segment is balanced" << std::endl;
                    if (triggerRebalance) {
                        rebalance(segmentLeft, segmentRight);
                    }
                    break;
                }
            }
        }

        // found position will indicate if the gap was found to the left or right
        // -1 for left, 1 for right
        uint64_t findFirstGapFrom(uint64_t startingIndex) {
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

        void deleteElement(int key) {
            uint64_t mid = binarySearchPMA(key);
            if (data[mid] && data[mid]->first == key) {
                data[mid] = std::nullopt;
                totalElements--;
                checkForRebalancing(mid);
            }
        }

        void insertElement(int key, int value) {
            if (capacity == 0) return;

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
};

void distInsert(PackedMemoryArray& pma) {
    Timer t;

    std::random_device rd;
    std::mt19937 eng(-1011927998);
    int seed = rd();

    DEBUG_PRINT << "Seed: " << seed << std::endl;
    //std::mt19937 eng(seed);
    std::uniform_int_distribution<> distr(0, 100000000);

    t.start();
    for (int count = 1; count <= 30; count++) {
        //       int num = distr(eng);
        pma.insertElement(count, count*10);
        //pma.print(pma.segmentSize);
        //pma.insertElement(count);
        //        pma.print(true, count);
        //        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        //        pma.checkIfSorted();

    }
    double time_taken = t.stop();
    std::cout << "Head Inserts: " << time_taken/10000000.0 << std::endl;
}

void sumTest(PackedMemoryArray& pma, int64_t min, int64_t max) {
    PackedMemoryArray::SumResult result = pma.sum(min, max);
    std::cout << "First Key: " << result.m_first_key << std::endl;
    std::cout << "Last Key: " << result.m_last_key << std::endl;
    std::cout << "Sum Keys: " << result.m_sum_keys << std::endl;
    std::cout << "Sum Values: " << result.m_sum_values << std::endl;
    std::cout << "Num Elements: " << result.m_num_elements << std::endl << std::endl;
}

void runSumTest(PackedMemoryArray& pma) {
    sumTest(pma, 0, 10); // 55
    sumTest(pma, 1, 10); // 55
    sumTest(pma, 2, 10); // 54
    sumTest(pma, 10, 15); // 75
    sumTest(pma, 98, 200); // 197
                           //
    sumTest(pma, 98, 200); // 197
    sumTest(pma, 28, 28); // 28
}

void manualInsert(PackedMemoryArray& pma) {
    std::list<int> keys = {5, 10, 6, 17, 1, 21, 9, 12, 8, 16, 20, 13, 7, 3, 15, 19, 14, 11, 22, 18, 4, 2};

    for (auto key : keys) {
        pma.print(pma.segmentSize);
        pma.insertElement(key, key*10);
    }
}

int main() {
    PackedMemoryArray pma(64 /* initial capacity */);
//    distInsert(pma);

    manualInsert(pma);

//    uint64_t index = pma.binarySearchPMA(12);
//    sumTest(pma, 28, 28); // 28
//
//    std::cout << "Index: " << index << std::endl;


    // pma.rebalance(0, pma.capacity - 1);
    pma.printStats();
    pma.checkIfSorted();

    return 0;
}
