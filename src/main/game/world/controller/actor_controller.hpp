#pragma once

#include "reflection/reflection.hpp"

enum EXEC_PRIORITY {
    EXEC_PRIORITY_FIRST = 0,
    EXEC_PRIORITY_PRE_ANIMATION = 100,
    EXEC_PRIORITY_POST_ANIMATION = 200,
    EXEC_PRIORITY_PRE_COLLISION = 300,
    EXEC_PRIORITY_POST_COLLISION = 400,
    EXEC_PRIORITY_LAST
};

class gameActorNode;
class gameActor;
class gameWorld;
class ActorController {
    friend gameWorld;
    friend gameActor;

    gameActor* owner = 0;

    virtual int getExecutionPriority() const { return 0; }
public:
    virtual ~ActorController() {}

    gameActor* getOwner() { return owner; }

    virtual void onReset() = 0;
    virtual void onSpawn(gameActor* actor) = 0;
    virtual void onDespawn(gameActor* actor) = 0;
    virtual void onActorNodeRegister(type t, gameActorNode* component, const std::string& name) = 0;
    virtual void onActorNodeUnregister(type t, gameActorNode* component, const std::string& name) = 0;
    virtual void onUpdate(gameWorld* world, float dt) = 0;
};

template<int PRIORITY>
class ActorControllerT : public ActorController {
    int getExecutionPriotity() const { return PRIORITY; }
};