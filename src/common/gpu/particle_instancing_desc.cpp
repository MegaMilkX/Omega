#include "particle_instancing_desc.hpp"



gpuParticleInstancingDesc::gpuParticleInstancingDesc() {
    shader_set = loadResource<gpuShaderSet>("core/shaders/modular/particle.inst");
    setInstanceAttribArray(VFMT::ParticlePosition_GUID, &gpu_buffer, sizeof(Instance), offsetof(Instance, pos));
    setInstanceAttribArray(VFMT::ParticleScale_GUID, &gpu_buffer, sizeof(Instance), offsetof(Instance, scale));
    setInstanceAttribArray(VFMT::ParticleColorRGBA_GUID, &gpu_buffer, sizeof(Instance), offsetof(Instance, rgba));
    setInstanceAttribArray(VFMT::ParticleSpriteData_GUID, &gpu_buffer, sizeof(Instance), offsetof(Instance, sprite_data));
    setInstanceAttribArray(VFMT::ParticleSpriteUV_GUID, &gpu_buffer, sizeof(Instance), offsetof(Instance, uv));
    setInstanceAttribArray(VFMT::ParticleRotation_GUID, &gpu_buffer, sizeof(Instance), offsetof(Instance, quat));
    setInstanceCount(0);
}

void gpuParticleInstancingDesc::setArray(Instance* instances, int count) {
    if (count <= 0) {
        gpu_buffer.setArrayData(nullptr, 0);
    }
    gpu_buffer.setArrayData(instances, count * sizeof(instances[0]));
    setInstanceCount(count);
}

void gpuParticleInstancingDesc::apply(GPU_INTERMEDIATE_PASS_DESC& pass) const {
    pass.to_world_vertex_shaders = const_cast<gpuShaderSet*>(shader_set.get());
}