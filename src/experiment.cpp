#include <random>

#include "timer.hpp" // from reddragon
#include "pma_btree.hpp"

void distInsert(pma::PackedMemoryArray& pma, int total) {
    Timer t;

    std::random_device rd;
    std::mt19937 eng(-1892682211);
    int seed = rd();
//    std::mt19937 eng(seed);

    std::cout << "Seed: " << seed << std::endl;
    std::uniform_int_distribution<> distr(1, 100);

    t.start();
    for (int count = 0; count < total; count++) {
//        int num = distr(eng);
        pma.insertElement(count, count*10);

//        if (!pma.isSorted()) {
//            std::cout << "Not Sorted" << std::endl;
//            std::cout << "Index: " << count << std::endl;
//            std::cout << "Seed: " << seed << std::endl;
//            exit(1);
//        }
    }
    double time_taken = t.stop();
    std::cout << "Head Inserts: " << time_taken/10000000.0 << std::endl;
}

// void sumTest(pma::PackedMemoryArray& pma, int64_t min, int64_t max) {
//     pma::SumResult result = pma.sum(min, max);
//     std::cout << "First Key: " << result.m_first_key << std::endl;
//     std::cout << "Last Key: " << result.m_last_key << std::endl;
//     std::cout << "Sum Keys: " << result.m_sum_keys << std::endl;
//     std::cout << "Sum Values: " << result.m_sum_values << std::endl;
//     std::cout << "Num Elements: " << result.m_num_elements << std::endl << std::endl;
// }
// 
// void runSumTest(pma::PackedMemoryArray& pma) {
//     sumTest(pma, 0, 10); // 55
//     sumTest(pma, 1, 10); // 55
//     sumTest(pma, 2, 10); // 54
//     sumTest(pma, 10, 15); // 75
//     sumTest(pma, 98, 200); // 197
//     sumTest(pma, 98, 200); // 197
//     sumTest(pma, 28, 28); // 28
// }
// 

int main() {
    uint64_t mid = 0;
    pma::PackedMemoryArray pma(4 /* initial capacity */);

//    pma.insertElement(8,70);
//
//    pma.insertElement(6,60, 3);
//    pma.insertElement(8,80, 4);
//    pma.insertElement(3,60);
//    pma.insertElement(4,60);
//    pma.insertElement(11,60);
//    pma.insertElement(10,60);
//
//    pma.insertElement(9,60);
//    pma.insertElement(19,60);
//
//    pma.insertElement(0,60);
//    pma.insertElement(1,60);
//
//    pma.insertElement(18,60);
//    pma.insertElement(999,60);

    distInsert(pma, 10000000);
//    pma.dumpValues();

//    distInsert(pma, 12);
//    pma.dump();

//    for (int i = 200; i > 0; i--) {
//        pma.insertElement(i, i*10000);
//        pma.dump();
//    }

//    std::cout << "is sorted? " << ( pma.isSorted() ? "true" : "false" ) << std::endl;

//
//
//    pma.insertElement(50497,60);
//    pma.insertElement(92828,60);
//    pma.insertElement(92002,60);
//
//    pma.insertElement(83565,60);
//    pma.insertElement(76954,60);
//
//    pma.insertElement(37201,60);
//    pma.insertElement(42963,60);
//    pma.insertElement(28061,60);
//    pma.insertElement(30981,60);

//    distInsert(pma, 10);
//    pma.print(pma.segmentSize);
//    pma.printIndices();


//
//
//    distInsert(pma, 1000000);
//    distInsert(pma, 10000);



//   pma.insertElement(6,60);
//   pma.insertElement(4,40);

//
//    pma.insertElement(2,20);
//    pma.print(pma.segmentSize);
//    pma.printIndices();
//
//    pma.insertElement(4,40);
//    pma.print(pma.segmentSize);
//    pma.printIndices();
//
//    pma.insertElement(22,220);
//    pma.print(pma.segmentSize);
//    pma.printIndices();
//
//    pma.insertElement(0,0);
//    pma.print(pma.segmentSize);
//    pma.printIndices();
//
//    pma.insertElement(3,30);
//    pma.print(pma.segmentSize);
//    pma.printIndices();
//
//    pma.insertElement(48,480);
//    pma.print(pma.segmentSize);
//    pma.printIndices();
//
//    pma.insertElement(60,600);
//    pma.print(pma.segmentSize);
//    pma.printIndices();
//
//    pma.insertElement(0,600);

//   pma.print(pma.segmentSize);


//    pma.insertElement(6,3);
//    pma.insertElement(8,4);
//    pma.insertElement(12,6);
//
//    pma.binarySearchPMA(3, &mid);
//    std::cout << "Find (3) : " << mid << std::endl;
//    pma.binarySearchPMA(5, &mid);
//    std::cout << "Find (5) : " << mid << std::endl;
//    pma.binarySearchPMA(11, &mid);
//    std::cout << "Find (11) : " << mid << std::endl;
//
//    distInsert(pma);
//    pma.print(pma.segmentSize);

//    uint64_t index = pma.binarySearchPMA(12);
//    sumTest(pma, 28, 28); // 28
//
//    std::cout << "Index: " << index << std::endl;


    // pma.rebalance(0, pma.capacity - 1);
//    pma.printStats();
//    pma.checkIfSorted();

    return 0;
}
