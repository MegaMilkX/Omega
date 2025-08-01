#pragma once

#include "particle_emitter/particle_data.hpp"
#include "reflection/reflection.hpp"
#include "util/static_block.hpp"


enum class EMIT_MODE {
    POINT,
    VOLUME,
    SHELL,
};

class IParticleEmitterShape {
public:
    TYPE_ENABLE();

    virtual ~IParticleEmitterShape() {}
    virtual void emitSome(ptclParticleData* pd, int count) = 0;
    virtual void advanceMovement(float dt, ptclParticleData* pd, float max_lifetime) = 0;
};