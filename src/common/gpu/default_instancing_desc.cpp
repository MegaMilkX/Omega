#include "default_instancing_desc.hpp"

#include "resource_manager/resource_manager.hpp"


gpuDefaultInstancingDesc::gpuDefaultInstancingDesc() {
    shader_set = loadResource<gpuShaderSet>("core/shaders/modular/default.inst");
    setInstanceAttribArray(VFMT::InstancePosition_GUID, &gpu_buffer, sizeof(Instance), offsetof(Instance, pos));
    setInstanceAttribArray(VFMT::InstanceQuat_GUID, &gpu_buffer, sizeof(Instance), offsetof(Instance, rot));
    setInstanceCount(0);
}

void gpuDefaultInstancingDesc::setArray(Instance* instances, int count) {
    if (count <= 0) {
        gpu_buffer.setArrayData(nullptr, 0);
    }
    gpu_buffer.setArrayData(instances, count * sizeof(instances[0]));
    setInstanceCount(count);
}

void gpuDefaultInstancingDesc::apply(GPU_INTERMEDIATE_PASS_DESC& pass) const {
    pass.to_world_vertex_shaders = const_cast<gpuShaderSet*>(shader_set.get());
}

