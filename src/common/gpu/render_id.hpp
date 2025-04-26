#pragma once

#include <stdint.h>


constexpr int RENDER_MAX_PASSES = 16;

typedef uint64_t render_id;
struct RenderId {
    union {
        uint64_t key;
        struct {
            uint32_t material;
            uint16_t pass;
        };
    };

    void clear() {
        key = 0;
    }
    void setMaterial(uint64_t material) {
        this->material = material;
        //const uint64_t mask = (1 << 16) - 1;
        //uint64_t masked = mask & material;
        //key |= masked << 43;
    }
    void setPass(uint64_t pass) {
        this->pass = pass;
        //const uint64_t mask = (1 << 4) - 1;
        //uint64_t masked = mask & pass;
        //key |= masked << 39;
    }

    uint64_t getMaterial() const {
        return material;
        //const uint64_t mask = (1 << 16) - 1;
        //return (key >> 43) & mask;
    }
    uint64_t getPass() const {
        return pass;
        //const uint64_t mask = (1 << 4) - 1;
        //return (key >> 39) & mask;
    }
};