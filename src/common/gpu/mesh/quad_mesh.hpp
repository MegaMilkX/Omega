#pragma once

#include "quad_mesh.auto.hpp"
#include "mesh_base.hpp"
#include "gpu/gpu_mesh.hpp"
#include "mesh3d/generate_primitive.hpp"


[[cppi_class]];
class QuadMesh : public Mesh {
    gpuMesh* gpu_mesh = nullptr;
public:
    QuadMesh();

    const gpuMeshDesc* getMeshDesc() const {
        return gpu_mesh->getMeshDesc();
    }
};

