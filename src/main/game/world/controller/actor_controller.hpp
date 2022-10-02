#pragma once

#include "reflection/reflection.hpp"


class gameActorNode;
class gameActor;
class gameWorld;
class ActorController {
public:
    virtual ~ActorController() {}

    virtual void onReset() = 0;
    virtual void onSpawn(gameActor* actor) = 0;
    virtual void onDespawn(gameActor* actor) = 0;
    virtual void onActorNodeRegister(type t, gameActorNode* component, const std::string& name) = 0;
    virtual void onActorNodeUnregister(type t, gameActorNode* component, const std::string& name) = 0;
    virtual void onUpdate(gameWorld* world, gameActor* actor, float dt) = 0;
};