#include "transform_system.hpp"

#include <set>


uint32_t TransformSystem::generation = 0;
std::set<TransformCallback*> scheduled_callbacks;


void TransformSystem::nextFrame() {
    for (auto cb : scheduled_callbacks) {
        cb->call();
    }
    scheduled_callbacks.clear();

    ++generation;
}
uint32_t TransformSystem::getGeneration() {
    return generation;
}


void TransformSystem::scheduleCallback(TransformCallback* cb) {
    scheduled_callbacks.insert(cb);
}
void TransformSystem::cancelCallback(TransformCallback* cb) {
    if(!cb) return;
    scheduled_callbacks.erase(cb);
}