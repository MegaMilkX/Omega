#pragma once

#include "actor_controller.auto.hpp"
#include "reflection/reflection.hpp"
#include "game_messaging/game_messaging.hpp"

enum EXEC_PRIORITY {
    EXEC_PRIORITY_FIRST = 0,
    EXEC_PRIORITY_PRE_ANIMATION = 100,
    EXEC_PRIORITY_POST_ANIMATION = 200,
    EXEC_PRIORITY_PRE_COLLISION = 300,
    EXEC_PRIORITY_CAMERA = 400,
    EXEC_PRIORITY_LAST
};

class gameActorNode;
class Actor;
class RuntimeWorld;

[[cppi_class]];
class ActorController {
    friend RuntimeWorld;
    friend Actor;

    Actor* owner = 0;

public:
    TYPE_ENABLE();

    virtual ~ActorController() {}
    virtual int getExecutionPriority() const { return 0; }

    Actor* getOwner() { return owner; }

    virtual void onReset() = 0;
    virtual void onSpawn(Actor* actor) = 0;
    virtual void onDespawn(Actor* actor) = 0;
    virtual void onActorNodeRegister(type t, gameActorNode* component, const std::string& name) = 0;
    virtual void onActorNodeUnregister(type t, gameActorNode* component, const std::string& name) = 0;
    virtual GAME_MESSAGE onMessage(GAME_MESSAGE msg) { return GAME_MSG::NOT_HANDLED; }
    virtual void onUpdate(RuntimeWorld* world, float dt) = 0;
};
