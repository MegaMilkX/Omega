#pragma once

#include "particle_emitter/particle_data.hpp"
#include "animation/curve.hpp"


class ptclComponent {
public:
    virtual ~ptclComponent() {}
    virtual void update(ptclParticleData* pd, float dt) = 0;
};
class ptclAngularVelocityComponent : public ptclComponent {
    curve<gfxm::vec3> euler_ang_vel_curve;
public:
    ptclAngularVelocityComponent() {
        euler_ang_vel_curve[.0f] = gfxm::vec3(.0f, .0f, 5.f);
        euler_ang_vel_curve[1.f] = gfxm::vec3(.0f, .0f, .0f);
    }
    void update(ptclParticleData* pd, float dt) override {
        for (int i = 0; i < pd->aliveCount(); ++i) {/*
            float lifetime = pd->particleData[i].w;
            float flip = 1.0f - (i % 2) * 2.0f;
            pd->particleRotation[i]
                = gfxm::euler_to_quat(euler_ang_vel_curve.at(lifetime / pd->maxLifetime) * dt)
                * pd->particleRotation[i];*/
        }
    }
};
class ptclVelocityComponent : public ptclComponent {
    curve<float> velocity_mul_curve;
public:
    ptclVelocityComponent() {
        velocity_mul_curve[.0f] = 5.f;
        velocity_mul_curve[1.f] = 1.f;
    }
    void update(ptclParticleData* pd, float dt) override {
        for (int i = 0; i < pd->aliveCount(); ++i) {
            float lifetime = pd->particleScale[i].w;
            // TODO: !!!
            gfxm::vec3 base_velo = gfxm::vec3(1.0f, 1.0f, 1.0f);
            gfxm::vec3 velo = base_velo * velocity_mul_curve.at(lifetime / pd->maxLifetime);
            
        }
    }
};