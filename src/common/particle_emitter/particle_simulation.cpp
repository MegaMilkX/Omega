#include "particle_simulation.hpp"
#include "world/world.hpp"


void ParticleSimulation::free_(ParticleEmitterInstance* inst) {
    if (inst == nullptr) {
        assert(false);
        return;
    }

    auto it = pools.find(inst->master.entryId());
    if (it == pools.end()) {
        return;
    }

    auto& pool = it->second;
    int slot = -1;
    for (int i = 0; i < pool.instances.size(); ++i) {
        if (inst == pool.instances[i].get()) {
            slot = i;
            break;
        }
    }
    if (slot != -1) {
        pool.free_slots.insert(slot);
        pool.instances[slot]->despawn(world->getRenderScene());
    }
}


ParticleSimulation::ParticleSimulation()
: world(nullptr) {
    // TODO: Check if this actually needs a world pointer
}
ParticleSimulation::ParticleSimulation(RuntimeWorld* world)
: world(world) {

}

ParticleSimulation::~ParticleSimulation() {

}


ParticleEmitterInstance* ParticleSimulation::acquire(ResourceRef<ParticleEmitter> em) {
    if (!em) {
        return 0;
    }

    auto it = pools.find(em.entryId());
    if (it == pools.end()) {
        it = pools.try_emplace(em.entryId()).first;
    }

    auto& pool = it->second;

    if (!pool.free_slots.empty()) {
        int slot = *pool.free_slots.begin();
        pool.free_slots.erase(pool.free_slots.begin());

        auto inst = pool.instances[slot].get();
        active_instances.insert(inst);
        inst->spawn(world->getRenderScene());
        inst->is_alive = true;
        inst->softReset();
        return inst;        
    }

    ParticleEmitterInstance* inst = new ParticleEmitterInstance();
    inst->init(em);

    auto& uptr = pool.instances.emplace_back();
    uptr.reset(inst);

    active_instances.insert(inst);
    inst->spawn(world->getRenderScene());
    inst->softReset();
    return inst;
}
void ParticleSimulation::release(ParticleEmitterInstance* inst) {
    if (inst == nullptr) {
        return;
    }

    auto it = pools.find(inst->master.entryId());
    if (it == pools.end()) {
        return;
    }

    auto& pool = it->second;
    int slot = -1;
    for (int i = 0; i < pool.instances.size(); ++i) {
        if (inst == pool.instances[i].get()) {
            slot = i;
            break;
        }
    }

    auto instance = pool.instances[slot].get();
    if (slot != -1) {
        active_instances.erase(instance);
        passive_instances.insert(instance);
        instance->is_alive = false;
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

