#pragma once

#include "math/gfxm.hpp"


typedef uint32_t GIZMO_COLOR;

constexpr GIZMO_COLOR GIZMO_COLOR_RED = 0xFF0000FF;
constexpr GIZMO_COLOR GIZMO_COLOR_GREEN = 0xFF00FF00;
constexpr GIZMO_COLOR GIZMO_COLOR_BLUE = 0xFFFF0000;


class gpuRenderBucket;

struct GizmoContext;


GizmoContext*   gizmoCreateContext();
void            gizmoReleaseContext(GizmoContext*);

void gizmoPushDrawCommands(GizmoContext* ctx, gpuRenderBucket* bucket);
void gizmoClearContext(GizmoContext* ctx);

void gizmoLine(GizmoContext* ctx, const gfxm::vec3& A, const gfxm::vec3& B, float thickness, GIZMO_COLOR color);
void gizmoQuad(
    GizmoContext* ctx, 
    const gfxm::vec3& A, const gfxm::vec3& B, const gfxm::vec3& C, const gfxm::vec3& D,
    GIZMO_COLOR color
);
void gizmoCone(GizmoContext* ctx, const gfxm::mat4& transform, float radius, float height, GIZMO_COLOR color);
void gizmoTorus(GizmoContext* ctx, const gfxm::mat4& transform, float radius, float inner_radius, GIZMO_COLOR color);
