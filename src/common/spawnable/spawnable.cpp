#include <assert.h>
#include "spawnable.hpp"
#include "world/world_system_registry.hpp"


void ISpawnable::tryDespawn() {
    if(!isSpawned()) return;
    despawn();
}
void ISpawnable::despawn() {
    assert(registry);
    registry->despawn(this);
}
void ISpawnable::despawnDeferred() {
    assert(registry);
    registry->despawnDeferred(this);
}

