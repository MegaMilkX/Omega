#pragma once

#include "actor_controller.auto.hpp"
#include "reflection/reflection.hpp"
#include "game_messaging/game_messaging.hpp"
#include "world/world_system_registry.hpp"

class ActorDriver;
class ActorNode;

class IActorNodeView {
    friend ActorDriver;

    type t;
    std::string name;
    bool is_root;

protected:
    ActorNode* ptr = 0;

    void assign(ActorNode* node) {
        ptr = node;
    }
    void clear() {
        ptr = nullptr;
    }

public:
    IActorNodeView(type t, const std::string& name, bool is_root)
        : t(t), name(name), is_root(is_root) {}

    bool check(type t, const std::string& name, bool is_root) const {
        if (this->t != t) {
            return false;
        }
        if (this->name != name) {
            return false;
        }
        if (this->is_root && !is_root) {
            return false;
        }
        return true;
    }
};

template<typename NODE_T>
class ActorNodeViewProxy : public IActorNodeView {


public:
    ActorNodeViewProxy(const std::string& name, bool is_root)
        : IActorNodeView(type_get<NODE_T>(), name, is_root) {}

    bool isValid() const {
        return ptr != nullptr;
    }

    NODE_T* operator->() {
        return (NODE_T*)ptr;
    }
    const NODE_T* operator->() const {
        return (const NODE_T*)ptr;
    }

    NODE_T* get() {
        return (NODE_T*)ptr;
    }
    const NODE_T* get() const {
        return (const NODE_T*)ptr;
    }
};

template<typename NODE_T>
class ActorNodeView {
    ActorNodeViewProxy<NODE_T>* prox = 0;
public:
    ActorNodeView() {}
    ActorNodeView(ActorNodeViewProxy<NODE_T>* prox)
        : prox(prox) {}
    ActorNodeView(const ActorNodeView<NODE_T>& other)
        : prox(prox) {}

    bool isValid() const { return prox && prox->isValid(); }

    NODE_T* operator->() {
        if(!prox) return nullptr;
        return prox->get();
    }
    const NODE_T* operator->() const {
        if(!prox) return nullptr;
        return prox->get();
    }
};

enum EXEC_PRIORITY {
    EXEC_PRIORITY_FIRST = 0,
    EXEC_PRIORITY_PRE_ANIMATION = 100,
    EXEC_PRIORITY_POST_ANIMATION = 200,
    EXEC_PRIORITY_PRE_COLLISION = 300,
    EXEC_PRIORITY_POST_COLLISION = 301,
    EXEC_PRIORITY_CAMERA = 400,
    EXEC_PRIORITY_LAST
};

class ActorNode;
class Actor;

[[cppi_class]];
class ActorDriver : public MetaObject {
    friend Actor;

    Actor* owner = 0;

    std::vector<std::unique_ptr<IActorNodeView>> node_views;
public:
    TYPE_ENABLE();

    ActorDriver() {}
    ActorDriver(const ActorDriver&) = delete;
    ActorDriver& operator=(const ActorDriver&) = delete;
    virtual ~ActorDriver() {}
    virtual int getExecutionPriority() const { return 0; }

    Actor* getOwner() { return owner; }

    virtual void onReset() = 0;
    virtual void onSpawnActorDriver(WorldSystemRegistry& reg, Actor* actor) = 0;
    virtual void onDespawnActorDriver(WorldSystemRegistry& reg, Actor* actor) = 0;
    virtual GAME_MESSAGE onMessage(GAME_MESSAGE msg) { return GAME_MSG::NOT_HANDLED; }
    virtual void onUpdate(float dt) = 0;

    // TODO: Maybe should not be overridable, but that would force ActorNodeView use
    virtual void onActorNodeRegister(type t, ActorNode* component, const std::string& name);
    virtual void onActorNodeUnregister(type t, ActorNode* component, const std::string& name);

    template<typename NODE_T>
    ActorNodeView<NODE_T> registerNodeView(const std::string& name, bool require_root = false) {
        ActorNodeViewProxy<NODE_T>* nv = new ActorNodeViewProxy<NODE_T>(name, require_root);
        node_views.push_back(std::unique_ptr<IActorNodeView>(nv));
        return ActorNodeView<NODE_T>(nv);
    }
};
