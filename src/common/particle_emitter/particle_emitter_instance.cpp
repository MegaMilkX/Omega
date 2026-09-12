#include "particle_emitter_instance.hpp"


ParticleEmitterInstance::~ParticleEmitterInstance() {
    if (master) {
        renderer_instances.clear();
        master->_unregisterInstance(this);
        // TODO: Destroy renderers
    }
}

void ParticleEmitterInstance::init(const ResourceRef<ParticleEmitter>& em) {
    if (master) {
        master->_unregisterInstance(this);
        particle_data.clear();

    }

    master = em;
    particle_data.init(em->params.max_count);

    renderer_instances.clear();
    for (auto& r : em->renderers) {
        auto renderer_instance = r->_createInstance();
        renderer_instance->init(&particle_data);
        renderer_instances.push_back(std::move(renderer_instance));
    }

    master->_registerInstance(this);
}

