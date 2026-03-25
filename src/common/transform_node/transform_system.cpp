#include "transform_system.hpp"


uint32_t TransformSystem::generation = 0;


void TransformSystem::nextFrame() {
    ++generation;
}
uint32_t TransformSystem::getGeneration() {
    return generation;
}

