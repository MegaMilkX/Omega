#pragma once

#include "projectile.auto.hpp"
#include "world/actor.hpp"

#include "world/node/rigid_body_node.hpp"
#include "world/node/skeletal_model.hpp"


[[cppi_class]];
class ProjectileActor : public Actor, public ITickable {
public:
    TYPE_ENABLE();

    [[cppi_decl]] SkeletalModelNode2 model;
    [[cppi_decl]] float velocity = 1.f;
    [[cppi_decl]] float radius = .1f;

    ProjectileActor();

    void onTick(float dt);

    void onSpawn(WorldSystemRegistry& reg);
    void onDespawn(WorldSystemRegistry& reg);
};

