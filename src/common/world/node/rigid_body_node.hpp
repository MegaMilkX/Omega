#pragma once

#include "rigid_body_node.auto.hpp"
#include "world/world.hpp"


[[cppi_class]];
class ConvexMeshRigidBodyNode : public ActorNode {
public:
    TYPE_ENABLE();
    CollisionConvexShape    shape;
    std::shared_ptr<CollisionConvexMesh> mesh;
    Collider	            collider;
    ConvexMeshRigidBodyNode()
        : mesh(mesh)
    {
        collider.setShape(&shape);
        collider.user_data.type = COLLIDER_USER_NODE;
        collider.user_data.user_ptr = this;
        
        collider.mass = 1.f;
        collider.friction = .6f;
        collider.collision_group = COLLISION_LAYER_DEFAULT;

        getTransformHandle()->addDirtyCallback([](void* ctx) {
            ConvexMeshRigidBodyNode* node = (ConvexMeshRigidBodyNode*)ctx;
            node->collider.markAsExternallyTransformed();
        }, this);
    }

    void setMesh(const std::shared_ptr<CollisionConvexMesh>& mesh) {
        this->mesh = mesh;
        shape.setMesh(mesh.get());
    }

    void onDefault() override {

    }
    void onUpdateTransform() override {
        collider.setPosition(getWorldTranslation());
        collider.setRotation(getWorldRotation());
    }
    void onUpdate(RuntimeWorld* world, float dt) override {}
    void onSpawn(RuntimeWorld* world) override {
        world->getCollisionWorld()->addCollider(&collider);
        collider.markAsExternallyTransformed();
    }
    void onDespawn(RuntimeWorld* world) override {
        world->getCollisionWorld()->removeCollider(&collider);
    }
};


[[cppi_class]];
class RigidBodyNode : public ActorNode {
public:
    TYPE_ENABLE();
    CollisionSphereShape    shape;
    Collider	            collider;
    RigidBodyNode() {
        collider.setShape(&shape);
        collider.user_data.type = COLLIDER_USER_NODE;
        collider.user_data.user_ptr = this;
        
        collider.mass = 1.f;
        collider.friction = .6f;
        collider.collision_group = COLLISION_LAYER_CHARACTER;
        
        getTransformHandle()->addDirtyCallback([](void* ctx) {
            RigidBodyNode* node = (RigidBodyNode*)ctx;
            node->collider.markAsExternallyTransformed();
        }, this);
    }
    void onDefault() override {

    }
    void onUpdateTransform() override {
        collider.setPosition(getWorldTranslation());
        collider.setRotation(getWorldRotation());
    }
    void onUpdate(RuntimeWorld* world, float dt) override {}
    void onSpawn(RuntimeWorld* world) override {
        world->getCollisionWorld()->addCollider(&collider);
        collider.markAsExternallyTransformed();
    }
    void onDespawn(RuntimeWorld* world) override {
        world->getCollisionWorld()->removeCollider(&collider);
    }
};
