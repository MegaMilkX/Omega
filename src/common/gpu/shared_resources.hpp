#pragma once

#include "gpu/gpu_mesh.hpp"
#include "gpu/shader_set.hpp"
#include "gpu/gpu_shader_program.hpp"
#include "gpu/param_block/transform_block.hpp"


class gpuSharedResources {
    std::unique_ptr<gpuMesh> mesh_decal_cube;

    ResourceRef<gpuShaderSet> to_world_shaders;
    ResourceRef<gpuShaderSet> attrib_vert_shaders[int(GPU_MESH_DESC_TYPE::COUNT)];
    ResourceRef<gpuShaderSet> attrib_frag_shaders[int(GPU_MESH_DESC_TYPE::COUNT)];

    std::unique_ptr<gpuShaderProgram> prog_present_rgb;
    std::unique_ptr<gpuShaderProgram> prog_present_rrr;
    std::unique_ptr<gpuShaderProgram> prog_present_ggg;
    std::unique_ptr<gpuShaderProgram> prog_present_bbb;
    std::unique_ptr<gpuShaderProgram> prog_present_aaa;
    std::unique_ptr<gpuShaderProgram> prog_present_depth;
    std::unique_ptr<gpuShaderProgram> prog_sample_cubemap;

    gpuTransformBlock* identity_transform_block = nullptr;
public:
    gpuSharedResources();

    gpuMesh* getUnitCube();
    gpuMesh* getInvertedUnitCube();
    gpuMesh* getDecalUnitCube();

    gpuShaderSet* getToWorldShader();
    gpuShaderSet* getAttribVertexShader(GPU_MESH_DESC_TYPE type);
    gpuShaderSet* getAttribFragmentShader(GPU_MESH_DESC_TYPE type);

    gpuShaderProgram* getPresentProgram(RT_OUTPUT type);
    gpuShaderProgram* getCubemapSampleProgram();

    gpuTransformBlock* getIdentityTransformBlock();
};