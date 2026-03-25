#pragma once

#include "log/log.hpp"


[[cppi_decl]];
class ISpawnable;

class WorldSystemRegistry;
class IWorld;
class ISpawnable {
    friend IWorld;
    IWorld* world = nullptr;

public:
    virtual ~ISpawnable() {
        if (isSpawned()) {
            LOG_ERR("ISpawnable destroyed before despawning");
            assert(false && "ISpawnable destroyed before despawning");
        }
    }

    WorldSystemRegistry* getRegistry();

    virtual void onSpawn(WorldSystemRegistry& reg) = 0;
    virtual void onDespawn(WorldSystemRegistry& reg) = 0;

    void tryDespawn();
    void despawn();
    void despawnDeferred();

    bool isSpawned() const { return world != nullptr; }
};

