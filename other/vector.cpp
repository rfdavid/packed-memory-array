// PMA : 0.526067
// multimap: 0.415903
//
// Random: 0.237972
// multimap O3: 0.716106
#include "../include/timer.hpp"
#include <iostream>
#include <map>
#include <random>

int main() {
    Timer t;
    std::multimap<int64_t, int64_t> map;

    std::random_device rd;
    std::mt19937 eng(-1892682211);

    std::uniform_int_distribution<> distr(0, 1000000000);

    int total = 10000000;

    t.start();
    for (int count = 0; count < total; count++) {
        int num = distr(eng);
        map.insert(std::pair<int64_t, int64_t>(num, count*10));
    }
    double time_taken = t.stop();
    std::cout << "Head Inserts: " << time_taken/10000000.0 << std::endl;
}
