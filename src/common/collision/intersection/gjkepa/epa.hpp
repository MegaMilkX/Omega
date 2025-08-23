#pragma once

#include "epa_types.hpp"
#include "gjk.hpp"


bool EPA_init(EPA_Context& ctx, const GJK_Simplex& simplex);
int EPA_findClosestFaceToOrigin(const EPA_Context& ctx);
EPA_Result EPA_finalize(const EPA_Context& ctx, const EPA_Face& face);
bool EPA_addPoint(EPA_Context& ctx, const GJK_SupportPoint& Ps);

EPA_Result EPA(const SphereShapeGJK& A, const SphereShapeGJK& B, EPA_Context& ctx, const GJK_Simplex& simplex);


template<typename SHAPEA_T, typename SHAPEB_T>
EPA_Result EPA_T(
    const GJKEPA_SupportGetter<SHAPEA_T>& support_getter_a,
    const GJKEPA_SupportGetter<SHAPEB_T>& support_getter_b,
    EPA_Context& ctx,
    const GJK_Simplex& simplex
) {
    if (!EPA_init(ctx, simplex)) {
        return EPA_Result{
            .valid = false,
            .normal = gfxm::vec3(0,0,0),
            .depth = .0f,
            .contact_a = gfxm::vec3(0,0,0),
            .contact_b = gfxm::vec3(0,0,0)
        };
    }

    const float EPS = 1e-3f;
    int iclosest = 0;
    EPA_Face face_closest;
    for (int i = 0; i < EPA_MAX_ITERATIONS; ++i) {
        iclosest = EPA_findClosestFaceToOrigin(ctx);
        face_closest = ctx.faces[iclosest];

        GJK_SupportPoint Ps = GJK_supportMinkowski_T(support_getter_a, support_getter_b, face_closest.normal);
        float dist = gfxm::dot(face_closest.normal, Ps.M);

        if(dist - face_closest.distance < EPS) {
            //EPA_debugDrawFaces(support_getter_a.getPosition(), ctx, 0xFFFFFF00);
            return EPA_finalize(ctx, ctx.faces[iclosest]);
        }

        if (!EPA_addPoint(ctx, Ps)) {
            break;
        }
    }

    //EPA_debugDrawFaces(support_getter_a.getPosition(), ctx, 0xFF0000FF);

    return EPA_finalize(ctx, face_closest);
    /*return EPA_Result{
        .valid = false,
        .normal = gfxm::vec3(0,0,0),
        .depth = .0f,
        .contact_a = gfxm::vec3(0,0,0),
        .contact_b = gfxm::vec3(0,0,0)
    };*/
}

