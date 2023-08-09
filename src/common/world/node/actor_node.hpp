#pragma once

#include "actor_node.auto.hpp"

#include <string>
#include "reflection/reflection.hpp"
#include "math/gfxm.hpp"
#include "world/controller/actor_controller.hpp"
#include "util/static_block.hpp"
#include "transform_node/transform_node.hpp"



class GameWorld;
class Actor;
[[cppi_class]];
class gameActorNode {
public:
    TYPE_ENABLE_BASE();
private:
    friend GameWorld;
    friend Actor;

    int world_container_index = -1;
    std::string name;

    Handle<TransformNode> transform;
    gameActorNode* parent = 0;
    std::vector<std::unique_ptr<gameActorNode>> children;


    void _registerGraphWorld(GameWorld* world);
    void _unregisterGraphWorld(GameWorld* world);

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
    void _spawn(GameWorld* world) {
        onSpawn(world);
        for (auto& c : children) {
            c->_spawn(world);
        }
    }
    void _despawn(GameWorld* world) {
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
    void _update(GameWorld* world, float dt) {
        onUpdate(world, dt);
        for (auto& c : children) {
            c->_update(world, dt);
        }
    }
    void _decay(GameWorld* world) {
        onDecay(world);
        for (auto& c : children) {
            c->_decay(world);
        }
    }
    void _updateDecay(GameWorld* world, float dt) {
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
    gameActorNode() {
        transform.acquire();
    }
    virtual ~gameActorNode() {
        transform.release();
    }

    bool isRoot() const { return parent == 0; }

    const std::string& getName() const { return name; }

    template<typename NODE_T>
    void forEachNode(std::function<void(NODE_T*)> cb) {
        if (get_type() == type_get<NODE_T>()) {
            cb((NODE_T*)this);
        }
        for (auto& ch : children) {
            ch->forEachNode<NODE_T>(cb);
        }
    }

    Handle<TransformNode> getTransformHandle() { return transform; }
    void attachTransformTo(gameActorNode* parent) {
        transformNodeAttach(parent->transform, transform);
    }
    void restoreTransformParent() {
        if (parent) {
            transformNodeAttach(parent->transform, transform);
        } else {
            transformNodeAttach(Handle<TransformNode>(0), transform);
        }
    }

    void translate(float x, float y, float z) { transform->translate(gfxm::vec3(x, y, z)); }
    void translate(const gfxm::vec3& t) { transform->translate(t); }
    void rotate(float angle, const gfxm::vec3& axis) { transform->rotate(angle, axis); }
    void rotate(const gfxm::quat& q) { transform->rotate(q); }
    void setTranslation(float x, float y, float z) { transform->setTranslation(gfxm::vec3(x, y, z)); }
    void setTranslation(const gfxm::vec3& t) { transform->setTranslation(t); }
    void setRotation(const gfxm::quat& q) { transform->setRotation(q); }

    void lookAtDir(const gfxm::vec3& dir) {
        gfxm::mat3 orient(1.f);
        orient[2] = dir;
        orient[1] = gfxm::vec3(0, 1, 0);
        orient[0] = gfxm::cross(orient[1], orient[2]);
        orient[1] = gfxm::cross(orient[0], orient[2]);
        gfxm::quat tgt_rot = gfxm::to_quat(orient);
        setRotation(tgt_rot);
    }

    const gfxm::vec3& getTranslation() const { return transform->getTranslation(); }
    const gfxm::quat& getRotation() const { return transform->getRotation(); }

    gfxm::vec3 getWorldTranslation() { return transform->getWorldTranslation(); }
    gfxm::quat getWorldRotation() { return transform->getWorldRotation(); }

    gfxm::vec3 getWorldForward() { return transform->getWorldForward(); }
    gfxm::vec3 getWorldBack() { return transform->getWorldBack(); }
    gfxm::vec3 getWorldLeft() { return transform->getWorldLeft(); }
    gfxm::vec3 getWorldRight() { return transform->getWorldRight(); }
    gfxm::vec3 getWorldUp() { return transform->getWorldUp(); }
    gfxm::vec3 getWorldDown() { return transform->getWorldDown(); }

    gfxm::mat4 getLocalTransform() { return transform->getLocalTransform(); }
    const gfxm::mat4& getWorldTransform() { return transform->getWorldTransform(); }

    template<typename CHILD_T>
    CHILD_T* createChild(const char* name) {
        CHILD_T* child = new CHILD_T;
        child->name = name;
        child->parent = this;
        transformNodeAttach(transform, child->transform);
        children.push_back(std::unique_ptr<gameActorNode>(child));
        child->onDefault();
        return child;
    }

    int childCount() const {
        return children.size();
    }
    const gameActorNode* getChild(int i) const {
        return children[i].get();
    }

    virtual void onDefault() {}
    virtual void onSpawn(GameWorld* world) = 0;
    virtual void onDespawn(GameWorld* world) = 0;
    virtual void onUpdateTransform() = 0;
    virtual void onUpdate(GameWorld* world, float dt) {}
    virtual void onDecay(GameWorld* world) {}
    virtual void onUpdateDecay(GameWorld* world, float dt) {}
    virtual bool hasDecayed() const { return true; }
};


class EmptyNode : public gameActorNode {
    TYPE_ENABLE(gameActorNode);
public:
    void onDefault() override {}
    void onUpdateTransform() override {}
    void onUpdate(GameWorld* world, float dt) override {}
    void onSpawn(GameWorld* world) override {}
    void onDespawn(GameWorld* world) override {}
};
