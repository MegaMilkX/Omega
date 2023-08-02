#pragma once

#include "world/world.hpp"

#include "resource/resource.hpp"
#include "particle_emitter/particle_emitter_master.hpp"
#include "particle_emitter/particle_impl.hpp"


class ParticleEmitterNode : public gameActorNode {
    TYPE_ENABLE(gameActorNode);
    RHSHARED<ParticleEmitterMaster> emitter;
    ParticleEmitterInstance* emitter_inst = 0;
public:
    ParticleEmitterNode() {
    }

    void setEmitter(const RHSHARED<ParticleEmitterMaster>& e) {
        if (emitter && emitter_inst) {
            emitter->destroyInstance(emitter_inst);
        }
        emitter = e;
        emitter_inst = emitter->createInstance();
    }
    void setAlive(bool a) {
        if (!emitter_inst) {
            return;
        }
        emitter_inst->is_alive = a;
    }
    bool isAlive() const {
        if (!emitter_inst) {
            return false;
        }
        return emitter_inst->isAlive();
    }

    void onDefault() override {
        /*
        auto shape = emitter.setShape<SphereParticleEmitterShape>();
        shape->radius = 0.05f;
        shape->emit_mode = EMIT_MODE::VOLUME;
        //emitter.addComponent<ptclAngularVelocityComponent>();

        curve<float> emit_curve;
        emit_curve[.0f] = 100.0f;
        emitter.setParticlePerSecondCurve(emit_curve);*/
    }
    void onUpdateTransform() override {
        emitter_inst->setWorldTransform(getWorldTransform());
    }
    void onUpdate(gameWorld* world, float dt) override {
        ptclUpdateEmit(dt, emitter_inst);
        ptclUpdate(dt, emitter_inst);
    }
    void onUpdateDecay(gameWorld* world, float dt) override {
        ptclUpdate(dt, emitter_inst);
    }
    void onDecay(gameWorld* world) override {
        emitter_inst->is_alive = false;
    }
    bool hasDecayed() const override {
        return !emitter_inst->isAlive();
    }
    void onSpawn(gameWorld* world) override {
        emitter_inst->reset();
        emitter_inst->spawn(world->getRenderScene());
    }
    void onDespawn(gameWorld* world) override {
        emitter_inst->despawn(world->getRenderScene());
    }
};
STATIC_BLOCK{
    type_register<ParticleEmitterNode>("ParticleEmitterNode")
        .parent<gameActorNode>();
};
