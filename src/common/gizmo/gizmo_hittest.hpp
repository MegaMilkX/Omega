#pragma once

#include "gizmo_common.hpp"


bool gizmoHitTranslate(
    GIZMO_TRANSFORM_STATE& state,
    int viewport_width, int viewport_height,
    int mouse_x, int mouse_y
);
bool gizmoHitRotate(
    GIZMO_TRANSFORM_STATE& state,
    int viewport_width, int viewport_height,
    int mouse_x, int mouse_y
);

