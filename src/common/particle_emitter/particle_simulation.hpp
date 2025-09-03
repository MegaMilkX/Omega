#pragma once

#include "particle_emitter/particle_emitter_master.hpp"
#include "particle_emitter/particle_impl.hpp"


class RuntimeWorld;
class ParticleSimulation {
    struct Pool {
        std::set<int> free_slots;
        std::vector<ParticleEmitterInstance*> instances;
    };

    RuntimeWorld* world = 0;
    std::unordered_map<ParticleEmitterMaster*, Pool> pools;

    std::set<ParticleEmitterInstance*> active_instances;
    std::set<ParticleEmitterInstance*> passive_instances;

    void free_(ParticleEmitterInstance* inst);

public:
    ParticleSimulation();
    ParticleSimulation(RuntimeWorld* world);
    ~ParticleSimulation();

    ParticleEmitterInstance*    acquire(HSHARED<ParticleEmitterMaster> em);
    void                        release(ParticleEmitterInstance* inst);

    void update(float dt);
};