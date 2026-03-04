#pragma once

#include <string>
#include "world/world_system_registry.hpp"


class IScene {
public:
    virtual ~IScene() {}

    virtual void onSpawnScene(WorldSystemRegistry& reg) = 0;
    virtual void onDespawnScene(WorldSystemRegistry& reg) = 0;
    virtual bool load(const std::string& path) { return true; }
};

