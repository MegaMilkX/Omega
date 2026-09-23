#include "projectile.hpp"



ProjectileActor::ProjectileActor() {
    registerNode(nullptr, &model);
}

void ProjectileActor::onTick(float dt) {
    auto phy_world = getWorld()->getSystem<phyWorld>();

    gfxm::vec3 pos = getTranslation();
    gfxm::vec3 dir = -getForward();
    gfxm::vec3 V = dir * velocity * dt;
    
    auto ssr = phy_world->sphereSweep(pos, pos + V, radius, COLLISION_LAYER_DEFAULT);
    if (ssr.hasHit) {
        LOG_DBG("Hit");
        despawnDeferred();
    } else {
        translate(V);
    }
}

void ProjectileActor::onSpawn(WorldSystemRegistry& reg) {
    Actor::onSpawn(reg);
    
    if (auto sys = reg.getSystem<TickSystem>()) {
        sys->add(this);
    }
}

void ProjectileActor::onDespawn(WorldSystemRegistry& reg) {
    if (auto sys = reg.getSystem<TickSystem>()) {
        sys->remove(this);
    }

    Actor::onDespawn(reg);
}

