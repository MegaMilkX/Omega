#pragma once

#include "torus_knot_mesh.auto.hpp"
#include "mesh_base.hpp"
#include "gpu/gpu_mesh.hpp"
#include "mesh3d/generate_primitive.hpp"


[[cppi_class]];
class TorusKnotMesh : public Mesh {
    gpuMesh gpu_mesh;

    void updateMesh() {
        Mesh3d mesh3d;
        meshGenerateTorusKnot(&mesh3d, u_segments, v_segments, pipe_radius, p, q);
        gpu_mesh.setData(&mesh3d, GPU_MESH_DESC_TYPE::GENERIC);
        gpu_mesh.setDrawMode(MESH_DRAW_MODE::MESH_DRAW_TRIANGLE_STRIP);
    }
public:
    TYPE_ENABLE();

    [[cppi_decl]] int u_segments = 128;
    [[cppi_decl]] int v_segments = 10;
    [[cppi_decl]] float pipe_radius = .2f;
    [[cppi_decl]] float p = 5.f;
    [[cppi_decl]] float q = 4.f;

    TorusKnotMesh() {
        // TODO: update on changes
        updateMesh();
    }

    const gpuMeshDesc* getMeshDesc() const {
        return gpu_mesh.getMeshDesc();
    }
};

