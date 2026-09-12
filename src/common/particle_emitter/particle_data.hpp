#pragma once

#include <vector>
#include "math/gfxm.hpp"
#include "gpu/gpu_buffer.hpp"
#include "gpu/gpu_renderable.hpp"
#include "gpu/vertex_format.hpp"
#include "gpu/particle_instancing_desc.hpp"


struct ParticleEmitterParams {
    int                 max_count = 1000;
    float               max_lifetime = 2.0f;

    float               duration = 25.5f/60.0f;
    bool                looping = true;

    gfxm::vec3          gravity;
    float               terminal_velocity = 200.f;
    curve<float>        pt_per_second_curve;
    curve<gfxm::vec3>   initial_scale_curve;
    curve<gfxm::vec4>   rgba_curve;
    curve<float>        scale_curve;
};

class ptclParticleData {
    int alive_count;
public:
    struct Particle {
        uint32_t identifier;
        gfxm::vec3 unmodified_pos;
        gfxm::vec3 velocity;
        gfxm::vec3 ang_velocity;
    };

#pragma pack(push, 1)
    struct ParticleDataA {
        gfxm::vec3  position;
        float       scale;
    };
    struct ParticleDataB {
        gfxm::vec3  scale;
        float       lifetime;
    };
#pragma pack(pop)

    uint32_t                next_particle_identifier = 1;
    std::vector<Particle>   particleStates;
    int maxParticles;

    gpuParticleInstancingDesc instDesc;
    std::vector<gpuParticleInstancingDesc::Instance> instances;
    std::vector<gfxm::vec4> local_positions; // For movement on/in emitter shape, exact data is shape dependent
    std::vector<gfxm::vec4> prev_pos;

    void init(int maxCount) {
        maxParticles = maxCount;
        alive_count = 0;

        particleStates.resize(maxParticles);

        instances.resize(maxParticles);
        local_positions.resize(maxParticles);
        prev_pos.resize(maxParticles);
    }
    void clear() {
        alive_count = 0;
    }
    int getAvailableSlots(int desired_count) {
        return gfxm::_min(desired_count, maxParticles - alive_count);
    }
    int emitOne() {
        if (maxParticles == alive_count) {
            return 0;
        }
        const int new_particle_id = alive_count;
        particleStates[new_particle_id].identifier = next_particle_identifier++;

        alive_count++;
        return new_particle_id;
    }
    int recycleParticle(int i) {
        int last_alive = alive_count - 1;
        alive_count--;

        particleStates[i] = particleStates[last_alive];
        instances[i] = instances[last_alive];
        return last_alive;
    }
    void updateBuffers() {
        instDesc.setArray(instances.data(), alive_count);
    }

    int aliveCount() const { return alive_count; }
};