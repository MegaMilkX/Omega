#pragma once

#include <stdint.h>


class TransformSystem {
    static uint32_t generation;
public:
    static void nextFrame();
    static uint32_t getGeneration();
};