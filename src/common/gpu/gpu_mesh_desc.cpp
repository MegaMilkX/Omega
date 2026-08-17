#include "gpu_mesh_desc.hpp"

#include "gpu_shader_program.hpp"
#include "gpu_material.hpp"
#include "gpu_instancing_desc.hpp"
#include "gpu_util.hpp"
#include "gpu.hpp"


void gpuMeshDesc::apply(GPU_INTERMEDIATE_PASS_DESC& pass) const {
    pass.attrib_vertex_shaders = gpuGetDevice()->getSharedResources()->getAttribVertexShader(mesh_type);
    pass.attrib_geometry_shaders = gpuGetDevice()->getSharedResources()->getAttribGeometryShader(mesh_type);
    pass.attrib_fragment_shaders = gpuGetDevice()->getSharedResources()->getAttribFragmentShader(mesh_type);
}

