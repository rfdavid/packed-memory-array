/* Notes
 * - Paper Canonical Density Control mentions having 
 *   total size = n < m < 2n (n elements, m size of the array)
 *
 *
 */
#include <iostream>
#include <vector>
#include <optional>

class PackedMemoryArray {
    void print() {
        for (int i = 0; i < capacity; i++) {
            if (data[i].has_value()) {
                std::cout << data[i].value() << " ";
            } else {
                std::cout << "_ ";
            }
        }
        std::cout << std::endl;
    }

    void resize();
    void rebalance();

    void doubleCapacity();
    void halveCapacity();

    void insert(int value) {
        // empty? insert in the middle
        if (data.empty()) {
            data[capacity / 2] = value;
            return;
        }
        // binary search the closest element in data
    }

    // find the nearest non-gap element index
    int findNearestNonGap(int low, int high, int mid) {
        int left = mid, right = mid;
        while (left >= low && !data[left].has_value()) {
            left--;
        }
        while (right <= high && !data[right].has_value()) {
            right++;
        }
        // if no non-gap elements are found, return the original mid which is
        // nullopt
        if (left < low && right > high) {
            return mid;
        }
        // choose the closer non-gap element if both sides have valid elements
        if (left >= low && right <= high) {
            return (abs(mid - left) <= abs(mid - right)) ? left : right;
        }
        // Otherwise, return whichever side has a valid element
        return (left >= low) ? left : right;
    }

    // insert 2
    //   find 2
    //   [ . 1 x x 5 x x . . . . . ]

    // TODO: use template instead of fixed int
    int binarySearch(int value) {
        uint64_t left = 0;
        uint64_t right = capacity - 1;
        // [ . 1 x x 5 x x . . . . . ]
        while (left <= right) {
            uint64_t mid = ((right - left) / 2) + left;
            // it is a gap
            if (!data[mid].has_value()) {
                mid = findNearestNonGap(left, right, mid);
                // insert at the gap if no non-gap elements found
                // nullopt position was returned from findNearestNonGap
                if (!data[mid].has_value()) {
                    return mid;
                }
            }
            int midValue = data[mid].value();
            if (midValue == value) {
                return mid;
            } else if (data[mid] < value) {
                left = mid + 1;
            } else {
                right = mid - 1;
            }
        }
        return left;
    }

    public:
    PackedMemoryArray(int capacity, int segmentSize) : capacity(capacity), segmentSize(segmentSize) {}

    std::vector<std::optional<int>> data = std::vector<std::optional<int>>(1024, std::nullopt);

    uint64_t density(int level);
    uint64_t segmentSize;
    uint64_t capacity;

    double upperThresholdAtLevel(int level) {
        return th + (th - t1) * (height - level) / (height - 1);
    }

    double lowerThresholdAtLevel(int level) {
        return ph - (ph - p1) * (height - level) / (height - 1);
    }

    int height;

    // lower threshold at level 1
    double p1 = 0.5;
    // upper threshold at level 1
    double t1 = 1;
    // lower threshold at top level h
    double ph = 0.75;
    // upper threshold at top level h
    double th = 0.75;

};

// initialize, capacity = 24, segmentSize = 6
// [ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ ]
// |            |           |           |            |
// insert(5)
//   empty? insert in the middle
// [ _ _ _ _ _ _ _ _ _ _ _ 5 _ _ _ _ _ 11 _ _ _ _ _ _ ]
// |            |           |           |            |
//
// insert(10)
//   search the closest gap to insert
//   calculate the density after inserting an element
//   
//
// [ _ _ _ _ _ _ _ _ _ _ _ 5 10 _ _ _ _ _ _ _ _ _ _ _ ]
// |            |           |            |            |
//
// insert(1)
//   binary search for the largest element less than 1, insert
//   
//
//
//   double/halve capacity: 
//     - recalculate tree height
//     - update capacity
//

int main() {
    PackedMemoryArray pma(24, 6);
    pma.insert(5);
    pma.insert(15);
    pma.insert(10);
    pma.insert(2);
    pma.print();

    return 0;
}

