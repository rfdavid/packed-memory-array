/* PMA Implementation 
 * - no index
 * - rebalance on each insert
 */
#include <iostream>
#include <vector>
#include <optional>
#include <random>
#include <cmath>
#include <thread>
#include <chrono>
#include <algorithm>
#include "timer.hpp" // from reddragon

#ifdef DEBUG
#define DEBUG_PRINT std::cout
#else
#define DEBUG_PRINT if (false) std::cout
#endif


class PackedMemoryArray {
public:
    std::vector<std::optional<int>> data;
    uint64_t segmentSize;
    uint64_t capacity;
    uint64_t totalElements = 0;

    // lower threshold at level 1
    static constexpr double p1 = 0.5;
    // upper threshold at level 1
    static constexpr double t1 = 1;
    // lower threshold at top level h
    static constexpr double ph = 0.75;
    // upper threshold at top level h
    static constexpr double th = 0.75;

    PackedMemoryArray(uint64_t segmentSize) : segmentSize(segmentSize), capacity(segmentSize) {
        data = std::vector<std::optional<int>>(log2(segmentSize), std::nullopt);
    }

    /* Helper functions */
    void checkIfSorted() {
        for (uint64_t i = 0; i < capacity - 1; i++) {
            if (data[i] && data[i + 1] && data[i].value() > data[i + 1].value()) {
                std::cerr << "Array is not sorted" << std::endl;
                exit(1);
            }
        }
        std::cout << "Array is sorted" << std::endl;
    }

    void print(bool overwrite = false, int highlightNumber = -1) {
        if (overwrite) {
            std::cout << "\033[2J\033[1;1H";
        }
        for (uint64_t i = 0; i < capacity; i++) {
            if (data[i].has_value()) {
                if (data[i].value() == highlightNumber) {
                    std::cout << "\033[31m" << data[i].value() << "\033[0m ";
                } else {
                    std::cout << data[i].value() << " ";
                }
                // std::cout << "[" << i << "] => " << data[i].value() << " ";
            } else {
                std::cout << "_ ";
            }
        }
        std::cout << std::endl << std::endl;
    }

    double upperThresholdAtLevel(int level) {
        int height = getTreeHeight();
        return th + (t1 - th) * (height - level) / (height - 1);
    }

    double lowerThresholdAtLevel(int level) {
        int height = getTreeHeight();
        return ph - (ph - p1) * (height - level) / (height - 1);
    }

    void rebalance(uint64_t left, uint64_t right) {
        // calculate gaps and elements for that segment
        // temporarily store the elements in a vector
        uint64_t numElements = 0;
        int segmentSize = right - left;
        std::vector<int> segmentElements;
        segmentElements.reserve(segmentSize);

        for (uint64_t i = left; i < right; i++) {
            if (data[i]) {
                segmentElements.push_back(data[i].value());
                numElements++;
            }
        }

        double step = static_cast<double>(segmentSize - 1) / (numElements - 1);
        DEBUG_PRINT << "Rebalancing Step: " << step << " between " << left << " and " << right << std::endl;

        // clear the segment
        for (uint64_t i = left; i < right; i++) {
            data[i] = std::nullopt;
        }

        for (uint64_t i = 0; i < numElements; i++) {
            data[std::round(i * step) + left] = segmentElements[i];
        }
    }

    void insertElement(int value, uint64_t index) {
        data[index] = value;
        totalElements++;
    }

    int binarySearchPMA(int value) {
        uint64_t left = 0;
        uint64_t right = capacity - 1;
        uint64_t mid = 0;

        // binary search the value
        while (left <= right) {
            mid = left + (right - left) / 2;

            // value already exists, do nothing
            if (data[mid] && data[mid].value() == value) {
                DEBUG_PRINT << "Value already exists" << std::endl;
                break;
            }

            if (data[mid] && data[mid].value() < value) {
                left = mid + 1;
            } else if (data[mid] && data[mid].value() > value) {
                // prevents underflow here
                if (mid == 0) {
                    break;
                }
                right = mid - 1;
            } else {
                // data[mid] is a gap (std::nullopt), search nearest non-nullopt values
                uint64_t nearestLeft = mid,  nearestRight = mid;

                while (nearestLeft > left && !data[nearestLeft]) nearestLeft--;
                while (nearestRight < right && !data[nearestRight]) nearestRight++;

                // no data between left and right
                if (!data[nearestLeft] && !data[nearestRight]) {
                    break;
                }

                // re-adjust left and right
                // There is a neareast left value 
                //       12     > 12            yes                 4457                 >= 3159          
                if (nearestLeft >= left && data[nearestLeft] && data[nearestLeft].value() >= value) {
                    right = nearestLeft;
                } else if (nearestRight <= right && data[nearestRight] && data[nearestRight].value() <= value) {
                    left = nearestRight;
                } else {
                    // no valid entries around, or only gaps between left and right
                    break;
                }
            }
        }
        return mid;
    }

    int getTreeHeight() {
        int noOfSegments = capacity / segmentSize;
        if (noOfSegments == 1) return 1;
        int height = log2(noOfSegments) + 1;
        DEBUG_PRINT << "CApacity: " << capacity << ", No of Segments: " << noOfSegments << ", Height: " << height << std::endl;
        return height;
    }

    // get the segment offset going from the bottom to the requested level
    // TODO: store those values as in RMA
    void getSegmentOffset(int level, int index, uint64_t &segmentLeft, uint64_t &segmentRight) {
        int levelMultiplier = 1;
        segmentLeft = index - (index % segmentSize);
        segmentRight = segmentLeft + segmentSize;

        for (int i = 1; i < level; i++) {
            uint64_t segmentPos = (index / (segmentSize * levelMultiplier)) + 1;
            if (segmentPos % 2 == 0) {
                // even, left neighbor
                segmentLeft -= segmentSize*levelMultiplier;
            } else {
                // odd, right neighbor
                segmentRight += segmentSize*levelMultiplier;
            }
            levelMultiplier *= 2;
        }
    }

    double getDensity(uint64_t left, uint64_t right) {
        // if the segment is the root level, we don't
        // need to perform a full linear search
        if (left == 0 && right == capacity) {
            return static_cast<double>(totalElements) / capacity;
        }
        int segmentElements = 0;
        for (uint64_t i = left; i < right; i++) {
            if (data[i]) {
                segmentElements++;
            }
        }
        return static_cast<double>(segmentElements) / (right - left);
    }

    void doubleCapacity() {
        capacity *= 2;
        data.resize(capacity, std::nullopt);
        segmentSize = log2(capacity);
    }

    // in paper example
    // p2, 11/12 = 0.91 (unbalanced) 10/12 = 0.83 (balanced)
    // p3, 19/24 = 0.79 (unbalanced) 18/24 = 0.75 (balanced)
    // 
    // it is possible to have 20 elements in both window, and upper level is unbalanced

    // check if the segment is balanced, if not, rebalance and check the upper level
    // if its balanced, check the upper level. If upper level is not balanced, rebalance and check the upper level
    void checkForRebalancing(int index) {
        uint64_t segmentLeft = 0;
        uint64_t segmentRight = 0;
        int treeHeight = getTreeHeight();
        int limit = treeHeight; // test

        DEBUG_PRINT << "Checking for rebalancing" << std::endl;
        DEBUG_PRINT << "Tree height: " << treeHeight << std::endl;
        DEBUG_PRINT << "Limit: " << limit << std::endl;

        for (int level = 1; level <= treeHeight; level++) {
            getSegmentOffset(level, index, segmentLeft, segmentRight);
            double density = getDensity(segmentLeft, segmentRight);
            double upperThreshold = upperThresholdAtLevel(level);

            DEBUG_PRINT << "Level: " << level << ", Segment Left: " << segmentLeft << ", Segment Right: " << segmentRight << std::endl;
            DEBUG_PRINT << "Density: " << density << std::endl;
            DEBUG_PRINT << "Upper Threshold: " << upperThreshold << std::endl;

            // check root level
            DEBUG_PRINT << "Total Elements: " << totalElements << ", Capacity: " << capacity << std::endl;

            if (totalElements == capacity) {
                DEBUG_PRINT << "Root level is full" << std::endl;
                doubleCapacity();
                rebalance(0, capacity - 1);
                return;
            }

            if (density > upperThreshold) {
                DEBUG_PRINT << "Segment is not balanced" << std::endl;
                // if level is root, double capacity and do full rebalancing
                if (level == treeHeight) {
                    DEBUG_PRINT << "Root level is unbalanced" << std::endl;
                    doubleCapacity();
                    rebalance(0, capacity - 1);
                } else {
                    // otherwise, rebalance one level up
                    // TODO: we have to do this for level all over again to
                    // make sure the calibrator tree is properly balanced
                    getSegmentOffset(level+1, index, segmentLeft, segmentRight);
                    rebalance(segmentLeft, segmentRight);
                }
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

    void insert(int value) {
        if (capacity == 0) return;

        uint64_t mid = binarySearchPMA(value);
        DEBUG_PRINT << "Value to insert: " << value << ", desired position: " << mid << std::endl;

        // value already exists
        if (data[mid] && value == data[mid].value()) return;

        // at this point, 'mid' is the most important value here
        // meaning where we want to insert the value
        // if there is a gap, insert the value and that's it 
        if (data[mid] == std::nullopt) {
            DEBUG_PRINT << "Inserting value: " << value << " at index: " << mid << std::endl;
            insertElement(value, mid);
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
            if (value > data[mid].value()) {
                mid++;
            }

            for (uint64_t i = nearestGap; i > mid; i--) {
                data[i] = data[i - 1];
            }
        } else {
            DEBUG_PRINT << "Shifting left" << std::endl;

            if (value < data[mid].value()) {
                mid--;
            }
            for (uint64_t i = nearestGap; i < mid; i++) {
                data[i] = data[i + 1];
            }
        }

        // insert the value into mid position
        insertElement(value, mid);
        checkForRebalancing(mid);
    }
};

void distInsert(PackedMemoryArray& pma) {
    Timer t;

    std::random_device rd;
    //std::mt19937 eng(-1011927998);
    int seed = rd();

    DEBUG_PRINT << "Seed: " << seed << std::endl;
    std::mt19937 eng(seed);
    std::uniform_int_distribution<> distr(0, 10000);
    t.start();

    for (int count = 0; count < 1000; count++) {
        int num = distr(eng);
        pma.insert(num);
        pma.print();
//        pma.insert(count);
//        pma.print(true, count);
//        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//        pma.checkIfSorted();
        
    }

    double time_taken = t.stop();
    std::cout << "Head Inserts: " << time_taken/10000000.0 << std::endl;
}

int main() {
    PackedMemoryArray pma(4 /* initial capacity */);
    distInsert(pma);
    pma.checkIfSorted();

    return 0;
}
