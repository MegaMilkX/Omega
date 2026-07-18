#include "node_particle_emitter.hpp"


STATIC_BLOCK {
    rtti::type_register<ParticleEmitterNode>("ParticleEmitterNode")
        .parent<ActorNode>();
};