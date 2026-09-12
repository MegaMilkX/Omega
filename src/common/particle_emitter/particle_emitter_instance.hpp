#pragma once

#include "particle_data.hpp"
#include "particle_emitter_master.hpp"


class ParticleSimulation;
class ParticleEmitter;
class ParticleEmitterInstance {
    friend ParticleSimulation;
    friend ParticleEmitter;

    ResourceRef<ParticleEmitter> master;
    ParticleSimulation* simulation = 0;
    bool has_teleported = false;
public:
    ptclParticleData particle_data;
    std::vector<ptclComponent*> component_instances;
    std::vector<std::unique_ptr<IParticleRendererInstance>> renderer_instances;
    float cursor = .0f;
    float time_cache = .0f;
    bool is_alive = true;
    gfxm::mat4 world_transform_old = gfxm::mat4(1.0f);
    gfxm::mat4 world_transform = gfxm::mat4(1.0f);

    ParticleEmitterInstance() {}
    ~ParticleEmitterInstance();

    void init(const ResourceRef<ParticleEmitter>& em);

    const ParticleSimulation* getSimulation() const { return simulation; }
    ParticleSimulation* getSimulation() { return simulation; }

    const ParticleEmitter* getMaster() const { return master.get(); }
    ParticleEmitter* getMaster() { return master.get(); }

    void softReset() {
        is_alive = true;
        cursor = .0f;
        time_cache = .0f;
    }
    void reset() {
        particle_data.clear();
        is_alive = true;
        cursor = .0f;
        time_cache = .0f;
    }

    bool isAlive() const {
        return is_alive || particle_data.aliveCount() > 0;
    }

    bool hasTeleported() const { return has_teleported; }

    void setWorldTransform(const gfxm::mat4& w, bool has_teleported = false) {
        world_transform = w;
        this->has_teleported = has_teleported;
    }


    void spawn(scnRenderScene* scn) {
        for (auto& r : renderer_instances) {
            r->onSpawn(scn);
        }
    }
    void despawn(scnRenderScene* scn) {
        for (auto& r : renderer_instances) {
            r->onDespawn(scn);
        }
    }
};