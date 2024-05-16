#include <iostream>
#include <vector>

class PackedMemoryArray {
    void print();

    void insert(int value) {
        // Binary search the element e_pred that is less than e_new
        // if empty, insert at the beginning
        // element found, search for the next empty slot going left and right

    }

    void find(int value) {}

    public:
    PackedMemoryArray();

    private:
    std::vector<int> data;
};

int main() {
}
