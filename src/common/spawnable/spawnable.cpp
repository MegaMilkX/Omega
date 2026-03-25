#include <assert.h>
#include "spawnable.hpp"
#include "world/world_base.hpp"


WorldSystemRegistry* ISpawnable::getRegistry() {
    return world;
}

void ISpawnable::tryDespawn() {
    if(!isSpawned()) return;
    despawn();
}
void ISpawnable::despawn() {
    assert(world);
    world->despawn(this);
}
void ISpawnable::despawnDeferred() {
    assert(world);
    world->despawnDeferred(this);
}

