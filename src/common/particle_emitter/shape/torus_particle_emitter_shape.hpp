#pragma once


#include <random>
#include "particle_emitter/shape/particle_emitter_shape.hpp"


class TorusParticleEmitterShape : public IParticleEmitterShape {

    std::random_device m_seed;
    std::mt19937_64 mt_gen;
    std::uniform_real_distribution<float> u01;

    void emitSome(ptclParticleData* pd, int count) override {
        for (int i = 0; i < count; ++i) {
            int pti = pd->emitOne();

            float u = (u01(mt_gen) + 1.0f) * 0.5f;
            float c = .0f;
            switch (emit_mode) {
            case EMIT_MODE::VOLUME:
                c = std::cbrt(u);
                break;
            case EMIT_MODE::SHELL:
                c = 1.0f;
                break;
            }

            float t = u01(mt_gen);
            float x = cosf(t * gfxm::pi * 2.f);
            float z = sinf(t * gfxm::pi * 2.f);
            gfxm::vec4 position = gfxm::vec4(
                x * radius_major, .0f, z * radius_major, .0f
            );

            gfxm::vec3 minor_x = gfxm::vec3(x, .0f, z);
            gfxm::vec3 minor_y = gfxm::vec3(.0f, 1.f, .0f);
            float tminor = u01(mt_gen);
            float xminor = cosf(tminor * gfxm::pi * 2.f);
            float yminor = sinf(tminor * gfxm::pi * 2.f);
            gfxm::vec3 pos_minor 
                = (minor_x * xminor + gfxm::vec3(.0f, yminor, .0f)) * radius_minor * c
                + gfxm::vec3(position);

            position = gfxm::vec4(pos_minor, .0f);
            
            pd->particleLocalPos[pti] = gfxm::vec4(t, tminor, u01(mt_gen), u01(mt_gen));
            pd->particlePositions[pti] = position;
            gfxm::vec3 velo = gfxm::vec3(0, 0, 0);
            pd->particleStates[pti].velocity = velo;
            pd->particleScale[pti].w = .0f;
        }
    }

    void advanceMovement(float dt, ptclParticleData* pd, float max_lifetime) override {
        for (int i = 0; i < pd->aliveCount(); ++i) {
            pd->particleLocalPos[i].x += .2f * pd->particleLocalPos[i].z * dt;
            pd->particleLocalPos[i].y += .2f * pd->particleLocalPos[i].w * dt;
            pd->particleLocalPos[i].x = gfxm::fract(pd->particleLocalPos[i].x);
            pd->particleLocalPos[i].y = gfxm::fract(pd->particleLocalPos[i].y);

            float t = pd->particleLocalPos[i].x;

            float x = cosf(t * gfxm::pi * 2.f);
            float z = sinf(t * gfxm::pi * 2.f);
            gfxm::vec4 position = gfxm::vec4(
                x * radius_major, .0f, z * radius_major, .0f
            );

            gfxm::vec3 minor_x = gfxm::vec3(x, .0f, z);
            gfxm::vec3 minor_y = gfxm::vec3(.0f, 1.f, .0f);
            float tminor = pd->particleLocalPos[i].y;
            float xminor = cosf(tminor * gfxm::pi * 2.f);
            float yminor = sinf(tminor * gfxm::pi * 2.f);
            gfxm::vec3 pos_minor 
                = (minor_x * xminor + gfxm::vec3(.0f, yminor, .0f)) * radius_minor * 1.0f
                + gfxm::vec3(position);

            position = gfxm::vec4(pos_minor, .0f);

            pd->particlePositions[i] = gfxm::vec4(
                position,
                pd->particlePositions[i].w
            );
        }
    }
public:
    TYPE_ENABLE();
    EMIT_MODE emit_mode = EMIT_MODE::VOLUME;
    float radius_major = 1.f;
    float radius_minor = .1f;

    TorusParticleEmitterShape()
        : mt_gen(m_seed()), u01(-1.0f, 1.f) {}
};