#pragma once

#include <memory>
#include <vector>
#include "gpu/gpu_buffer.hpp"
#include "gpu/gpu_mesh_desc.hpp"
#include "gpu/gpu_material.hpp"
#include "static_model/static_model.hpp"
#include "hl2_phy.hpp"


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

    RHSHARED<StaticModel> static_model;

    PHYFile phy;
};


bool hl2LoadModel(const char* path, MDLModel* out_model);

