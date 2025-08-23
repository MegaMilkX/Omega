#pragma once

#include <format>
#include "gjk_types.hpp"


constexpr int GJK_MAX_ITERATIONS = 30;


GJK_SupportPoint GJK_supportMinkowski(const SphereShapeGJK& A, const SphereShapeGJK& B, const gfxm::vec3& dir);
bool GJK_containsOrigin(GJK_Simplex& simplex, gfxm::vec3& dir);
void GJK_debugDrawSimplex(const gfxm::vec3& at, const GJK_Simplex& simplex);
void GJK_debugDrawSimplexA(const gfxm::vec3& at, const GJK_Simplex& simplex);
void GJK_debugDrawSimplexB(const gfxm::vec3& at, const GJK_Simplex& simplex);
bool GJK(const SphereShapeGJK& A, const SphereShapeGJK& B, GJK_Simplex& simplex);


template<typename T>
class GJKEPA_SupportGetter {
    gfxm::mat4 transform;
    const T* shape;
public:
    GJKEPA_SupportGetter(const gfxm::mat4& transform, const T* shape)
        : transform(transform), shape(shape) {}

    gfxm::vec3 getPosition() const { return transform[3]; }

    gfxm::vec3 operator()(const gfxm::vec3& dir) const {
        return shape->getMinkowskiSupportPoint(dir, transform);
    }
};

template<typename SHAPEA_T, typename SHAPEB_T>
GJK_SupportPoint GJK_supportMinkowski_T(
    const GJKEPA_SupportGetter<SHAPEA_T>& support_getter_a,
    const GJKEPA_SupportGetter<SHAPEB_T>& support_getter_b,
    const gfxm::vec3& dir
) {
    GJK_SupportPoint sp;
    sp.A = support_getter_a(dir);
    sp.B = support_getter_b(-dir);
    sp.M = sp.A - sp.B;
    return sp;
}

template<typename SHAPEA_T, typename SHAPEB_T>
bool GJK_T(
    const GJKEPA_SupportGetter<SHAPEA_T>& support_getter_a,
    const GJKEPA_SupportGetter<SHAPEB_T>& support_getter_b,
    GJK_Simplex& simplex
) {
    simplex.count = 0;

    gfxm::vec3 dir(0, 1, 0);
    dir = support_getter_b.getPosition() - support_getter_a.getPosition();

    GJK_SupportPoint sp = GJK_supportMinkowski_T(support_getter_a, support_getter_b, dir);
    simplex.insert_point(sp);

    dir = -sp.M; // Towards the origin

    for(int i = 0; i < GJK_MAX_ITERATIONS; ++i) {
        GJK_SupportPoint np = GJK_supportMinkowski_T(support_getter_a, support_getter_b, dir);
        if (gfxm::dot(np.M, dir) <= .0f) {
            return false;
        }

        simplex.insert_point(np);

        //GJK_debugDrawSimplex(simplex);
        if(GJK_containsOrigin(simplex, dir)) {
            return true;
        }
    }

    // Ran out of iterations
    return false;
}

template<typename SHAPEA_T, typename SHAPEB_T>
bool GJK_distance_T(
    const GJKEPA_SupportGetter<SHAPEA_T>& support_getter_a,
    const GJKEPA_SupportGetter<SHAPEB_T>& support_getter_b,
    GJK_Simplex& simplex,
    gfxm::vec3& closestA,
    gfxm::vec3& closestB,
    float& dist
) {
    if (GJK_T(support_getter_a, support_getter_b, simplex)) {
        return true;
    }
    assert(simplex.count > 0);

    struct RESULT {
        float distance;
        gfxm::vec3 closestA;
        gfxm::vec3 closestB;

        bool operator<(const RESULT& other) const {
            return distance < other.distance;
        }
        bool operator>(const RESULT& other) const {
            return distance > other.distance;
        }
    };


    auto closestPointOnTriangle = [](const gfxm::vec3& p,
        const gfxm::vec3& a,
        const gfxm::vec3& b,
        const gfxm::vec3& c
    ) -> gfxm::vec3 {
        // From Real-Time Collision Detection, Christer Ericson
        gfxm::vec3 ab = b - a;
        gfxm::vec3 ac = c - a;
        gfxm::vec3 ap = p - a;

        float d1 = gfxm::dot(ab, ap);
        float d2 = gfxm::dot(ac, ap);
        if (d1 <= 0.0f && d2 <= 0.0f) return a; // barycentric (1,0,0)

        gfxm::vec3 bp = p - b;
        float d3 = gfxm::dot(ab, bp);
        float d4 = gfxm::dot(ac, bp);
        if (d3 >= 0.0f && d4 <= d3) return b; // barycentric (0,1,0)

        float vc = d1*d4 - d3*d2;
        if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
            float v = d1 / (d1 - d3);
            return a + ab * v; // barycentric (1-v, v, 0)
        }

        gfxm::vec3 cp = p - c;
        float d5 = gfxm::dot(ab, cp);
        float d6 = gfxm::dot(ac, cp);
        if (d6 >= 0.0f && d5 <= d6) return c; // barycentric (0,0,1)

        float vb = d5*d2 - d1*d6;
        if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
            float w = d2 / (d2 - d6);
            return a + ac * w; // barycentric (1-w, 0, w)
        }

        float va = d3*d6 - d5*d4;
        if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
            float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
            return b + (c - b) * w; // barycentric (0,1-w,w)
        }

        // Inside face region
        float denom2 = 1.0f / (va + vb + vc);
        float v = vb * denom2;
        float w = vc * denom2;
        return a + ab * v + ac * w;
    };

    auto testTriangle = [&closestPointOnTriangle](
        const GJK_SupportPoint& v0,
        const GJK_SupportPoint& v1,
        const GJK_SupportPoint& v2
    ) -> RESULT {
            gfxm::vec3 Am = v0.M;
            gfxm::vec3 Bm = v1.M;
            gfxm::vec3 Cm = v2.M;
            gfxm::vec3 aAw = v0.A;
            gfxm::vec3 aBw = v1.A;
            gfxm::vec3 aCw = v2.A;
            gfxm::vec3 bAw = v0.B;
            gfxm::vec3 bBw = v1.B;
            gfxm::vec3 bCw = v2.B;

            gfxm::vec3 CPm = closestPointOnTriangle(gfxm::vec3(0,0,0), Am, Bm, Cm);
            RESULT res;
            {
                gfxm::vec3 ab = Bm - Am;
                gfxm::vec3 ac = Cm - Am;
                gfxm::vec3 ap = CPm - Am;

                float d00 = gfxm::dot(ab, ab);
                float d01 = gfxm::dot(ab, ac);
                float d11 = gfxm::dot(ac, ac);
                float d20 = gfxm::dot(ap, ab);
                float d21 = gfxm::dot(ap, ac);

                float denom = d00 * d11 - d01 * d01;
                if (fabsf(denom) < 1e-8f) {
                    float abLen2 = d00;
                    float acLen2 = d11;
                    if (abLen2 > acLen2 && abLen2 > 1e-12f) {
                        // Treat as line AB
                        float t = gfxm::clamp(gfxm::dot(-Am, ab) / abLen2, 0.0f, 1.0f);
                        res.closestA = v0.A + t * (v1.A - v0.A);
                        res.closestB = v0.B + t * (v1.B - v0.B);
                    } else if (acLen2 > 1e-12f) {
                        // Treat as line AC
                        float t = gfxm::clamp(gfxm::dot(-Am, ac) / acLen2, 0.0f, 1.0f);
                        res.closestA = v0.A + t * (v2.A - v0.A);
                        res.closestB = v0.B + t * (v2.B - v0.B);
                    } else {
                        // Treat as point
                        res.closestA = v0.A;
                        res.closestB = v0.B;
                    }
                } else {
                    float v = (d11 * d20 - d01 * d21) / denom;
                    float w = (d00 * d21 - d01 * d20) / denom;
                    float u = 1.0f - v - w;

                    v = gfxm::clamp(v, .0f, 1.f);
                    w = gfxm::clamp(w, .0f, 1.f);
                    u = gfxm::clamp(u, .0f, 1.f);

                    res.closestA = aAw * u + aBw * v + aCw * w;
                    res.closestB = bAw * u + bBw * v + bCw * w;
                }

                //res.distance = gfxm::length(CPm);
                res.distance = gfxm::length(res.closestB - res.closestA);
            }

            return res;
    };

    if (simplex.count == 1) {
        // TODO: length of .M is the distance?
        closestA = simplex[0].A;
        closestB = simplex[0].B;
        dist = gfxm::length(simplex[0].B - simplex[0].A);
        //dist = gfxm::length(simplex[0].M);
    } else if (simplex.count == 2) {
        const GJK_SupportPoint& v0 = simplex[0];
        const GJK_SupportPoint& v1 = simplex[1];
        const gfxm::vec3 ab = v1.M - v0.M;
        const gfxm::vec3 ao = -v0.M;

        const float abLen2 = dot(ab, ab);
        if (abLen2 < 1e-12f) {
            closestA = v0.A;
            closestB = v0.B;
            dist = gfxm::length(closestB - closestA);
        } else {
            float tseg = gfxm::dot(-v0.M, ab) / abLen2;
            tseg = gfxm::clamp(tseg, 0.0f, 1.0f);
            closestA = v0.A + tseg * (v1.A - v0.A);
            closestB = v0.B + tseg * (v1.B - v0.B);
            dist = gfxm::length(closestB - closestA);
        }
    } else if (simplex.count == 3) {
        RESULT res = testTriangle(simplex[0], simplex[1], simplex[2]);
        closestA = res.closestA;
        closestB = res.closestB;
        dist = res.distance;
        //GJK_debugDrawSimplex(support_getter_a.getPosition(), simplex);
    } else if (simplex.count == 4) {
        RESULT r0 = testTriangle(simplex[0], simplex[1], simplex[2]);
        RESULT r1 = testTriangle(simplex[0], simplex[1], simplex[3]);
        RESULT r2 = testTriangle(simplex[0], simplex[2], simplex[3]);
        RESULT r3 = testTriangle(simplex[1], simplex[2], simplex[3]);
        RESULT r = r0;
        if(r1.distance < r.distance) r = r1;
        if(r2.distance < r.distance) r = r2;
        if(r3.distance < r.distance) r = r3;
        closestA = r.closestA;
        closestB = r.closestB;
        dist = r.distance;
    }

    //dbgDrawText(support_getter_a.getPosition(), std::format("{:.3f}", dist), 0xFFFFFFFF);
    return false;
}

