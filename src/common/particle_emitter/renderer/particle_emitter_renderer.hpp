#pragma once

#include <set>
#include "reflection/reflection.hpp"
#include "particle_emitter/particle_data.hpp"
#include "render_scene/render_scene.hpp"


class IParticleRendererInstance;
class IParticleRendererMaster {
public:
    TYPE_ENABLE();

    virtual ~IParticleRendererMaster() {}

    virtual void init() = 0;

    virtual IParticleRendererInstance* _createInstance() = 0;
    virtual void _destroyInstance(IParticleRendererInstance* inst) = 0;
};

class IParticleRendererInstance {
public:
    virtual ~IParticleRendererInstance() {}

    virtual void init(ptclParticleData* pd) = 0;
    virtual void onParticlesSpawned(ptclParticleData* pd, int begin, int end) {}
    virtual void onParticleMemMove(ptclParticleData* pd, int from, int to) {}
    virtual void onParticleDespawn(ptclParticleData* pd, int i) {}
    virtual void update(ptclParticleData* pd, float dt) {}

    virtual void onSpawn(scnRenderScene* scn) = 0;
    virtual void onDespawn(scnRenderScene* scn) = 0;
};


template<typename INSTANCE_T>
class IParticleRendererMasterT : public IParticleRendererMaster {

    std::set<INSTANCE_T*> instances;
public:
    TYPE_ENABLE();
    ~IParticleRendererMasterT() {
        for (auto inst : instances) {
            delete inst;
        }
        instances.clear();
    }

    virtual void onInstanceCreated(INSTANCE_T*) const = 0;

    IParticleRendererInstance* _createInstance() override {
        return createInstance();
    }
    void _destroyInstance(IParticleRendererInstance* inst) {
        destroyInstance((INSTANCE_T*)inst);
    }

    INSTANCE_T* createInstance() {
        auto inst = new INSTANCE_T;
        inst->_setMaster((INSTANCE_T::master_t*)this);
        instances.insert(inst);
        onInstanceCreated(inst);
        return inst;
    }
    void destroyInstance(INSTANCE_T* instance) {
        auto it = instances.find(instance);
        if (it == instances.end()) {
            assert(false);
            return;
        }
        INSTANCE_T* inst = (*it);
        instances.erase(inst);
        delete inst;
    }
};

template<typename MASTER_T>
class IParticleRendererInstanceT : public IParticleRendererInstance {
    MASTER_T* master = 0;
public:
    using master_t = MASTER_T;

    void _setMaster(MASTER_T* m) { master = m; }

    const MASTER_T* getMaster() const { return master; }
    MASTER_T* getMaster() { return master; }
};
