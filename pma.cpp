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

class PackedMemoryArray {
public:
    std::vector<std::optional<int>> data;
    uint64_t segmentSize;
    uint64_t capacity;

    PackedMemoryArray(uint64_t segmentSize) : segmentSize(segmentSize), capacity(segmentSize) {
        data = std::vector<std::optional<int>>(segmentSize, std::nullopt);
 //       levels = std::vector<std::vector<segmentMetadata>>(1, std::vector<segmentMetadata>(1, segmentMetadata{segmentSize, 0}));
    }

    struct segmentMetadata {
        uint64_t gaps;
        uint64_t elements;
    };

    // segment metadata for each segment, divided by levels
//    std::vector<std::vector<segmentMetadata>> levels;

//    void traverseTreeFromValue(lambda operation) {} 

    void checkIfSorted() {
        for (uint64_t i = 0; i < capacity - 1; i++) {
            if (data[i] && data[i + 1] && data[i].value() > data[i + 1].value()) {
                std::cerr << "Array is not sorted" << std::endl;
                exit(1);
            }
        }
        std::cout << "Array is sorted" << std::endl;
    }

 //   void printLevelsInformation() {
 //       for (uint64_t i = 0; i < levels.size(); i++) {
 //           std::cout << "Level: " << i << std::endl;
 //           for (uint64_t j = 0; j < levels[i].size(); j++) {
 //               std::cout << "Segment: " << j << " Gaps: " << levels[i][j].gaps << " Elements: " << levels[i][j].elements << std::endl;
 //           }
 //       }
 //   }
    
    void print() {
        for (uint64_t i = 0; i < capacity; i++) {
            if (data[i].has_value()) {
                std::cout << data[i].value() << " ";
            } else {
                std::cout << "_ ";
            }
        }
        std::cout << std::endl;
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
        std::cout << "Rebalancing Step: " << step << std::endl;

        // clear the segment
        for (uint64_t i = left; i < right; i++) {
            data[i] = std::nullopt;
        }

        for (uint64_t i = 0; i < numElements; i++) {
            data[std::round(i * step) + left] = segmentElements[i];
        }
    }

    void insert(int value) {
        // guard against zero capacity
        if (capacity == 0) return;
        bool fullRebalance = false;

        uint64_t left = 0;
        uint64_t right = capacity - 1;
        uint64_t mid = 0;
        bool isEmpty = false;

        // binary search the value
        while (left <= right) {
            mid = left + (right - left) / 2;

            // value already exists, do nothing
            if (data[mid] && data[mid].value() == value) {
                std::cout << "Value already exists" << std::endl;
                return;
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
                    isEmpty = true;
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
                    isEmpty = true;
                    break;
                }

            }
        }

        // insert the value into the first available gap
        if (isEmpty || data[mid] == std::nullopt) {
            std::cout << "Inserting value: " << value << " at index: " << mid << std::endl;
            data[mid] = value;
            return;
        } 

        // no gaps, search for the nearest gap
        std::cerr << "No space to insert " << value << ", stopped at: " <<  mid << std::endl;
        int64_t nearestLeft = mid, nearestRight = mid;
        while (data[nearestLeft]) { 
            nearestLeft--;
            // no more left gaps available
            if (nearestLeft < 0) {
                nearestLeft = INT64_MIN;
                break;
            }
        }

        while (data[nearestRight]) {
            nearestRight++;
            // no more right gaps available
            if (nearestRight >= capacity) {
                nearestRight = INT64_MAX;
                break;
            }
        }

        // TODO: no gaps? double the array
        if (nearestLeft == INT64_MIN && nearestRight == INT64_MAX) {
            std::cerr << "No gaps available, doubling the array" << std::endl;
            capacity *= 2;
            data.resize(capacity, std::nullopt);

            // data resized, set to trigger rebalance
            fullRebalance = true;

            // search for gaps again
            nearestRight = mid;
            nearestLeft = INT64_MIN;

            // TODO: repeated code
            while (data[nearestRight]) {
                nearestRight++;
                // no more right gaps available
                if (nearestRight >= capacity) {
                    nearestRight = INT64_MAX;
                    break;
                }
            }
        }

        uint64_t nearestGap = (mid - nearestLeft) < (nearestRight - mid) ? nearestLeft : nearestRight;

        std::cout << "mid - nearestLeft: " << mid - nearestLeft << " nearestRight - mid: " << nearestRight - mid << std::endl;
        std::cout << "nearestLeft: " << nearestLeft << " nearestRight: " << nearestRight << std::endl;
        std::cout << "Neareast gap found at: " << nearestGap << std::endl;
        
        // 'mid' is where we want the element to be placed
        // if nearestGap is greater than mid, shift right
        // if nearestGap is less than mid, shift left
        if (nearestGap > mid) {
            std::cout << "Shifting right" << std::endl;
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
            std::cout << "Shifting left" << std::endl;

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
        }
        std::cout << std::endl;
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
    std::random_device rd;
//    std::mt19937 eng(-1011927998);
    int seed = rd();
//    int seed = -1011927998;
    std::cout << "Seed: " << seed << std::endl;
    std::mt19937 eng(seed);
    std::uniform_int_distribution<> distr(0, 10000);

    int count = 0;
    while (count < 10) {
        int num = distr(eng);
        pma.insert(num);
        pma.print();
        ++count;
    }
}

int main() {
    PackedMemoryArray pma(6);

//    fixedInsert(pma);

    pma.insert(5);
    pma.print();
    pma.insert(15);
    pma.print();
    pma.insert(10);
    pma.print();
    pma.insert(2);
    pma.print();
    pma.insert(3);
    pma.print();
    pma.insert(11);
    pma.print();
    pma.insert(44);
    pma.print();
    pma.insert(6);
    pma.print();
    pma.insert(77);
    pma.print();
    distInsert(pma);

    // pma.rebalance(0, pma.capacity - 1);
    pma.rebalance(0, 24);
    pma.print();

    pma.checkIfSorted();
 //   pma.printLevelsInformation();

    return 0;
}


// 2 3 5 6 10 11 15 44 77 1554 1766 3151 4547 _  _  _  _  _  _  _   _  _  _  _
// 0 1 2 3 4  5  6  7  8  9    10   11   12   13 14 15 16 17 18 19 20 21 22 23
//
