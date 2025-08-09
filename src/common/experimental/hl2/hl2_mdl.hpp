#pragma once

#include <memory>
#include <vector>
#include "gpu/gpu_buffer.hpp"
#include "gpu/gpu_mesh_desc.hpp"
#include "gpu/gpu_material.hpp"


struct MDLMesh {
    gpuBuffer index_buffer;
    gpuMeshDesc mesh_desc;
    RHSHARED<gpuMaterial> material;
};

struct MDLModel {
    gpuBuffer vertex_buffer;
    gpuBuffer normal_buffer;
    gpuBuffer uv_buffer;
    gpuBuffer tangent_buffer;
    gpuBuffer binormal_buffer;
    gpuBuffer color_buffer;
    std::vector<std::unique_ptr<MDLMesh>> meshes;
    std::vector<RHSHARED<gpuMaterial>> materials;
};


bool hl2LoadModel(const char* path, MDLModel* out_model);

