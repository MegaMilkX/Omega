#pragma once

#include "math/gfxm.hpp"


struct GJK_SupportPoint {
    gfxm::vec3 M;
    gfxm::vec3 A;
    gfxm::vec3 B;
};

struct GJK_Simplex {
    GJK_SupportPoint points[4];
    int count = 0;

    void insert_point(const GJK_SupportPoint& pt) {
        assert(count < 4);
        points[count++] = pt;
    }
    void remove_point(int at) {
        assert(count > 0);
        assert(at < count - 1 && at >= 0);
        points[at] = points[count - 1];
        --count;
    }

    GJK_SupportPoint& operator[](int i) {
        return points[i];
    }
    const GJK_SupportPoint& operator[](int i) const {
        return points[i];
    }
};


// TODO: Remove
struct SphereShapeGJK {
    gfxm::vec3 pos;
    float radius = .5f;

    SphereShapeGJK(float r)
        : radius(r) {}

    gfxm::vec3 support(const gfxm::vec3& dir) const {
        return pos + gfxm::normalize(dir) * radius;
    }
};
