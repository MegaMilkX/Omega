#pragma once

#include <stdint.h>
#include <vector>


template<typename T, int OBJECTS_PER_BLOCK>
class BLOCK_STORAGE {
    uint32_t element_count = 0;
    std::vector<unsigned char*> blocks;
    void add_block() {
        blocks.push_back(new unsigned char[OBJECTS_PER_BLOCK * sizeof(T)]);
    }
public:
    ~BLOCK_STORAGE() {
        for (int i = 0; i < blocks.size(); ++i) {
            delete[] blocks[i];
        }
    }
    T* deref(uint32_t index) {
        uint32_t lcl_id = index % OBJECTS_PER_BLOCK;
        uint32_t block_id = index / OBJECTS_PER_BLOCK;
        if (block_id >= blocks.size()) {
            return 0;
        }
        return (T*)&blocks[block_id][lcl_id * sizeof(T)];
    }
    // expand by 1 and return the last element pointer
    uint32_t add_one() {
        uint32_t index = element_count++;
        uint32_t lcl_id = index % OBJECTS_PER_BLOCK;
        uint32_t block_id = index / OBJECTS_PER_BLOCK;
        if (block_id == blocks.size()) {
            add_block();
        }
        return index;
    }
};

