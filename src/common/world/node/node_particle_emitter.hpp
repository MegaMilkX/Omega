#pragma once

#include "world/world.hpp"

#include "resource/resource.hpp"
#include "particle_emitter/particle_emitter.hpp"


class nodeParticleEmitter : public gameActorNode {
    TYPE_ENABLE(gameActorNode);
public:
    ptclEmitter emitter;
    nodeParticleEmitter() {
        emitter.init();
    }
    void onDefault() override {
        auto shape = emitter.setShape<ptclSphereShape>();
        shape->radius = 0.05f;
        shape->emit_mode = ptclSphereShape::EMIT_MODE::VOLUME;
        emitter.addComponent<ptclAngularVelocityComponent>();

        curve<float> emit_curve;
        emit_curve[.0f] = 100.0f;
        emitter.setParticlePerSecondCurve(emit_curve);/*
        curve<float> scale_curve;
        scale_curve[.0f] = .1f;
        scale_curve[1.0f] = 1.2f;
        emitter.setScaleOverLifetimeCurve(scale_curve);
        curve<gfxm::vec4> rgba_curve;
        rgba_curve[.0f] = gfxm::vec4(1, 0.65f, 0, 1);
        rgba_curve[1.f] = gfxm::vec4(.1f, .1f, 0.1f, .0f);
        emitter.setRGBACurve(rgba_curve);*/
    }
    void onUpdateTransform() override {
        emitter.setWorldTransform(getWorldTransform());
    }
    void onUpdate(gameWorld* world, float dt) override {
        emitter.update_emit(dt);
        emitter.update(dt);
    }
    void onUpdateDecay(gameWorld* world, float dt) override {
        emitter.update(dt);
    }
    void onDecay(gameWorld* world) override {
        emitter.is_alive = false;
    }
    bool hasDecayed() const override {
        return !emitter.isAlive();
    }
    void onSpawn(gameWorld* world) override {
        emitter.reset();
        emitter.spawn(world->getRenderScene());
    }
    void onDespawn(gameWorld* world) override {
        emitter.despawn(world->getRenderScene());
    }
};
STATIC_BLOCK{
    type_register<nodeParticleEmitter>("nodeParticleEmitter")
        .parent<gameActorNode>();
};
