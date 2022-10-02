#pragma once

#include "math/gfxm.hpp"


inline gfxm::vec3 closestPointOnSegment(const gfxm::vec3& point, const gfxm::vec3& A, const gfxm::vec3& B) {
    gfxm::vec3 BA = B - A;
    float t = gfxm::dot(point - A, BA) / gfxm::dot(BA, BA);
    return gfxm::lerp(A, B, gfxm::clamp(t, .0f, 1.f));
}