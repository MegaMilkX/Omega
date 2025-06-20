#pragma once

#include "particle_data.hpp"


class ParticleSimulation;
class ParticleEmitterMaster;
class ParticleEmitterInstance {
    friend ParticleSimulation;
    friend ParticleEmitterMaster;

    ParticleEmitterMaster* master = 0;
    ParticleSimulation* simulation = 0;
    bool has_teleported = false;
public:
    ptclParticleData particle_data;
    std::vector<ptclComponent*> component_instances;
    std::vector<IParticleRendererInstance*> renderer_instances;
    float cursor = .0f;
    float time_cache = .0f;
    bool is_alive = true;
    gfxm::mat4 world_transform_old = gfxm::mat4(1.0f);
    gfxm::mat4 world_transform = gfxm::mat4(1.0f);

    ParticleEmitterInstance(int max_count) {
        particle_data.init(max_count);

    }

    const ParticleSimulation* getSimulation() const { return simulation; }
    ParticleSimulation* getSimulation() { return simulation; }

    const ParticleEmitterMaster* getMaster() const { return master; }
    ParticleEmitterMaster* getMaster() { return master; }

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