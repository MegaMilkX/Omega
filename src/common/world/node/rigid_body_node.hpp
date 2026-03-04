#pragma once

#include "rigid_body_node.auto.hpp"
#include "world/world.hpp"
#include "collision/collision_world.hpp"
#include "collision/shape/sphere.hpp"
#include "collision/shape/convex_mesh.hpp"


[[cppi_class]];
class ConvexMeshRigidBodyNode : public TActorNode<phyWorld> {
public:
    TYPE_ENABLE();
    phyConvexMeshShape    shape;
    std::shared_ptr<phyConvexMesh> mesh;
    phyRigidBody	            collider;
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

    void setMesh(const std::shared_ptr<phyConvexMesh>& mesh) {
        this->mesh = mesh;
        shape.setMesh(mesh.get());
    }

    void onDefault() override {

    }
    void onSpawnActorNode(phyWorld* world) override {
        world->addCollider(&collider);
        collider.markAsExternallyTransformed();
    }
    void onDespawnActorNode(phyWorld* world) override {
        world->removeCollider(&collider);
    }
};


[[cppi_class]];
class RigidBodyNode : public TActorNode<phyWorld> {
public:
    TYPE_ENABLE();
    phySphereShape    shape;
    phyRigidBody	            collider;
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
    void onSpawnActorNode(phyWorld* world) override {
        world->addCollider(&collider);
        collider.markAsExternallyTransformed();
    }
    void onDespawnActorNode(phyWorld* world) override {
        world->removeCollider(&collider);
    }
};

