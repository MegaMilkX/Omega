#pragma once

#include "projectile.auto.hpp"
#include "world/actor.hpp"

#include "world/node/rigid_body_node.hpp"
#include "world/node/skeletal_model.hpp"


[[cppi_class]];
class ProjectileActor : public Actor {
public:
    TYPE_ENABLE();

    [[cppi_decl]] RigidBodyNode rigid_body;
    [[cppi_decl]] SkeletalModelNode2 model;
    [[cppi_decl]] float velocity = 1.f;
};

