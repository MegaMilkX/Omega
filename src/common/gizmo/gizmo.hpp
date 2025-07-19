#pragma once

#include "gizmo_common.hpp"
#include "gizmo_hittest.hpp"


class gpuRenderBucket;


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
