#include "pma_key_value.hpp"

namespace pma {

bool PackedMemoryArray::isSorted() {
    for (uint64_t i = 0; i < capacity - 1; i++) {
        if (data[i] && data[i + 1] && data[i]->first > data[i + 1]->first) {
            return false;
        }
    }
    return true;
}

void PackedMemoryArray::printStats() {
    std::cout << "Capacity: " << capacity << std::endl;
    std::cout << "Segment Size: " << segmentSize << std::endl;
    std::cout << "Total Elements: " << totalElements << std::endl;
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
    std::cout << std::endl << std::endl;
}

} // namespace pma
