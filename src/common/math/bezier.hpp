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

// va and vb - control vectors
inline gfxm::vec3 bezierCubic(gfxm::vec3 a, gfxm::vec3 b, gfxm::vec3 va, gfxm::vec3 vb, float t) {
    gfxm::vec3 _a = gfxm::lerp(a, a + va, t);
    gfxm::vec3 _b = gfxm::lerp(a + va, b + vb, t);
    gfxm::vec3 _c = gfxm::lerp(b + vb, b, t);
    return bezier(_a, _b, _c, t);
}


#endif
