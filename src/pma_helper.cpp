#include "pma_index.hpp"

namespace pma {

bool PackedMemoryArray::isSorted() {
    int64_t previousData = -1;
    for (uint64_t i = 0; i < capacity - 1; i++) {
        if (keyValues[i].key != -1) {
            if (keyValues[i].key < previousData) {
//                std::cout << "=-=================================================================\n";
 //               std::cout << "Error at index: " << i << ", data: " << data[i]->first << ", previous: " << previousData << std::endl;
                return false;
            }
            previousData = keyValues[i].key;
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
    for (uint64_t i = 0; i < indexKeys.size(); i++) {
        std::cout << indexKeys[i] << "   ";
    }
    std::cout << std::endl;
}


void PackedMemoryArray::print(int segmentSize, bool printIndex) {
    for (uint64_t i = 0; i < capacity; i++) {
        if (i > 0 && segmentSize > 0 && i % segmentSize == 0) {
            std::cout << " | ";
        }
        if (keyValues[i].key != -1) {
            if (printIndex) {
                std::cout << i << ": ";
            }
            std::cout << keyValues[i].key << " (" << keyValues[i].value << ") ";
        } else {
            std::cout << "_ ";
        }
    }
    std::cout << std::endl;
}

} // namespace pma
