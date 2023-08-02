#pragma once

#include <random>
#include "particle_emitter/shape/particle_emitter_shape.hpp"


class SphereParticleEmitterShape : public IParticleEmitterShape {
    TYPE_ENABLE(IParticleEmitterShape);
public:
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

            gfxm::vec3 p(x1 * c, x2 * c, x3 * c);
            pd->particlePositions[pti] = gfxm::vec4(
                p * radius, .0f
            );

            gfxm::vec3 velo = gfxm::normalize(p);
            pd->particleStates[pti].velocity = velo;

            pd->particleScale[pti].w = .0f;
        }
    }
public:
    EMIT_MODE emit_mode = EMIT_MODE::VOLUME;
    float radius = 0.5f;

    SphereParticleEmitterShape()
        : mt_gen(m_seed()), u01(-1.0f, 1.f) {}
};
STATIC_BLOCK{
    type_register<SphereParticleEmitterShape>("SphereParticleEmitterShape")
        .parent<IParticleEmitterShape>()
        .prop("radius", &SphereParticleEmitterShape::radius);
};