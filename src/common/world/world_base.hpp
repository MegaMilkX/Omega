#pragma once

#include <memory>
#include <set>
#include "world_system_registry.hpp"
#include "spawnable/spawnable.hpp"


class IScene;
class IWorld : public WorldSystemRegistry {
    IScene* active_scene = nullptr;

    std::set<ISpawnable*> spawned;
    std::set<ISpawnable*> deferred_despawns;
public:
    ~IWorld() {
        // Clear systems first since they can't be considered valid at this point
        clearSystems();
        for (auto& s : spawned) {
            despawn(s);
        }
    }
    
    virtual void update(float dt) = 0;

    void attachScene(IScene* scene);
    void detachScene(IScene* scene);

    void beginFrame() {
        for (auto s : deferred_despawns) {
            despawn(s);
        }
        deferred_despawns.clear();
    }
    
    void spawn(ISpawnable* s) {
        s->world = this;
        spawned.insert(s);
        s->onSpawn(*this);
    }
    void despawn(ISpawnable* s) {
        s->onDespawn(*this);
        spawned.erase(s);
        s->world = nullptr;
    }
    void despawnDeferred(ISpawnable* s) {
        deferred_despawns.insert(s);
    }
};

