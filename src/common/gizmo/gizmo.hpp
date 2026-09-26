#pragma once

#include <vector>
#include "gizmo_common.hpp"
#include "gizmo_hittest.hpp"
#include "gpu/gpu_buffer.hpp"
#include "gpu/gpu_renderable.hpp"
#include "gpu/gpu_material.hpp"
#include "gpu/scene_query_interface.hpp"


#pragma pack(push, 1)
struct GizmoLineVertex {
    gfxm::vec3 position;
    float thickness;
    uint32_t color;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct GizmoTriVertex {
    gfxm::vec3 position;
    uint32_t color;
};
#pragma pack(pop)

template<typename VERTEX_T>
struct GizmoMesh {
    std::vector<VERTEX_T> vertices;
    std::vector<uint32_t> indices;
    gpuBuffer buffer;
    gpuBuffer index_buffer;
    gpuMeshDesc mesh_desc;
    RHSHARED<gpuMaterial> material;
    std::unique_ptr<gpuGeometryRenderable> renderable;
};


struct GizmoContext : public gpuSceneQueryInterface {
    GizmoMesh<GizmoLineVertex> lines;
    GizmoMesh<GizmoTriVertex> triangles;

    void queryGeometry(const GeometryQuery& q) override;
};


GizmoContext*   gizmoCreateContext();
void            gizmoReleaseContext(GizmoContext*);

void gizmoPushDrawCommands(GizmoContext* ctx, gpuRenderBucket* bucket);
void gizmoClearContext(GizmoContext* ctx);

void gizmoLine(GizmoContext* ctx, const gfxm::vec3& A, const gfxm::vec3& B, float thickness, GIZMO_COLOR color);
void gizmoCircle(GizmoContext* ctx, const gfxm::mat4& transform, float radius, float thickness, GIZMO_COLOR color);
void gizmoQuad(
    GizmoContext* ctx, 
    const gfxm::vec3& A, const gfxm::vec3& B, const gfxm::vec3& C, const gfxm::vec3& D,
    GIZMO_COLOR color
);
void gizmoAABB(GizmoContext* ctx, const gfxm::aabb& box, const gfxm::mat4& transform, GIZMO_COLOR color);
void gizmoCylinder(GizmoContext* ctx, float radius, float height, int nsegments, const gfxm::mat4& transform, uint32_t color);
void gizmoCone(GizmoContext* ctx, const gfxm::mat4& transform, float radius, float height, GIZMO_COLOR color);
void gizmoTorus(GizmoContext* ctx, const gfxm::mat4& transform, float radius, float inner_radius, GIZMO_COLOR color);


void gizmoTranslate(GizmoContext* ctx, const GIZMO_TRANSFORM_STATE& state);
void gizmoRotate(GizmoContext* ctx, const GIZMO_TRANSFORM_STATE& state);
