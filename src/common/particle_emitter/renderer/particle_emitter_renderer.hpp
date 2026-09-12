#pragma once

#include <set>
#include <memory>
#include "reflection/reflection.hpp"
#include "particle_emitter/particle_data.hpp"
#include "render_scene/render_scene.hpp"


class IParticleRendererInstance;
class IParticleRendererMaster : public rtti::MetaObject {
public:
    TYPE_ENABLE();

    virtual ~IParticleRendererMaster() {}

    virtual void init() = 0;

    virtual std::unique_ptr<IParticleRendererInstance> _createInstance() = 0;
};

class IParticleRendererInstance {
public:
    virtual ~IParticleRendererInstance() {}

    virtual void init(ptclParticleData* pd) = 0;
    virtual void onParticlesSpawned(ptclParticleData* pd, int begin, int end) {}
    virtual void onParticleMemMove(ptclParticleData* pd, int from, int to) {}
    virtual void onParticleDespawn(ptclParticleData* pd, int i) {}
    virtual void update(const ParticleEmitterParams* params, ptclParticleData* pd, float dt) {}

    virtual void onSpawn(scnRenderScene* scn) = 0;
    virtual void onDespawn(scnRenderScene* scn) = 0;
};


template<typename INSTANCE_T>
class IParticleRendererMasterT : public IParticleRendererMaster {
    std::set<IParticleRendererInstance*> instances;
public:
    TYPE_ENABLE();
    ~IParticleRendererMasterT() {}

    virtual void onInstanceCreated(INSTANCE_T*) const = 0;

    std::unique_ptr<IParticleRendererInstance> _createInstance() override {
        return createInstance();
    }

    void _unregisterInstance(IParticleRendererInstance* inst) {
        instances.erase(inst);
    }

    std::unique_ptr<INSTANCE_T> createInstance() {
        auto inst = new INSTANCE_T;
        inst->_setMaster((typename INSTANCE_T::master_t*)this);
        instances.insert(inst);
        onInstanceCreated(inst);
        return std::unique_ptr<INSTANCE_T>(inst);
    }
};

template<typename MASTER_T>
class IParticleRendererInstanceT : public IParticleRendererInstance {
    MASTER_T* master = 0;
public:
    using master_t = MASTER_T;

    ~IParticleRendererInstanceT() {
        if (master) {
            master->_unregisterInstance(this);
        }
    }

    void _setMaster(MASTER_T* m) { master = m; }

    const MASTER_T* getMaster() const { return master; }
    MASTER_T* getMaster() { return master; }
};
