#include "node_particle_emitter.hpp"


STATIC_BLOCK {
    type_register<ParticleEmitterNode>("ParticleEmitterNode")
        .parent<gameActorNode>();
};