#pragma once

#include "mdl_component_prototype.hpp"

#include "game/world/world.hpp"

#include "reflection/reflection.hpp"

class mdlModelInstance;
struct mdlComponentInstance {
    TYPE_ENABLE_BASE();

    virtual ~mdlComponentInstance() {}

    virtual void make(mdlModelInstance* owner, mdlComponentPrototype* p) = 0;

    virtual void onSpawn(wWorld* world) = 0;
    virtual void onDespawn(wWorld* world) = 0;

    virtual void reflect() = 0;
};