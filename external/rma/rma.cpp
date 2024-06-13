#include "btreepma_v2.hpp"
#include "../../timer.hpp"

int main() {
    Timer t;
    pma::BTreePMA_v2 btree(8,8);

    std::cout << "Initialized" << std::endl;

    t.start();
    for(int i = 0; i < 10000000; i++) {
        btree.insert(i, i);
    }

    double time_taken = t.stop();
    std::cout << "Head Inserts: " << time_taken/10000000.0 << std::endl;

//    btree.dump();

    return 0;
}
