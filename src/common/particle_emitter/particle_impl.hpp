#pragma once

#include "particle_emitter_master.hpp"
#include "particle_emitter_instance.hpp"


void ptclSetTimeScale(float scale);

void ptclUpdateEmit(float dt, ParticleEmitterInstance* inst);
void ptclUpdate(float dt, ParticleEmitterInstance* inst);
