#pragma once

#include <memory>
#include "particle_emitter/particle_emitter_master.hpp"
#include "particle_emitter/particle_impl.hpp"


class RuntimeWorld;
class ParticleSimulation {
    struct Pool {
        std::set<int> free_slots;
        std::vector<std::unique_ptr<ParticleEmitterInstance>> instances;

        Pool() = default;
        Pool(const Pool&) = delete;
        Pool& operator=(const Pool&) = delete;
        Pool(Pool&&) = default;
        Pool& operator=(Pool&&) = default;
    };

    RuntimeWorld* world = 0;
    std::unordered_map<uint32_t, Pool> pools; // key is resource entry id

    std::set<ParticleEmitterInstance*> active_instances;
    std::set<ParticleEmitterInstance*> passive_instances;

    void free_(ParticleEmitterInstance* inst);

public:
    ParticleSimulation();
    ParticleSimulation(RuntimeWorld* world);
    ParticleSimulation(ParticleSimulation&) = delete;
    ~ParticleSimulation();

    ParticleEmitterInstance*    acquire(ResourceRef<ParticleEmitter> em);
    void                        release(ParticleEmitterInstance* inst);

    void update(float dt);
};