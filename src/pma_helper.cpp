#include "pma_index.hpp"

namespace pma {

bool PackedMemoryArray::isSorted() {
    int64_t previousData = -1;
    for (uint64_t i = 0; i < capacity - 1; i++) {
        if (data[i]) {
            if (data[i]->first < previousData) {
//                std::cout << "=-=================================================================\n";
 //               std::cout << "Error at index: " << i << ", data: " << data[i]->first << ", previous: " << previousData << std::endl;
                return false;
            }
            previousData = data[i]->first;
        }
    }
    return true;
}

void PackedMemoryArray::printStats() {
    std::cout << "[Storage] PMA total capacity: " << capacity 
        << ", segment capacity: " << segmentSize
        << ", number of segments: " << capacity / segmentSize
        << ", height: " << getTreeHeight() << ", cardinality: " << totalElements << std::endl;
}

void PackedMemoryArray::printIndices() {
    std::cout << "Index keys: ";
    for (uint64_t i = 0; i < indexValues.size(); i++) {
        std::cout << indexKeys[i] << " => " << indexValues[i] << "   ";
    }
    std::cout << std::endl;
}


void PackedMemoryArray::print(int segmentSize, bool printIndex) {
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
    std::cout << std::endl;
}

} // namespace pma
