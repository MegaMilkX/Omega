#pragma once

#include <random>
#include "particle_emitter/shape/particle_emitter_shape.hpp"


class BoxParticleEmitterShape : public IParticleEmitterShape {
    TYPE_ENABLE(IParticleEmitterShape);

    std::random_device m_seed;
    std::mt19937_64 mt_gen;
    std::uniform_real_distribution<float> u01;

    void emitSome(ptclParticleData* pd, int count) override {
        for (int i = 0; i < count; ++i) {
            int pti = pd->emitOne();

            gfxm::vec3 p(
                -half_extents.x + (half_extents.x - -half_extents.x) * u01(mt_gen),
                -half_extents.y + (half_extents.y - -half_extents.y) * u01(mt_gen),
                -half_extents.z + (half_extents.z - -half_extents.z) * u01(mt_gen)
            );
            pd->particlePositions[pti] = gfxm::vec4(
                p, .0f
            );

            gfxm::vec3 velo = gfxm::normalize(p);
            pd->particleStates[pti].velocity = velo;

            pd->particleScale[pti].w = .0f;
        }
    }
public:
    gfxm::vec3 half_extents{ .5f, .5f, .5f };

    BoxParticleEmitterShape()
        : mt_gen(m_seed()), u01(.0f, 1.f) {}
    
};
STATIC_BLOCK{
    type_register<BoxParticleEmitterShape>("BoxParticleEmitterShape")
        .parent<IParticleEmitterShape>()
        .prop("half_extents", &BoxParticleEmitterShape::half_extents);
};