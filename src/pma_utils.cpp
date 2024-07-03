#include "pma_btree.hpp"

namespace pma {

size_t hyperceil(size_t value) {
    return (size_t) pow(2, ceil(log2(static_cast<double>(value))));
}

}
