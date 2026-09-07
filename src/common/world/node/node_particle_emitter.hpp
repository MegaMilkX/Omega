#pragma once

#include "node_particle_emitter.auto.hpp"
#include "world/world.hpp"
#include "particle_emitter/particle_simulation.hpp"

#include "resource/resource.hpp"
#include "particle_emitter/particle_emitter_master.hpp"
#include "particle_emitter/particle_impl.hpp"


[[cppi_class]];
class ParticleEmitterNode : public TActorNode<ParticleSimulation> {
    //ParticleSimulation* sim = 0;
    ResourceRef<ParticleEmitter> emitter;
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

    [[cppi_decl, set("emitter")]]
    void setEmitter(const ResourceRef<ParticleEmitter>& e) {
        /*if (emitter_inst && sim) {
            sim->release(emitter_inst);
        }*/
        emitter = e;
        requestRebuild();
    }
    [[cppi_decl, get("emitter")]]
    ResourceRef<ParticleEmitter> getEmitter() const { return emitter; }

    [[cppi_decl, set("alive")]]
    void setAlive(bool a) {
        if (!emitter_inst) {
            return;
        }
        emitter_inst->is_alive = a;
    }
    [[cppi_decl, get("alive")]]
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
    void onReady() override {

    }
    void onSpawnActorNode(ParticleSimulation* sim) override {
        if (emitter_inst) {
            sim->release(emitter_inst);
            emitter_inst = nullptr;
        }
        if (!emitter) {
            return;
        }
        emitter_inst = sim->acquire(emitter);
        emitter_inst->setWorldTransform(getWorldTransform(), true);
    }
    void onDespawnActorNode(ParticleSimulation* sim) override {
        if (!emitter_inst) {
            return;
        }
        sim->release(emitter_inst);
        emitter_inst = nullptr;
    }
};

