#pragma once

#include "node_particle_emitter.auto.hpp"
#include "world/world.hpp"
#include "particle_emitter/particle_simulation.hpp"

#include "resource/resource.hpp"
#include "particle_emitter/particle_emitter_master.hpp"
#include "particle_emitter/particle_impl.hpp"


[[cppi_class]];
class ParticleEmitterNode : public TActorNode<ParticleSimulation> {
    ParticleSimulation* sim = 0;
    RHSHARED<ParticleEmitterMaster> emitter;
    ParticleEmitterInstance* emitter_inst = 0;

public:
    TYPE_ENABLE();
    ParticleEmitterNode() {
        // TODO: AAAAAAAAAAAAAAAAAAAAAAA
        getTransformHandle()->addDirtyCallback([](void* ctx) {
            ParticleEmitterNode* node = (ParticleEmitterNode*)ctx;
            if(node->emitter_inst) {
                node->emitter_inst->setWorldTransform(node->getWorldTransform());
            }
        }, this);
    }
    ~ParticleEmitterNode() {

    }

    void setEmitter(const RHSHARED<ParticleEmitterMaster>& e) {
        if (emitter_inst && sim) {
            sim->release(emitter_inst);
        }
        emitter = e;
        //emitter_inst = emitter->createInstance();
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
        if(emitter_inst) {
            emitter_inst->setWorldTransform(getWorldTransform());
        }
    }
    void onUpdate(RuntimeWorld* world, float dt) override {
        //ptclUpdateEmit(dt, emitter_inst);
        //ptclUpdate(dt, emitter_inst);
    }
    void onUpdateDecay(RuntimeWorld* world, float dt) override {
        //ptclUpdate(dt, emitter_inst);
    }
    void onDecay(RuntimeWorld* world) override {
        if(emitter_inst) {
            emitter_inst->is_alive = false;
        }
    }
    bool hasDecayed() const override {
        if (!emitter_inst) {
            return false;
        }
        return !emitter_inst->isAlive();
    }
    void onSpawn(ParticleSimulation* sim) override {
        emitter_inst = sim->acquire(emitter);
        emitter_inst->setWorldTransform(getWorldTransform(), true);
    }
    void onDespawn(ParticleSimulation* sim) override {
        sim->release(emitter_inst);
        emitter_inst = 0;
    }
};

