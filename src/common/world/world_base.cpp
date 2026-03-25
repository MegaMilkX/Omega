#include "world_base.hpp"
#include "scene/scene.hpp"


void IWorld::attachScene(IScene* scene) {
    if (active_scene) {
        active_scene->onDespawnScene(*this);
    }
    active_scene = scene;
    if(active_scene) {
        active_scene->onSpawnScene(*this);
    }
}
void IWorld::detachScene(IScene* scene) {
    if (active_scene != scene) {
        return;
    }
    active_scene->onDespawnScene(*this);
    active_scene = nullptr;
}

