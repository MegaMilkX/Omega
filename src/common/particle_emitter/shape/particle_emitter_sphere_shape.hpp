#pragma once

#include <random>
#include "particle_emitter/shape/particle_emitter_shape.hpp"


class SphereParticleEmitterShape : public IParticleEmitterShape {
public:
    TYPE_ENABLE();
private:
    std::random_device m_seed;
    std::mt19937_64 mt_gen;
    std::uniform_real_distribution<float> u01;

    void emitSome(ptclParticleData* pd, int count) override {
        for (int i = 0; i < count; ++i) {
            int pti = pd->emitOne();

            float u = (u01(mt_gen) + 1.0f) * 0.5f;
            float x1 = u01(mt_gen);
            float x2 = u01(mt_gen);
            float x3 = u01(mt_gen);
            float mag = gfxm::sqrt(x1 * x1 + x2 * x2 + x3 * x3);
            x1 /= mag; x2 /= mag; x3 /= mag;
            float c = .0f;
            switch (emit_mode) {
            case EMIT_MODE::VOLUME:
                c = std::cbrt(u);
                break;
            case EMIT_MODE::SHELL:
                c = 1.0f;
                break;
            }            

            auto& inst = pd->instances[pti];
            auto& lcl_pos = pd->local_positions[pti];
            lcl_pos 
                = gfxm::vec4(
                    (u01(mt_gen) + 1.f) * .5f, (u01(mt_gen) + 1.f) * .5f, 
                    u01(mt_gen), u01(mt_gen)
                );
            gfxm::vec3 p(x1 * c, x2 * c, x3 * c);
            inst.pos = gfxm::vec4(
                p * radius, .0f
            );

            gfxm::vec3 velo = gfxm::normalize(p);
            pd->particleStates[pti].velocity = velo;

            inst.scale.w = .0f;
        }
    }

    void advanceMovement(float dt, ptclParticleData* pd, float max_lifetime) override {
        for (int i = 0; i < pd->aliveCount(); ++i) {
            auto& inst = pd->instances[i];
            auto& lcl_pos = pd->local_positions[i];

            lcl_pos.x += .2f * lcl_pos.z * dt;
            lcl_pos.y += .2f * lcl_pos.w * dt;
            lcl_pos.x = gfxm::fract(lcl_pos.x);
            lcl_pos.y = gfxm::fract(lcl_pos.y);
            
            float t0 = lcl_pos.x;
            float t1 = lcl_pos.y;

            float x = radius * cosf(t0 * gfxm::pi * 2.f) * sinf(t1 * gfxm::pi * 2.f);
            float y = radius * cosf(t1 * gfxm::pi * 2.f);
            float z = radius * sinf(t0 * gfxm::pi * 2.f) * sinf(t1 * gfxm::pi * 2.f);
            gfxm::vec4 position = gfxm::vec4(
                x, y, z, .0f
            );

            inst.pos = gfxm::vec4(
                position,
                inst.pos.w
            );
        }
    }
public:
    EMIT_MODE emit_mode = EMIT_MODE::VOLUME;
    float radius = 0.5f;

    SphereParticleEmitterShape()
        : mt_gen(m_seed()), u01(-1.0f, 1.f) {}
};