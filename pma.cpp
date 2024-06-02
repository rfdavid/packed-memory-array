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
    void print() {
        for (uint64_t i = 0; i < capacity; i++) {
            if (data[i].has_value()) {
                std::cout << data[i].value() << " ";
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
        int height = log2(capacity/noOfSegments) + 1;
        return height;
    }

    // get the segment offset going from the bottom to the requested level
    // TODO: store those values as in RMA
    void getSegmentOffset(int level, int index, uint64_t &segmentLeft, uint64_t &segmentRight) {
        int levelMultiplier = 1;
        int segmentLeft = index - (index % segmentSize);
        int segmentRight = segmentLeft + segmentSize;

        for (int i = 1; i < level; i++) {
            uint64_t segmentPos = (index / (segmentSize * levelMultipler)) + 1;
            if (segmentPos % 2 == 0) {
                // even, left neighbor
                segmentLeft -= segmentSize*levelMultipler;
            } else {
                // odd, right neighbor
                segmentRight += segmentSize*levelMultipler;
            }
            levelMultiplier *= 2;
        }
    }

    // calculate density of the segment
    // check thresholds, starting from the index value
    // return true if the segment is balanced
    // return false if the segment is not balanced, and set the segmentLeft, segmentRight
    bool checkThresholds(int index, int startLevel, uint16_t limit,  int &sLeft, int &sRight) {
        // let level 0 be the bottom level
        int level = 1;
        int levelMultipler = 1;
        // get initial offsets
        int segmentLeft = index - (index % segmentSize);
        int segmentRight = segmentLeft + segmentSize;

        DEBUG_PRINT << "Logical tree height: " << limit << std::endl;

        while(level <= limit) {
            uint64_t segmentPos = (index / (segmentSize * levelMultipler)) + 1;

            // calculate the density
            // TODO: don't need to go over the whole upper segment
            // only count the neighbor segment and sum up
            int segmentElements = 0;
            for (uint64_t i = segmentLeft; i < segmentRight; i++) {
                if (data[i]) {
                    segmentElements++;
                }
            }
            DEBUG_PRINT << "Segment elements: " << segmentElements << std::endl;
            DEBUG_PRINT << "Segment size: " << segmentSize*levelMultipler << std::endl;

            double density = static_cast<double>(segmentElements) / (segmentSize*levelMultipler);
            double upperThreshold = upperThresholdAtLevel(level);
            double lowerThreshold = lowerThresholdAtLevel(level);

            DEBUG_PRINT << "Level: " << level << ", Density: " << density << ", Segment Pos: " << segmentPos << ", Segment Left: " << segmentLeft << ", Segment Right: " << segmentRight << std::endl;
            DEBUG_PRINT << "Upper Threshold: " << upperThreshold << ", Lower Threshold: " << lowerThreshold << std::endl;

            // upper segment is violated? 
            if (level >= startLevel && density > upperThreshold) {
                DEBUG_PRINT << "Segment is violated" << std::endl;
                sLeft = segmentLeft;
                sRight = segmentRight;
                return false;
            }

            // TODO: lower segment is violated?

            // find the segment neighbor
            if (segmentPos % 2 == 0) {
                // even, left neighbor
                segmentLeft -= segmentSize*levelMultipler;
            } else {
                // odd, right neighbor
                segmentRight += segmentSize*levelMultipler;
            }
            // level increased, we're considering now the upper level
            level++;

            // this is necessary to expand the range of the segment
            levelMultipler *= 2;
        }

        return true;
    }

    void checkForRebalancing(int index) {
        bool isBalanced = false;
        int segmentLeft = 0;
        int segmentRight = 0;
        int startLevel = 1;
        int treeHeight = getTreeHeight();
        int limit = treeHeight; // test

        DEBUG_PRINT << "Checking for rebalancing" << std::endl;
        DEBUG_PRINT << "Tree height: " << treeHeight << std::endl;
        DEBUG_PRINT << "Limit: " << limit << std::endl;

        while (!isBalanced && startLevel <= treeHeight) {
            isBalanced = checkThresholds(index, startLevel, limit, segmentLeft, segmentRight);
            limit++;
            startLevel++;
            rebalance(segmentLeft, segmentRight);
        }

        // still not balanced, double the capacity
        // and do a full rebalance
        if (!isBalanced) {
            DEBUG_PRINT << "Doubling capacity and performing full rebalancing" << std::endl;
            capacity *= 2;
            data.resize(capacity, std::nullopt);
            rebalance(0, capacity - 1);
        }
    }

    uint64_t findGapWithinSegment(uint64_t left, uint64_t right) {
        for (uint64_t i = left; i < right; i++) {
            if (data[i] == std::nullopt) {
                return i;
            }
        }
        return UINT64_MAX;
    }

    uint64_t findGapAtLeftNeighbor(uint64_t segmentLeft, uint64_t segmentRight) {
        if (segmentLeft == 0) return UINT64_MAX;
        segmentRight = segmentLeft;
        segmentLeft = segmentRight - segmentSize;
        return findGapWithinSegment(segmentLeft, segmentRight);
    }
    
    uint64_t findGapAtRightNeighbor(uint64_t segmentLeft, uint64_t segmentRight) {
        if (segmentRight == capacity) return UINT64_MAX;
        segmentLeft = segmentRight;
        segmentRight = segmentLeft + segmentSize;
        return findGapWithinSegment(segmentLeft, segmentRight);
    }

    void insert(int value) {
        if (capacity == 0) return;
        bool fullRebalance = false;
        bool partialRebalance = false;

        DEBUG_PRINT << "Value to insert: " << value << std::endl;
        int mid = binarySearchPMA(value);

        // value already exists
        if (data[mid] && value == data[mid].value()) return;

        // at this point, 'mid' is the most important value here
        // meaning where we want to insert the value
        // if there is a gap, insert the value and that's it 
        if (data[mid] == std::nullopt) {
            DEBUG_PRINT << "Inserting value: " << value << " at index: " << mid << std::endl;
            data[mid] = value;
            return;
        } 

        // get the segment boundaries where mid lies
        // segmentLeft, segmentRight
        int segmentLeft = mid - (mid % segmentSize);
        int segmentRight = segmentLeft + segmentSize;
        uint64_t nearestGap = UINT64_MAX;

        DEBUG_PRINT << "Searching for nearest gap in the segment " << segmentLeft << ":" << segmentRight << std::endl;
        
        // linear search for a gap in this segment, define nearestGap
        // TODO: start from the offset closest to mid
        nearestGap = findGapWithinSegment(segmentLeft, segmentRight);

        // no gaps in this segment. Is there any other segment? if not, double the capacity
        if (nearestGap == UINT64_MAX) {
            // set this segment to partil rebalancing
            partialRebalance = true;
            bool neighborGapAvailable = false;

            DEBUG_PRINT << "No gaps found in the segment" << std::endl;

            // TODO: fix this repeated code. for now its ok for readability
            // try to find gap at the left neighbor
            nearestGap = findGapAtLeftNeighbor(segmentLeft, segmentRight);
            // not found, try the right neighbor
            if (nearestGap == UINT64_MAX) {
                nearestGap = findGapAtRightNeighbor(segmentLeft, segmentRight);
            }
            // try to check rebalancing
            if (nearestGap == UINT64_MAX) {
                checkForRebalancing(mid);
            }
            // now try the neighbors again
            nearestGap = findGapAtLeftNeighbor(segmentLeft, segmentRight);
            // not found, try the right neighbor
            if (nearestGap == UINT64_MAX) {
                nearestGap = findGapAtRightNeighbor(segmentLeft, segmentRight);
            }
            // nothing? double the capacity
            if (nearestGap == UINT64_MAX) {
                DEBUG_PRINT << "No gaps available, doubling the array" << std::endl;
                capacity *= 2;
                data.resize(capacity, std::nullopt);

                // data resized, set to trigger rebalance
                fullRebalance = true;

                // search for gaps again
                uint64_t nearestRight = mid;

                while (data[nearestRight]) nearestRight++;
                nearestGap = nearestRight;
            }
        }

        // if yes, find the nearest gap in the next segment that belongs to the same window. 
        // insert, check the threshold for the upper level
        // If upper level is violated, rebalance upper level + neighbor window.
        DEBUG_PRINT << "Neareast gap found at: " << nearestGap << std::endl;
        
        // 'mid' is where we want the element to be placed
        // if nearestGap is greater than mid, shift right
        // if nearestGap is less than mid, shift left
        if (nearestGap > mid) {
            DEBUG_PRINT << "Shifting right" << std::endl;
            // in case of shifting right, we adjust mid to the right because of
            // the binary search (the mid calculation is always rounded down)
            // but first adjust mid accordingly to where it should be placed
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
        data[mid] = value;
        if (fullRebalance) {
            rebalance(0, capacity - 1);
        } else if (partialRebalance) {
            rebalance(segmentLeft, segmentRight);
            checkForRebalancing(mid);
        }
    }
};

void fixedInsert(PackedMemoryArray& sv) {
    // 2 3 5 6 10 11 15 44 77 665 724 1554 1766 3151 4547 _ _ 3794
    // 2 3 5 6 10 11 15 44 77 665 724 1554 1766 3151 4547 _ _ 3794 6463 8949 9896 _ _ _

    sv.insert(2);
    sv.insert(3);
    sv.insert(5);
    sv.insert(6);
    sv.insert(10);
    sv.insert(11);
    sv.insert(15);
    sv.insert(44);
    sv.insert(77);
    sv.insert(665);
    sv.insert(724);
    sv.insert(1554);
    sv.insert(1766);
    sv.insert(3151);
    sv.insert(4547);
    sv.insert(3794);
    sv.print();
}

void distInsert(PackedMemoryArray& pma) {

    Timer t;
    t.start();

    std::random_device rd;
//    std::mt19937 eng(-1011927998);
    int seed = rd();
//    int seed = -1011927998;
    std::cout << "Seed: " << seed << std::endl;
    std::mt19937 eng(seed);
    std::uniform_int_distribution<> distr(0, 10000);

    int count = 0;
    while (count < 50) {
       // int num = distr(eng);
        pma.insert(count);
        pma.print();
        ++count;
    }

    double time_taken = t.stop();
    std::cout << "Head Inserts: " << time_taken/10000000.0 << std::endl;
    
}

int main() {
    PackedMemoryArray pma(6);

//    fixedInsert(pma);

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
    pma.print();

    pma.checkIfSorted();
 //   pma.printLevelsInformation();

    return 0;
}


// 2 3 5 6 10 11 15 44 77 1554 1766 3151 4547 _  _  _  _  _  _  _   _  _  _  _
// 0 1 2 3 4  5  6  7  8  9    10   11   12   13 14 15 16 17 18 19 20 21 22 23
//
//
