#pragma once

#include <string>
#include "reflection/reflection.hpp"
#include "math/gfxm.hpp"
#include "game/world/controller/actor_controller.hpp"
#include "util/static_block.hpp"


class gameWorld;
class gameActor;
class gameActorNode {
public:
    TYPE_ENABLE_BASE();
private:
    friend gameWorld;
    friend gameActor;

    int world_container_index = -1;

    gfxm::vec3 translation = gfxm::vec3(0,0,0);
    gfxm::quat rotation = gfxm::quat(0, 0, 0, 1);
    gfxm::mat4 world_transform = gfxm::mat4(1.0f);
    bool dirty_ = true;
    std::string name;
    gameActorNode* parent = 0;
    std::vector<std::unique_ptr<gameActorNode>> children;

    inline void dirty() { 
        dirty_ = true;
        for (auto& c : children) {
            c->dirty();
        }
    }
    gfxm::mat4 calcLocalTransform() {
        return gfxm::translate(gfxm::mat4(1.0f), translation)
            * gfxm::to_mat4(rotation);
    }

    void _registerGraphWorld(gameWorld* world);
    void _unregisterGraphWorld(gameWorld* world);

    void _registerGraph(ActorController* controller) {
        controller->onActorNodeRegister(get_type(), this, name);
        for (auto& c : children) {
            c->_registerGraph(controller);
        }
    }
    void _unregisterGraph(ActorController* controller) {
        for (auto& c : children) {
            c->_unregisterGraph(controller);
        }
        controller->onActorNodeUnregister(get_type(), this, name);
    }
    void _spawn(gameWorld* world) {
        onSpawn(world);
        for (auto& c : children) {
            c->_spawn(world);
        }
    }
    void _despawn(gameWorld* world) {
        for (auto& c : children) {
            c->_despawn(world);
        }
        onDespawn(world);
    }
    void _updateTransform() {
        onUpdateTransform();
        for (auto& c : children) {
            c->_updateTransform();
        }
    }
    void _update(gameWorld* world, float dt) {
        onUpdate(world, dt);
        for (auto& c : children) {
            c->_update(world, dt);
        }
    }
    void _decay(gameWorld* world) {
        onDecay(world);
        for (auto& c : children) {
            c->_decay(world);
        }
    }
    void _updateDecay(gameWorld* world, float dt) {
        onUpdateDecay(world, dt);
        for (auto& c : children) {
            c->_updateDecay(world, dt);
        }
    }
    bool hasDecayed_actor() const { 
        if (!hasDecayed()) {
            return false;
        }
        for (auto& c : children) {
            if (!c->hasDecayed_actor()) {
                return false;
            }
        }
        return true;
    }
public:
    virtual ~gameActorNode() {}

    bool isRoot() const { return parent == 0; }

    void translate(const gfxm::vec3& t) { translation += t; dirty(); }
    void rotate(const gfxm::quat& q) { rotation = q * rotation; dirty(); }
    void setTranslation(const gfxm::vec3& t) { translation = t; dirty(); }
    void setRotation(const gfxm::quat& q) { rotation = q; dirty(); }

    gfxm::vec3 getWorldTranslation() { return getWorldTransform() * gfxm::vec4(0,0,0,1); }
    gfxm::quat getWorldRotation() { return gfxm::to_quat(gfxm::to_orient_mat3(getWorldTransform())); }

    gfxm::vec3 getForward() { return gfxm::normalize(getWorldTransform()[2]); }
    gfxm::vec3 getLeft() { return gfxm::normalize(-getWorldTransform()[0]); }

    const gfxm::mat4& getWorldTransform() {
        if (!dirty_) {
            return world_transform;
        } else {
            if (parent) {
                world_transform = parent->getWorldTransform() * calcLocalTransform();
            } else {
                world_transform = calcLocalTransform();
            }
            dirty_ = false;
            return world_transform;
        }
    }

    template<typename CHILD_T>
    CHILD_T* createChild(const char* name) {
        CHILD_T* child = new CHILD_T;
        child->name = name;
        child->parent = this;
        children.push_back(std::unique_ptr<gameActorNode>(child));
        child->onDefault();
        return child;
    }

    virtual void onDefault() {}
    virtual void onSpawn(gameWorld* world) = 0;
    virtual void onDespawn(gameWorld* world) = 0;
    virtual void onUpdateTransform() = 0;
    virtual void onUpdate(gameWorld* world, float dt) {}
    virtual void onDecay(gameWorld* world) {}
    virtual void onUpdateDecay(gameWorld* world, float dt) {}
    virtual bool hasDecayed() const { return true; }
};