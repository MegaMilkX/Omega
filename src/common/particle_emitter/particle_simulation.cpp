#include "particle_simulation.hpp"
#include "world/world.hpp"


void ParticleSimulation::free_(ParticleEmitterInstance* inst) {
    if (inst == nullptr) {
        assert(false);
        return;
    }

    auto it = pools.find(inst->master);
    if (it == pools.end()) {
        return;
    }

    auto& pool = it->second;
    int slot = -1;
    for (int i = 0; i < pool.instances.size(); ++i) {
        if (inst == pool.instances[i]) {
            slot = i;
            break;
        }
    }
    if (slot != -1) {
        pool.free_slots.insert(slot);
        pool.instances[slot]->despawn(world->getRenderScene());
    }
}


ParticleSimulation::ParticleSimulation(RuntimeWorld* world)
: world(world) {

}

ParticleSimulation::~ParticleSimulation() {

}


ParticleEmitterInstance* ParticleSimulation::acquire(HSHARED<ParticleEmitterMaster> em) {
    if (!em.isValid()) {
        return 0;
    }

    auto it = pools.find(em.get());
    if (it == pools.end()) {
        it = pools.insert(std::make_pair(em.get(), Pool())).first;
    }

    auto& pool = it->second;
    if (!pool.free_slots.empty()) {
        int slot = *pool.free_slots.begin();
        pool.free_slots.erase(pool.free_slots.begin());

        active_instances.insert(pool.instances[slot]);
        pool.instances[slot]->spawn(world->getRenderScene());
        pool.instances[slot]->is_alive = true;
        pool.instances[slot]->softReset();
        return pool.instances[slot];        
    }

    auto instance = em->createInstance();
    pool.instances.push_back(instance);
    active_instances.insert(instance);
    instance->spawn(world->getRenderScene());
    instance->softReset();
    return pool.instances.back();
}
void ParticleSimulation::release(ParticleEmitterInstance* inst) {
    if (inst == nullptr) {
        return;
    }

    auto it = pools.find(inst->master);
    if (it == pools.end()) {
        return;
    }

    auto& pool = it->second;
    int slot = -1;
    for (int i = 0; i < pool.instances.size(); ++i) {
        if (inst == pool.instances[i]) {
            slot = i;
            break;
        }
    }
    if (slot != -1) {
        active_instances.erase(pool.instances[slot]);
        passive_instances.insert(pool.instances[slot]);
        pool.instances[slot]->is_alive = false;
    }
}

void ParticleSimulation::update(float dt) {
    for (auto inst : active_instances) {
        ptclUpdateEmit(dt, inst);
    }
    for (auto inst : active_instances) {
        ptclUpdate(dt, inst);
    }
    for (auto inst : passive_instances) {
        ptclUpdate(dt, inst);
    }

    std::set<ParticleEmitterInstance*> to_remove;
    for (auto inst : passive_instances) {
        if (!inst->isAlive()) {
            to_remove.insert(inst);
        }
    }

    for (auto inst : to_remove) {
        free_(inst);
        passive_instances.erase(inst);
    }
}

