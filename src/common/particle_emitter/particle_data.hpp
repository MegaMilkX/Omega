#pragma once

#include <vector>
#include "math/gfxm.hpp"
#include "gpu/gpu_buffer.hpp"
#include "gpu/gpu_renderable.hpp"
#include "gpu/vertex_format.hpp"


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
    std::vector<gfxm::vec4> particlePositions;
    std::vector<gfxm::vec4> particlePrevPositions;
    std::vector<gfxm::vec4> particleLocalPos;
    std::vector<gfxm::vec4> particleScale;
    std::vector<gfxm::vec4> particleColors;
    std::vector<gfxm::vec4> particleSpriteData;
    std::vector<gfxm::vec4> particleSpriteUV;
    std::vector<gfxm::quat> particleRotation;
    int maxParticles;

    gpuBuffer posBuffer;
    gpuBuffer particleScaleBuffer;
    gpuBuffer particleColorBuffer;
    gpuBuffer particleSpriteDataBuffer;
    gpuBuffer particleSpriteUVBuffer;
    gpuBuffer particleRotationBuffer;

    gpuInstancingDesc instDesc;

    void init(int maxCount) {
        maxParticles = maxCount;
        alive_count = 0;

        particleStates.resize(maxParticles);

        particlePositions.resize(maxParticles);
        particlePrevPositions.resize(maxParticles);
        particleLocalPos.resize(maxParticles);
        particleScale.resize(maxParticles);
        particleColors.resize(maxParticles);
        particleSpriteData.resize(maxParticles);
        particleSpriteUV.resize(maxParticles);
        particleRotation.resize(maxParticles);

        instDesc.setInstanceAttribArray(VFMT::ParticlePosition_GUID, &posBuffer);
        instDesc.setInstanceAttribArray(VFMT::ParticleScale_GUID, &particleScaleBuffer);
        instDesc.setInstanceAttribArray(VFMT::ParticleColorRGBA_GUID, &particleColorBuffer);
        instDesc.setInstanceAttribArray(VFMT::ParticleSpriteData_GUID, &particleSpriteDataBuffer);
        instDesc.setInstanceAttribArray(VFMT::ParticleSpriteUV_GUID, &particleSpriteUVBuffer);
        instDesc.setInstanceAttribArray(VFMT::ParticleRotation_GUID, &particleRotationBuffer);
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

        particleScale[i] = particleScale[last_alive];
        particlePositions[i] = particlePositions[last_alive];
        particlePrevPositions[i] = particlePrevPositions[last_alive];
        particleLocalPos[i] = particleLocalPos[last_alive];
        particleColors[i] = particleColors[last_alive];
        particleSpriteData[i] = particleSpriteData[last_alive];
        particleSpriteUV[i] = particleSpriteUV[last_alive];
        particleRotation[i] = particleRotation[last_alive];

        return last_alive;
    }
    void updateBuffers() {
        instDesc.setInstanceCount(alive_count);

        posBuffer.setArrayData(particlePositions.data(), alive_count * sizeof(particlePositions[0]));
        particleScaleBuffer.setArrayData(particleScale.data(), alive_count * sizeof(particleScale[0]));
        particleColorBuffer.setArrayData(particleColors.data(), alive_count * sizeof(particleColors[0]));
        particleSpriteDataBuffer.setArrayData(particleSpriteData.data(), alive_count * sizeof(particleSpriteData[0]));
        particleSpriteUVBuffer.setArrayData(particleSpriteUV.data(), alive_count * sizeof(particleSpriteUV[0]));
        particleRotationBuffer.setArrayData(particleRotation.data(), alive_count * sizeof(particleRotation[0]));
    }

    int aliveCount() const { return alive_count; }
};