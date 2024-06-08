// window that belongs:
//
// |               x
// |       x       |
// |   x   |   x   |   x   |  
// | x | x | x | x | x | x |  
//   1   2   3   4   5   6
//
//   odd = x + x_(n + 1)
//   even =  x + x_(n - 1)
//
#include <iostream>
#include <vector>
#include <optional>
#include <random>
#include <cmath>
#include <thread>
#include <chrono>
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

    PackedMemoryArray(uint64_t segmentSize) : segmentSize(segmentSize), capacity(segmentSize) {
        data = std::vector<std::optional<int>>(segmentSize, std::nullopt);
    }

    void checkIfSorted() {
        for (uint64_t i = 0; i < capacity - 1; i++) {
            if (data[i] && data[i + 1] && data[i].value() > data[i + 1].value()) {
                std::cerr << "Array is not sorted" << std::endl;
                exit(1);
            }
        }
        std::cout << "Array is sorted" << std::endl;
    }

    void print(bool overwrite = false, uint64_t highlightNumber = UINT64_MAX) {
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

    // lower threshold at level 1
    static constexpr double p1 = 0.5;
    // upper threshold at level 1
    static constexpr double t1 = 1;
    // lower threshold at top level h
    static constexpr double ph = 0.75;
    // upper threshold at top level h
    static constexpr double th = 0.75;

    double upperThresholdAtLevel(int level) {
        int height = getTreeHeight();
        return th + (t1 - th) * (height - level) / (height - 1);
    }

    double lowerThresholdAtLevel(int level) {
        int height = getTreeHeight();
        return ph - (ph - p1) * (height - level) / (height - 1);
    }

    // TODO: review this implementation, it doesn't seem to be the best way to do it
    void rebalance(uint64_t left, uint64_t right) {
        // count gaps and non-gaps
        // [ _ _ 3 4 5 _ _ _ _ _ ]
        // 6 gaps, 3 elements, 10 total
        //
        // gaps/elements = 2
        // [ _ _ 3 _ _ 4 _ _ 5 _ ]
        //
        // [ _ _ 3 4 5 6 _ _ _ _ ]
        // 6 gaps, 4 elements, 10 total
        // 6/4 = 1.5
        // [ 3 _ 4 _ 5 _ 6 _ _ _ ]
        // [ 3 _ _ 4 _ _ 5 _ _ 6 ]
        //
        //
        // [ 1 2 _ _ _ 3 4 5 6 7 ]
        // 3 gaps, 6 elements, 10 total
        // 0.5 gaps / element => 1 gap / 2 elements
        // [ 1 2 _ 3 4 _ 5 6 _ 7 ]
        //
        //
        // calculate gaps and elements for that segment
        // temporarily store the elements in a vector
        int numElements = 0;
        int segmentSize = right - left;
        std::vector<int> segmentElements;
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
                int64_t nearestLeft = mid,  nearestRight = mid;

                // 2 3 5 6 10 11 15 44 77 1554 1766 3151 4547 _  _  _  _  _  _  _   _  _  _  _
                // 0 1 2 3 4  5  6  7  8  9    10   11   12   13 14 15 16 17 18 19 20 21 22 23

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
        // have to perform a full linear search
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
    }

    // in paper example
    // p2, 11/12 = 0.91 (unbalanced) 10/12 = 0.83 (balanced)
    // p3, 19/24 = 0.79 (unbalanced) 18/24 = 0.75 (balanced)
    // 
    // it is possible to have 20 elements in both window, and upper level is unbalanced

    // check if the segment is balanced, if not, rebalance and check the upper level
    // if its balanced, check the upper level. If upper level is not balanced, rebalance and check the upper level
    // 
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


    uint64_t findGapAtLeftNeighbor(uint64_t segmentLeft, uint64_t segmentRight) {
        if (segmentLeft == 0) return UINT64_MAX;
        segmentRight = segmentLeft;
        segmentLeft = segmentRight - segmentSize;
        // return findGapWithinSegment(segmentLeft, segmentRight);
        return 0;
    }

    uint64_t findGapAtRightNeighbor(uint64_t segmentLeft, uint64_t segmentRight) {
        if (segmentRight == capacity) return UINT64_MAX;
        segmentLeft = segmentRight;
        segmentRight = segmentLeft + segmentSize;
        //return findGapWithinSegment(segmentLeft, segmentRight);
        return 0;
    }

    uint64_t findFirstGapFrom(uint64_t startingIndex) {
        uint64_t leftCursor = startingIndex;
        uint64_t rightCursor = startingIndex;
        uint64_t gapIndex = UINT64_MAX;

        while (leftCursor >= 0 || rightCursor < capacity) {
            if (leftCursor >= 0) {
                if (data[leftCursor] == std::nullopt) {
                    gapIndex = leftCursor;
                    break;
                }
                if (leftCursor == 0) break;
                leftCursor--;
            }
            if (rightCursor < capacity) {
                if (data[rightCursor] == std::nullopt) {
                    gapIndex = rightCursor;
                    break;
                }
                rightCursor++;
            }
        }
        return gapIndex;
    }

    uint64_t findGapWithinSegment(uint64_t left, uint64_t right, uint64_t cursor, uint64_t *totalElements) {
        uint64_t leftCursor = cursor;
        uint64_t rightCursor = cursor;
        uint64_t gapIndex = UINT64_MAX;

        while (leftCursor >= left || rightCursor < right) {
            if (leftCursor >= left) {
                if (data[leftCursor] == std::nullopt) {
                    if (gapIndex == UINT64_MAX) gapIndex = leftCursor;
                } else {
                    (*totalElements)++;
                }
                if (leftCursor == 0) break;
                leftCursor--;
            }
            if (rightCursor < right) {
                if (data[rightCursor] == std::nullopt) {
                    if (gapIndex == UINT64_MAX) gapIndex = rightCursor;
                } else {
                    (*totalElements)++;
                }
                rightCursor++;
            }
        }

        return gapIndex;
    }

    void insert(int value) {
        if (capacity == 0) return;
        bool fullRebalance = false;
        bool partialRebalance = false;

        DEBUG_PRINT << "Value to insert: " << value << std::endl;
        uint64_t mid = binarySearchPMA(value);

        // value already exists
        if (data[mid] && value == data[mid].value()) return;

        // at this point, 'mid' is the most important value here
        // meaning where we want to insert the value
        // if there is a gap, insert the value and that's it 
        if (data[mid] == std::nullopt) {
            DEBUG_PRINT << "Inserting value: " << value << " at index: " << mid << std::endl;
            insertElement(value, mid);
            return;
        } 

        DEBUG_PRINT << "Searching for nearest gap" << std::endl;
        uint64_t nearestGap = findFirstGapFrom(mid);

        DEBUG_PRINT << "Neareast gap found at: " << nearestGap << std::endl;

        // 'mid' is where we want the element to be placed
        // if nearestGap is greater than mid, shift right
        // if nearestGap is less than mid, shift left
        if (nearestGap > mid) {
            DEBUG_PRINT << "Shifting right" << std::endl;
            for (uint64_t i = nearestGap; i > mid; i--) {
                data[i] = data[i - 1];
            }
        } else {
            DEBUG_PRINT << "Shifting left" << std::endl;
            for (uint64_t i = nearestGap; i < mid; i++) {
                data[i] = data[i + 1];
            }
        }

        // insert the value into mid position
        insertElement(value, mid);
        checkForRebalancing(mid);

        // rebalance
        // v1
        // find the neareast gap, regardless the segment
        // trigger rebalance
        // that's it
	// v2
	// assume there will be always a gap in the segment:
	// 1. shift left or right
	// 2. insert
	// 3. check if the segment is full
	//    3a. is it full? rebalance the window (upper level)
	//        upper level is not balanced? 
	//           go to a higher level, rebalance
	//              upper level is not balanced?
	//                 go to a higher level, rebalance
	//                 ...
	//                 is root level? double the capacity and rebalance
        //                 for root level, keep count the number of elements
        //                 so we don't have to do a complete linear search
	

    }
};

void distInsert(PackedMemoryArray& pma) {
    Timer t;

    std::random_device rd;
    std::mt19937 eng(-1011927998);
    int seed = rd();

    DEBUG_PRINT << "Seed: " << seed << std::endl;
    //std::mt19937 eng(seed);
    std::uniform_int_distribution<> distr(0, 100);

    t.start();
    for (int count = 0; count < 100; count++) {
        int num = distr(eng);
        pma.insert(num);
//        pma.print(true, count);
//        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        pma.print();
    }

    double time_taken = t.stop();
    std::cout << "Head Inserts: " << time_taken/10000000.0 << std::endl;
    
}

int main() {
    PackedMemoryArray pma(6);

//    pma.insert(5);
//    pma.print();
//    pma.insert(15);
//    pma.print();
//    pma.insert(10);
//    pma.print();
//    pma.insert(2);
//    pma.print();
//    pma.insert(3);
//    pma.print();
//    pma.insert(11);
//    pma.print();
//    pma.insert(44);
//    pma.print();
//    pma.insert(6);
//    pma.print();
//    pma.insert(77);
//    pma.print();
    distInsert(pma);

    // pma.rebalance(0, pma.capacity - 1);
    //pma.rebalance(0, 24);
 //   pma.print();

//    pma.checkIfSorted();
 //   pma.printLevelsInformation();

    return 0;
}


// 2 3 5 6 10 11 15 44 77 1554 1766 3151 4547 _  _  _  _  _  _  _   _  _  _  _
// 0 1 2 3 4  5  6  7  8  9    10   11   12   13 14 15 16 17 18 19 20 21 22 23

//             x           x           x           x           x           x
// 0 _ _ 1 _ _ _ 2 _ _ 3 _ _ _ 4 _ _ 5 _ _ _ 6 _ _ 7 _ _ _ 8 _ _ 9 _ _ _ 10 _ _ 11 _ _ _ 12 _ _ 13 _ _ 14 15 _ 16 17 _ 18 19 _ 20 21 _ 22 23 _ 24 25 _ 26 27 _ 28 29 _ 30 31 32 _ 33 34 _ 35 36 _ 37 38 _ 39 40 _ 41 42 43 44 45 46 47 48
