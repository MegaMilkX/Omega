#include "box_box.hpp"

#include <assert.h>
#include "debug_draw/debug_draw.hpp"


bool intersectBoxBox(
    const gfxm::vec3& half_extents_a,
    const gfxm::mat4& transform_a,
    const gfxm::vec3& half_extents_b,
    const gfxm::mat4& transform_b,
    ContactPoint& cp
) {
    gfxm::vec3 axes[15] = {
        transform_a[0],
        transform_a[1],
        transform_a[2],
        transform_b[0],
        transform_b[1],
        transform_b[2],
        gfxm::normalize(gfxm::cross(gfxm::vec3(transform_a[0]), gfxm::vec3(transform_b[0]))),
        gfxm::normalize(gfxm::cross(gfxm::vec3(transform_a[0]), gfxm::vec3(transform_b[1]))),
        gfxm::normalize(gfxm::cross(gfxm::vec3(transform_a[0]), gfxm::vec3(transform_b[2]))),
        gfxm::normalize(gfxm::cross(gfxm::vec3(transform_a[1]), gfxm::vec3(transform_b[0]))),
        gfxm::normalize(gfxm::cross(gfxm::vec3(transform_a[1]), gfxm::vec3(transform_b[1]))),
        gfxm::normalize(gfxm::cross(gfxm::vec3(transform_a[1]), gfxm::vec3(transform_b[2]))),
        gfxm::normalize(gfxm::cross(gfxm::vec3(transform_a[2]), gfxm::vec3(transform_b[0]))),
        gfxm::normalize(gfxm::cross(gfxm::vec3(transform_a[2]), gfxm::vec3(transform_b[1]))),
        gfxm::normalize(gfxm::cross(gfxm::vec3(transform_a[2]), gfxm::vec3(transform_b[2])))
    };

    gfxm::vec3 P = transform_a[3];
    gfxm::vec3 P2 = transform_b[3];

    gfxm::vec3 lcl_points_a[8] = {
        gfxm::vec3(-half_extents_a.x, -half_extents_a.y, -half_extents_a.z),
        gfxm::vec3(-half_extents_a.x, -half_extents_a.y, half_extents_a.z),
        gfxm::vec3(half_extents_a.x, -half_extents_a.y, half_extents_a.z),
        gfxm::vec3(half_extents_a.x, -half_extents_a.y, -half_extents_a.z),
        gfxm::vec3(-half_extents_a.x, half_extents_a.y, -half_extents_a.z),
        gfxm::vec3(-half_extents_a.x, half_extents_a.y, half_extents_a.z),
        gfxm::vec3(half_extents_a.x, half_extents_a.y, half_extents_a.z),
        gfxm::vec3(half_extents_a.x, half_extents_a.y, -half_extents_a.z)
    };
    gfxm::vec3 lcl_points_b[8] = {
        gfxm::vec3(-half_extents_b.x, -half_extents_b.y, -half_extents_b.z),
        gfxm::vec3(-half_extents_b.x, -half_extents_b.y, half_extents_b.z),
        gfxm::vec3(half_extents_b.x, -half_extents_b.y, half_extents_b.z),
        gfxm::vec3(half_extents_b.x, -half_extents_b.y, -half_extents_b.z),
        gfxm::vec3(-half_extents_b.x, half_extents_b.y, -half_extents_b.z),
        gfxm::vec3(-half_extents_b.x, half_extents_b.y, half_extents_b.z),
        gfxm::vec3(half_extents_b.x, half_extents_b.y, half_extents_b.z),
        gfxm::vec3(half_extents_b.x, half_extents_b.y, -half_extents_b.z)
    };
    gfxm::vec3 points_a[8] = {
        transform_a * gfxm::vec4(lcl_points_a[0], 1.f),
        transform_a * gfxm::vec4(lcl_points_a[1], 1.f),
        transform_a * gfxm::vec4(lcl_points_a[2], 1.f),
        transform_a * gfxm::vec4(lcl_points_a[3], 1.f),
        transform_a * gfxm::vec4(lcl_points_a[4], 1.f),
        transform_a * gfxm::vec4(lcl_points_a[5], 1.f),
        transform_a * gfxm::vec4(lcl_points_a[6], 1.f),
        transform_a * gfxm::vec4(lcl_points_a[7], 1.f)
    };
    gfxm::vec3 points_b[8] = {
        transform_b * gfxm::vec4(lcl_points_b[0], 1.f),
        transform_b * gfxm::vec4(lcl_points_b[1], 1.f),
        transform_b * gfxm::vec4(lcl_points_b[2], 1.f),
        transform_b * gfxm::vec4(lcl_points_b[3], 1.f),
        transform_b * gfxm::vec4(lcl_points_b[4], 1.f),
        transform_b * gfxm::vec4(lcl_points_b[5], 1.f),
        transform_b * gfxm::vec4(lcl_points_b[6], 1.f),
        transform_b * gfxm::vec4(lcl_points_b[7], 1.f)
    };

    struct POINT {
        float t;
        int index;

        bool operator<(const POINT& other) const {
            return t < other.t;
        }
        bool operator>(const POINT& other) const {
            return t > other.t;
        }
    };
#define PT(T, IDX) POINT{ .t = T, .index = IDX }

    float min_dist = FLT_MAX;
    POINT PA;
    POINT PB;
    gfxm::vec3 N;
    int axis_idx = -1;
    for (int i = 0; i < 6; ++i) {
        const gfxm::vec3& axis = axes[i];

        float da0 = gfxm::dot(axis, points_a[0]);
        POINT min_a = PT(da0, 0);
        POINT max_a = PT(da0, 0);
        for(int j = 1; j < 8; ++j) {
            POINT d = PT(gfxm::dot(axis, points_a[j]), j);
            min_a = gfxm::_min(min_a, d);
            max_a = gfxm::_max(max_a, d);
        }
        float db0 = gfxm::dot(axis, points_b[0]);
        POINT min_b = PT(db0, 0);
        POINT max_b = PT(db0, 0);
        for(int j = 1; j < 8; ++j) {
            POINT d = PT(gfxm::dot(axis, points_b[j]), j);
            min_b = gfxm::_min(min_b, d);
            max_b = gfxm::_max(max_b, d);
        }

        POINT max = gfxm::_min(max_a, max_b);
        POINT min = gfxm::_max(min_a, min_b);
        float diff = max.t - min.t;
        if (diff <= .0f) {
            return false;
        }

        if (diff < min_dist) {
            min_dist = diff;
            if (min.t == max_a.t) {
                PA = max;
                PB = min;
            } else {
                PA = min;
                PB = max;
            }
            N = axis;
            axis_idx = i;
        }
    }


    gfxm::vec3 contact_a;
    gfxm::vec3 contact_b;
    if (axis_idx >= 0 && axis_idx < 3) {
        contact_b = points_b[PB.index];
        float d = gfxm::dot(N, points_b[PB.index]) - PB.t;
        contact_a = N * d;
    } else if (axis_idx >= 3 && axis_idx < 6) {
        contact_a = points_a[PA.index];
        float d = gfxm::dot(N, points_a[PA.index]) - PA.t;
        contact_b = N * d;
    }

    cp.depth = min_dist;
    cp.point_a = contact_a;
    cp.point_b = contact_b;
    cp.normal_a = N;
    cp.normal_b = -N;
    cp.type = CONTACT_POINT_TYPE::DEFAULT;
    return true;
}

