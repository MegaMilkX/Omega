#pragma once

#include "world/world.hpp"

#include "resource/resource.hpp"
#include "render_scene/render_object/scn_decal.hpp"


class nodeDecal : public gameActorNode {
    TYPE_ENABLE(gameActorNode);
    scnNode scn_node;
    scnDecal scn_decal;
public:
    void onDefault() override {
        scn_decal.setTexture(resGet<gpuTexture2d>("fire_circle.png"));
        scn_decal.setColor(gfxm::vec4(1, 1, 1, 1));
        scn_decal.setBoxSize(gfxm::vec3(2, 1, 2));
        scn_decal.setNode(&scn_node);
    }
    void onUpdateTransform() override {
        scn_node.local_transform = getWorldTransform();
        scn_node.world_transform = getWorldTransform();
    }
    void onUpdate(gameWorld* world, float dt) override {}
    void onSpawn(gameWorld* world) override {
        world->getRenderScene()->addRenderObject(&scn_decal);
    }
    void onDespawn(gameWorld* world) override {
        world->getRenderScene()->removeRenderObject(&scn_decal);
    }
};
STATIC_BLOCK{
    type_register<nodeDecal>("nodeDecal")
        .parent<gameActorNode>();
};
