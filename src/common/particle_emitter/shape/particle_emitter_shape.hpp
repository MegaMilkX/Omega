#pragma once

#include "particle_emitter/particle_data.hpp"


class ptclShape {
public:
    virtual ~ptclShape() {}
    virtual void emitSome(ptclParticleData* pd, int count) = 0;
};