#pragma once

#include <stdint.h>


constexpr int RENDER_MAX_PASSES = 16;

typedef uint64_t render_id;
struct RenderId {
    union {
        uint64_t key;
    };

    void clear() {
        key = 0;
    }
    void setTechnique(uint64_t tech) {
        const uint64_t mask = (1 << 4) - 1;
        uint64_t tech_masked = mask & tech;
        key |= tech_masked << 59;
    }
    void setMaterial(uint64_t material) {
        const uint64_t mask = (1 << 16) - 1;
        uint64_t masked = mask & material;
        key |= masked << 43;
    }
    void setPass(uint64_t pass) {
        const uint64_t mask = (1 << 4) - 1;
        uint64_t masked = mask & pass;
        key |= masked << 39;
    }

    uint64_t getTechnique() const {
        const uint64_t mask = (1 << 4) - 1;
        return (key >> 59) & mask;
    }
    uint64_t getMaterial() const {
        const uint64_t mask = (1 << 16) - 1;
        return (key >> 43) & mask;
    }
    uint64_t getPass() const {
        const uint64_t mask = (1 << 4) - 1;
        return (key >> 39) & mask;
    }
};