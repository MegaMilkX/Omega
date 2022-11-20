#pragma once

#include "particle_emitter/particle_data.hpp"


class ptclRenderer {
public:
    virtual ~ptclRenderer() {}

    virtual void init(ptclParticleData* pd) = 0;
    virtual void onParticlesSpawned(ptclParticleData* pd, int begin, int end) {}
    virtual void onParticleMemMove(ptclParticleData* pd, int from, int to) {}
    virtual void onParticleDespawn(ptclParticleData* pd, int i) {}
    virtual void update(ptclParticleData* pd, float dt) {}
    //virtual void draw(ptclParticleData* pd, float dt) = 0;

    virtual void onSpawn(scnRenderScene* scn) = 0;
    virtual void onDespawn(scnRenderScene* scn) = 0;
};
