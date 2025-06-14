#pragma once

#include <stdint.h>
#include "math/gfxm.hpp"

struct GizmoContext;


typedef uint32_t GIZMO_COLOR;

constexpr GIZMO_COLOR GIZMO_COLOR_RED = 0xFF0000FF;
constexpr GIZMO_COLOR GIZMO_COLOR_GREEN = 0xFF00FF00;
constexpr GIZMO_COLOR GIZMO_COLOR_BLUE = 0xFFFF0000;


constexpr uint32_t GIZMO_X = 0b0001;
constexpr uint32_t GIZMO_Y = 0b0010;
constexpr uint32_t GIZMO_Z = 0b0100;
constexpr uint32_t GIZMO_XY = 0b0011;
constexpr uint32_t GIZMO_XZ = 0b0101;
constexpr uint32_t GIZMO_YZ = 0b0110;
constexpr uint32_t GIZMO_VIEW = 0b1000;


struct GIZMO_TRANSFORM_STATE {
    gfxm::mat4 transform = gfxm::mat4(1.f);
    gfxm::mat4 projection = gfxm::mat4(1.f);
    gfxm::mat4 view = gfxm::mat4(1.f);
    union {
        int hovered_axis;
        int hovered_plane;
    };
    bool is_active = false;
};

