#include "btreepma_v2.hpp"
#include "../../include/timer.hpp"

int main() {
    Timer t;
    pma::BTreePMA_v2 btree(8 /* index_b */,8 /* storage_b */);

    std::cout << "Initialized" << std::endl;

    t.start();
    for(int i = 0; i < 100; i++) {
        btree.insert(i, i*10);
    }

    double time_taken = t.stop();
    std::cout << "Head Inserts: " << time_taken/10000000.0 << std::endl;

//    btree.dump_info();
    btree.dump();

    return 0;
}

// [00] L 0x122e04580 N: 2
//      Keys: 0: 4
//     Values: 0: 0 #4, 1: 8 #6
//  
//  8 is the offset of segment 1
