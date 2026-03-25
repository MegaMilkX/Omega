#pragma once

#include "gpu/gpu_mesh.hpp"
#include "gpu/gpu_shader_program.hpp"
#include "gpu/param_block/transform_block.hpp"


class gpuSharedResources {
    std::unique_ptr<gpuMesh> mesh_decal_cube;

    std::unique_ptr<gpuShaderProgram> prog_present_rgb;
    std::unique_ptr<gpuShaderProgram> prog_present_rrr;
    std::unique_ptr<gpuShaderProgram> prog_present_ggg;
    std::unique_ptr<gpuShaderProgram> prog_present_bbb;
    std::unique_ptr<gpuShaderProgram> prog_present_aaa;
    std::unique_ptr<gpuShaderProgram> prog_present_depth;
    std::unique_ptr<gpuShaderProgram> prog_sample_cubemap;

    gpuTransformBlock* identity_transform_block = nullptr;
public:
    gpuMesh* getUnitCube();
    gpuMesh* getInvertedUnitCube();
    gpuMesh* getDecalUnitCube();

    gpuShaderProgram* getPresentProgram(RT_OUTPUT type);
    gpuShaderProgram* getCubemapSampleProgram();

    gpuTransformBlock* getIdentityTransformBlock();
};