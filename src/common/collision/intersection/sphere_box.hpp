#pragma once

#include "math/gfxm.hpp"
#include "debug_draw/debug_draw.hpp"


inline float getSpherePenetration(const gfxm::vec3& box_half_extents, const gfxm::vec3& sphere_relative_pos, gfxm::vec3& out_closest_point, gfxm::vec3& out_normal) {
    float face_dist = box_half_extents.x - sphere_relative_pos.x;
    float min_dist = face_dist;
    out_closest_point.x = box_half_extents.x;
    out_normal = gfxm::vec3(1.0f, 0.0f, 0.0f);
    
    face_dist = box_half_extents.x + sphere_relative_pos.x;
    if (face_dist < min_dist) {
        min_dist = face_dist;
        out_closest_point = sphere_relative_pos;
        out_closest_point.x = -box_half_extents.x;
        out_normal = gfxm::vec3(-1.0f, 0.0f, 0.0f);
    }

    face_dist = box_half_extents.y - sphere_relative_pos.y;
    if (face_dist < min_dist) {
        min_dist = face_dist;
        out_closest_point = sphere_relative_pos;
        out_closest_point.y = box_half_extents.y;
        out_normal = gfxm::vec3(0, 1, 0);
    }

    face_dist = box_half_extents.y + sphere_relative_pos.y;
    if (face_dist < min_dist) {
        min_dist = face_dist;
        out_closest_point = sphere_relative_pos;
        out_closest_point.y = -box_half_extents.y;
        out_normal = gfxm::vec3(0, -1, 0);
    }

    face_dist = box_half_extents.z - sphere_relative_pos.z;
    if (face_dist < min_dist) {
        min_dist = face_dist;
        out_closest_point = sphere_relative_pos;
        out_closest_point.z = box_half_extents.z;
        out_normal = gfxm::vec3(0, 0, 1);
    }

    face_dist = box_half_extents.z + sphere_relative_pos.z;
    if (face_dist < min_dist) {
        min_dist = face_dist;
        out_closest_point = sphere_relative_pos;
        out_closest_point.z = -box_half_extents.z;
        out_normal = gfxm::vec3(0, 0, -1);
    }

    return min_dist;
}

inline float squaredDistancePointAABB(const gfxm::vec3& p, const gfxm::vec3& min, const gfxm::vec3& max) {
    auto check = [](float pn, float min, float max) -> float {
        float out = .0f;
        float v = pn;
        if (v < min) {
            float val = (min - v);
            out += val * val;
        }
        if (v > max) {
            float val = (v - max);
            out += val * val;
        }
        return out;
    };
    float sq = .0f;
    sq += check(p.x, min.x, max.x);
    sq += check(p.y, min.y, max.y);
    sq += check(p.z, min.z, max.z);
    return sq;
}

inline bool intersectionSphereBox(
    float sphere_radius, const gfxm::vec3& sphere_pos, const gfxm::vec3& box_half_extents, 
    const gfxm::mat4& box_transform, ContactPoint& cp
) {
    float max_contact_distance = 1.0f; // TODO
    float box_margin = .0f;

    gfxm::vec3 sphere_relative_pos = gfxm::inverse(box_transform) * gfxm::vec4(sphere_pos, 1.0f);
    
    gfxm::vec3 closest_pt = sphere_relative_pos;
    closest_pt.x = gfxm::_min(box_half_extents.x, closest_pt.x);
    closest_pt.x = gfxm::_max(-box_half_extents.x, closest_pt.x);
    closest_pt.y = gfxm::_min(box_half_extents.y, closest_pt.y);
    closest_pt.y = gfxm::_max(-box_half_extents.y, closest_pt.y);
    closest_pt.z = gfxm::_min(box_half_extents.z, closest_pt.z);
    closest_pt.z = gfxm::_max(-box_half_extents.z, closest_pt.z);

    //dbgDrawCross(box_transform * gfxm::translate(gfxm::mat4(1.0f), closest_pt), .5f, DBG_COLOR_RED);

    float intersection_dist = sphere_radius + box_margin;
    float contact_dist = intersection_dist + max_contact_distance;
    gfxm::vec3 normal = sphere_relative_pos - closest_pt;

    float squared_dist = squaredDistancePointAABB(
        sphere_relative_pos,
        gfxm::vec3(-box_half_extents.x, -box_half_extents.y, -box_half_extents.z),
        gfxm::vec3(box_half_extents.x, box_half_extents.y, box_half_extents.z)
    );
    if (squared_dist > (sphere_radius * sphere_radius)) {
        return false;
    }

    // no penetration
    float dist2 = gfxm::dot(normal, normal);
    if (dist2 > contact_dist * contact_dist) {
        return false;
    }

    float distance = .0f;

    // sphere is inside the box
    if (dist2 <= FLT_EPSILON) {
        distance = -getSpherePenetration(box_half_extents, sphere_relative_pos, closest_pt, normal);
    } else {
        distance = normal.length();
        normal /= distance;
    }

    gfxm::vec3 point_on_box = closest_pt + normal * box_margin;
    
    cp.depth = distance - intersection_dist;
    cp.point_b = box_transform * gfxm::vec4(point_on_box, 1.0f);
    cp.normal_b = box_transform * gfxm::vec4(normal, .0f);
    cp.point_a = cp.point_b; // TODO?
    cp.normal_b = -cp.normal_a;

    return true;
}