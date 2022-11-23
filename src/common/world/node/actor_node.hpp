#pragma once

#include <string>
#include "reflection/reflection.hpp"
#include "math/gfxm.hpp"
#include "world/controller/actor_controller.hpp"
#include "util/static_block.hpp"
#include "transform_node/transform_node.hpp"



class gameWorld;
class gameActor;
class gameActorNode {
public:
    TYPE_ENABLE_BASE();
private:
    friend gameWorld;
    friend gameActor;

    int world_container_index = -1;
    std::string name;
    /*
    gfxm::vec3 translation = gfxm::vec3(0,0,0);
    gfxm::quat rotation = gfxm::quat(0, 0, 0, 1);
    gfxm::mat4 world_transform = gfxm::mat4(1.0f);
    bool dirty_ = true;
    */
    Handle<TransformNode> transform;
    gameActorNode* parent = 0;
    std::vector<std::unique_ptr<gameActorNode>> children;
    /*
    inline void dirty() { 
        dirty_ = true;
        for (auto& c : children) {
            c->dirty();
        }
    }
    gfxm::mat4 calcLocalTransform() {
        return gfxm::translate(gfxm::mat4(1.0f), translation)
            * gfxm::to_mat4(rotation);
    }*/

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
    gameActorNode() {
        transform.acquire();
    }
    virtual ~gameActorNode() {
        transform.release();
    }

    bool isRoot() const { return parent == 0; }

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
    /*
    void translate(const gfxm::vec3& t) { translation += t; dirty(); }
    void rotate(const gfxm::quat& q) { rotation = q * rotation; dirty(); }
    void setTranslation(const gfxm::vec3& t) { translation = t; dirty(); }
    void setRotation(const gfxm::quat& q) { rotation = q; dirty(); }

    const gfxm::vec3& getTranslation() const { return translation; }
    const gfxm::quat& getRotation() const { return rotation; }

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
    }*/

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

    virtual void onDefault() {}
    virtual void onSpawn(gameWorld* world) = 0;
    virtual void onDespawn(gameWorld* world) = 0;
    virtual void onUpdateTransform() = 0;
    virtual void onUpdate(gameWorld* world, float dt) {}
    virtual void onDecay(gameWorld* world) {}
    virtual void onUpdateDecay(gameWorld* world, float dt) {}
    virtual bool hasDecayed() const { return true; }
};


class nodeEmpty : public gameActorNode {
    TYPE_ENABLE(gameActorNode);
public:
    void onDefault() override {}
    void onUpdateTransform() override {}
    void onUpdate(gameWorld* world, float dt) override {}
    void onSpawn(gameWorld* world) override {}
    void onDespawn(gameWorld* world) override {}
};
STATIC_BLOCK{
    type_register<nodeEmpty>("nodeEmpty")
        .parent<gameActorNode>();
};
