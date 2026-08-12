#include "trail_instancing_desc.hpp"


gpuTrailInstancingDesc::gpuTrailInstancingDesc() {
    shader_set = loadResource<gpuShaderSet>("core/shaders/modular/trail.inst");
    setInstanceAttribArray(VFMT::TrailInstanceData0_GUID, &gpu_buffer, sizeof(Instance), offsetof(Instance, length_distance));
    setInstanceCount(0);
}

void gpuTrailInstancingDesc::setArray(Instance* instances, int count) {
    if (count <= 0) {
        gpu_buffer.setArrayData(nullptr, 0);
    }
    gpu_buffer.setArrayData(instances, count * sizeof(instances[0]));
    setInstanceCount(count);
}

void gpuTrailInstancingDesc::apply(GPU_INTERMEDIATE_PASS_DESC& pass) const {
    pass.to_world_vertex_shaders = const_cast<gpuShaderSet*>(shader_set.get());
    pass.addTexture("lutPos", lut_->getId(), SHADER_SAMPLER_TEXTURE_BUFFER);
}
