#pragma once

#include "cube_mesh.auto.hpp"
#include "mesh_base.hpp"
#include "gpu/gpu_mesh.hpp"
#include "mesh3d/generate_primitive.hpp"


[[cppi_class]];
class CubeMesh : public Mesh {
    gpuMesh gpu_mesh;
public:
    CubeMesh() {
        Mesh3d mesh3d;
        meshGenerateCube(&mesh3d, 1, 1, 1);
        gpu_mesh.setData(&mesh3d, GPU_MESH_DESC_TYPE::GENERIC);
    }

    const gpuMeshDesc* getMeshDesc() const {
        return gpu_mesh.getMeshDesc();
    }
};

