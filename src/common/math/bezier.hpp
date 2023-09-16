#ifndef KT_BEZIER_HPP
#define KT_BEZIER_HPP

#include "gfxm.hpp"


inline gfxm::vec3 bezier(gfxm::vec3 a, gfxm::vec3 b, gfxm::vec3 c, float t) {
    gfxm::vec3 a1;
    gfxm::vec3 b1;
    a1 = gfxm::lerp(a, b, t);
    b1 = gfxm::lerp(b, c, t);
    return gfxm::lerp(a1, b1, t);
}
inline gfxm::vec2 bezier(gfxm::vec2 a, gfxm::vec2 b, gfxm::vec2 c, float t) {
    gfxm::vec2 a1;
    gfxm::vec2 b1;
    a1 = gfxm::lerp(a, b, t);
    b1 = gfxm::lerp(b, c, t);
    return gfxm::lerp(a1, b1, t);
}

// va and vb - control vectors
inline gfxm::vec3 bezierCubic(gfxm::vec3 a, gfxm::vec3 b, gfxm::vec3 va, gfxm::vec3 vb, float t) {
    gfxm::vec3 _a = gfxm::lerp(a, a + va, t);
    gfxm::vec3 _b = gfxm::lerp(a + va, b + vb, t);
    gfxm::vec3 _c = gfxm::lerp(b + vb, b, t);
    return bezier(_a, _b, _c, t);
}
inline gfxm::vec3 bezierCubic_(gfxm::vec3 a, gfxm::vec3 b, gfxm::vec3 c, gfxm::vec3 d, float t) {
    gfxm::vec3 _a = gfxm::lerp(a, b, t);
    gfxm::vec3 _b = gfxm::lerp(b, c, t);
    gfxm::vec3 _c = gfxm::lerp(c, d, t);
    return bezier(_a, _b, _c, t);
}
inline float bezierCubic1d(gfxm::vec2 a, gfxm::vec2 b, gfxm::vec2 c, gfxm::vec2 d, float t) {
    gfxm::vec2 _a = gfxm::lerp(a, b, t);
    gfxm::vec2 _b = gfxm::lerp(b, c, t);
    gfxm::vec2 _c = gfxm::lerp(c, d, t);
    gfxm::vec2 r = bezier(_a, _b, _c, t);
    return r.y;
}

inline void bezierCubicRecursive(
    float x1, float y1, float z1,
    float x2, float y2, float z2,
    float x3, float y3, float z3,
    float x4, float y4, float z4,
    std::function<void(const gfxm::vec3&)> cb
) {
    const float distance_tolerance = .25f;

    const float x12 = (x1 + x2) * .5f;
    const float y12 = (y1 + y2) * .5f;
    const float z12 = (z1 + z2) * .5f;
    const float x23 = (x2 + x3) * .5f;
    const float y23 = (y2 + y3) * .5f;
    const float z23 = (z2 + z3) * .5f;
    const float x34 = (x3 + x4) * .5f;
    const float y34 = (y3 + y4) * .5f;
    const float z34 = (z3 + z4) * .5f;
    const float x123 = (x12 + x23) * .5f;
    const float y123 = (y12 + y23) * .5f;
    const float z123 = (z12 + z23) * .5f;
    const float x234 = (x23 + x34) * .5f;
    const float y234 = (y23 + y34) * .5f;
    const float z234 = (z23 + z34) * .5f;
    const float x1234 = (x123 + x234) * .5f;
    const float y1234 = (y123 + y234) * .5f;
    const float z1234 = (z123 + z234) * .5f;

    const float dx = x4 - x1;
    const float dy = y4 - y1;
    const float dz = z4 - z1;
    if (fabsf(dx) <= FLT_EPSILON && fabsf(dy) <= FLT_EPSILON && fabsf(dz) <= FLT_EPSILON) {
        return;
    }

    const float d2 = fabs(((x2 - x4) * dy - (y2 - y4) * dx));
    const float d3 = fabs(((x3 - x4) * dy - (y3 - y4) * dx));

    float t = (d2 + d3)*(d2 + d3);
    if (t < 3.0f * (dx * dx + dy * dy)) {
        cb(gfxm::vec3(x1234, y1234, z1234));// add_point(x1234, y1234);
        return;
    }

    // Continue subdivision
    //----------------------
    bezierCubicRecursive(x1, y1, z1, x12, y12, z12, x123, y123, z123, x1234, y1234, z1234, cb);
    bezierCubicRecursive(x1234, y1234, z1234, x234, y234, z234, x34, y34, z34, x4, y4, z4, cb);
}
inline void bezierCubicRecursive(
    const gfxm::vec3& a, const gfxm::vec3& b,
    const gfxm::vec3& c, const gfxm::vec3& d,
    std::function<void(const gfxm::vec3&)> cb
) {
    cb(a);
    bezierCubicRecursive(
        a.x, a.y, a.z, b.x, b.y, b.z,
        c.x, c.y, c.z, d.x, d.y, d.z,
        cb
    );
    cb(d);
}

inline void simplifyPath(std::vector<gfxm::vec3>& points) {
    if (points.size() < 2) {
        return;
    }
    int key = 1;
    gfxm::vec3 ref = gfxm::normalize(points[1] - points[0]);
    while (key < points.size() - 1) {
        gfxm::vec3& p0 = points[key];
        gfxm::vec3& p1 = points[key + 1];
        if (gfxm::length(p1 - p0) <= FLT_EPSILON) {
            points.erase(points.begin() + key);
            continue;
        }
        gfxm::vec3 ref2 = gfxm::normalize(p1 - p0);
        float d = gfxm::dot(ref, ref2);
        if (d < .985f) {
            ++key;
            ref = ref2;
        } else {
            points.erase(points.begin() + key);
        }
    }
}
inline void simplifyPath2d(std::vector<gfxm::vec2>& points) {
    if (points.size() < 2) {
        return;
    }
    int key = 1;
    gfxm::vec2 ref = gfxm::normalize(points[1] - points[0]);
    while (key < points.size() - 1) {
        gfxm::vec2& p0 = points[key];
        gfxm::vec2& p1 = points[key + 1];
        if (gfxm::length(p1 - p0) <= FLT_EPSILON) {
            points.erase(points.begin() + key);
            continue;
        }
        gfxm::vec2 ref2 = gfxm::normalize(p1 - p0);
        float d = gfxm::dot(ref, ref2);
        if (d < .985f) {
            ++key;
            ref = ref2;
        } else {
            points.erase(points.begin() + key);
        }
    }
}

#endif
