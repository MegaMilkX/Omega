#pragma once

#include <string>
#include "world/world_base.hpp"


class IScene {
public:
    virtual ~IScene() {}

    virtual void onSpawnScene(IWorld& world) = 0;
    virtual void onDespawnScene(IWorld& world) = 0;
    virtual bool load(const std::string& path) { return true; }
};

