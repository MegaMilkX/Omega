#include "quad_mesh.hpp"

#include "gpu/gpu.hpp"


QuadMesh::QuadMesh() {
    gpu_mesh = gpuGetDevice()->getSharedResources()->getQuad();
}

