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
            auto& inst = pd->instances[pti];

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
            
            pd->local_positions[pti] = gfxm::vec4(t, tminor, u01(mt_gen), u01(mt_gen));
            inst.pos = position;
            gfxm::vec3 velo = gfxm::vec3(0, 0, 0);
            pd->particleStates[pti].velocity = velo;
            inst.scale.w = .0f;
        }
    }

    void advanceMovement(float dt, ptclParticleData* pd, float max_lifetime) override {
        for (int i = 0; i < pd->aliveCount(); ++i) {
            auto& inst = pd->instances[i];

            pd->local_positions[i].x += .2f * pd->local_positions[i].z * dt;
            pd->local_positions[i].y += .2f * pd->local_positions[i].w * dt;
            pd->local_positions[i].x = gfxm::fract(pd->local_positions[i].x);
            pd->local_positions[i].y = gfxm::fract(pd->local_positions[i].y);

            float t = pd->local_positions[i].x;

            float x = cosf(t * gfxm::pi * 2.f);
            float z = sinf(t * gfxm::pi * 2.f);
            gfxm::vec4 position = gfxm::vec4(
                x * radius_major, .0f, z * radius_major, .0f
            );

            gfxm::vec3 minor_x = gfxm::vec3(x, .0f, z);
            gfxm::vec3 minor_y = gfxm::vec3(.0f, 1.f, .0f);
            float tminor = pd->local_positions[i].y;
            float xminor = cosf(tminor * gfxm::pi * 2.f);
            float yminor = sinf(tminor * gfxm::pi * 2.f);
            gfxm::vec3 pos_minor 
                = (minor_x * xminor + gfxm::vec3(.0f, yminor, .0f)) * radius_minor * 1.0f
                + gfxm::vec3(position);

            position = gfxm::vec4(pos_minor, .0f);

            inst.pos = gfxm::vec4(
                position,
                inst.pos.w
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