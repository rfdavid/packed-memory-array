#include <random>

#include "timer.hpp" // from reddragon
#include "pma_key_value.hpp"

void distInsert(pma::PackedMemoryArray& pma) {
    Timer t;

    std::random_device rd;
    std::mt19937 eng(-1011927998);
    int seed = rd();

    DEBUG_PRINT << "Seed: " << seed << std::endl;
    //std::mt19937 eng(seed);
    std::uniform_int_distribution<> distr(0, 100000000);

    t.start();
    for (int count = 0; count < 10000; count++) {
        //       int num = distr(eng);
        pma.insertElement(count, count*10);

        if (count % 10000 == 0) {
            std::cout << ".";
        }

        //pma.print(pma.segmentSize);
        //pma.insertElement(count);
        //        pma.print(true, count);
        //        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        //        pma.checkIfSorted();

    }
    double time_taken = t.stop();
    std::cout << "Head Inserts: " << time_taken/10000000.0 << std::endl;
}

void sumTest(pma::PackedMemoryArray& pma, int64_t min, int64_t max) {
    pma::SumResult result = pma.sum(min, max);
    std::cout << "First Key: " << result.m_first_key << std::endl;
    std::cout << "Last Key: " << result.m_last_key << std::endl;
    std::cout << "Sum Keys: " << result.m_sum_keys << std::endl;
    std::cout << "Sum Values: " << result.m_sum_values << std::endl;
    std::cout << "Num Elements: " << result.m_num_elements << std::endl << std::endl;
}

void runSumTest(pma::PackedMemoryArray& pma) {
    sumTest(pma, 0, 10); // 55
    sumTest(pma, 1, 10); // 55
    sumTest(pma, 2, 10); // 54
    sumTest(pma, 10, 15); // 75
    sumTest(pma, 98, 200); // 197
    sumTest(pma, 98, 200); // 197
    sumTest(pma, 28, 28); // 28
}


int main() {
    pma::PackedMemoryArray pma(8 /* initial capacity */);
    distInsert(pma);
//    pma.print(pma.segmentSize);

//    uint64_t index = pma.binarySearchPMA(12);
//    sumTest(pma, 28, 28); // 28
//
//    std::cout << "Index: " << index << std::endl;


    // pma.rebalance(0, pma.capacity - 1);
    pma.printStats();
//    pma.checkIfSorted();

    return 0;
}
