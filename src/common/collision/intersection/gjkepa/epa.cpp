#include "epa.hpp"

#include "gjk.hpp"
#include "log/log.hpp"
#include "debug_draw/debug_draw.hpp"


int EPA_findClosestFaceToOrigin(const EPA_Context& ctx) {
    const EPA_Face* faces = ctx.faces.data;
    const int face_count = ctx.faces.count;

    int iclosest = 0;
    float minDist = faces[0].distance;
    for(int i = 0; i < face_count; ++i) {
        if (faces[i].distance < minDist) {
            minDist = faces[i].distance;
            iclosest = i;
        }
    }
    return iclosest;
    
}
gfxm::vec3 EPA_closestPointOnTriangle(const gfxm::vec3& p,
    const gfxm::vec3& a,
    const gfxm::vec3& b,
    const gfxm::vec3& c
) {
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
}

void EPA_buildSimplexFaces(EPA_Context& ctx, const GJK_Simplex& simplex) {
    ctx.faces = {
        { .a = 0, .b = 1, .c = 2, .normal = gfxm::vec3(0,0,0), .distance = .0f },
        { .a = 0, .b = 3, .c = 1, .normal = gfxm::vec3(0,0,0), .distance = .0f },
        { .a = 1, .b = 3, .c = 2, .normal = gfxm::vec3(0,0,0), .distance = .0f },
        { .a = 2, .b = 3, .c = 0, .normal = gfxm::vec3(0,0,0), .distance = .0f }
    };

    for (int i = 0; i < ctx.faces.count; ++i) {
        EPA_Face& f = ctx.faces[i];
        gfxm::vec3 ab = ctx.points[f.b].M - ctx.points[f.a].M;
        gfxm::vec3 ac = ctx.points[f.c].M - ctx.points[f.a].M;
        f.normal = gfxm::normalize(gfxm::cross(ab, ac));

        if(gfxm::dot(f.normal, ctx.points[f.a].M) < 0) {
            f.normal = -f.normal;
            std::swap(f.b, f.c);
        }

        f.distance = gfxm::dot(f.normal, ctx.points[f.a].M);
    }
}

bool EPA_init(EPA_Context& ctx, const GJK_Simplex& simplex) {
    assert(simplex.count == 4);
    if(simplex.count != 4) {
        return false;
    }
    memcpy(ctx.points.data, simplex.points, simplex.count * sizeof(ctx.points[0]));
    ctx.points.count = 4;

    EPA_buildSimplexFaces(ctx, simplex);
    return true;
}

void EPA_debugDrawFaces(const gfxm::vec3& at, const EPA_Context& ctx, uint32_t color) {
    for (int j = 0; j < ctx.faces.count; ++j) {
        const auto& f = ctx.faces[j];
        gfxm::vec3 A_ = ctx.points[f.a].M;
        gfxm::vec3 B_ = ctx.points[f.b].M;
        gfxm::vec3 C_ = ctx.points[f.c].M;

        dbgDrawLine(at + A_, at + B_, color);
        dbgDrawLine(at + B_, at + C_, color);
        dbgDrawLine(at + C_, at + A_, color);      
    }
}

EPA_Result EPA_finalize(const EPA_Context& ctx, const EPA_Face& face) {
    gfxm::vec3 Am = ctx.points[face.a].M;
    gfxm::vec3 Bm = ctx.points[face.b].M;
    gfxm::vec3 Cm = ctx.points[face.c].M;
    gfxm::vec3 aAw = ctx.points[face.a].A;
    gfxm::vec3 aBw = ctx.points[face.b].A;
    gfxm::vec3 aCw = ctx.points[face.c].A;
    gfxm::vec3 bAw = ctx.points[face.a].B;
    gfxm::vec3 bBw = ctx.points[face.b].B;
    gfxm::vec3 bCw = ctx.points[face.c].B;

    gfxm::vec3 CPm = EPA_closestPointOnTriangle(gfxm::vec3(0,0,0), Am, Bm, Cm);
    gfxm::vec3 contact_a;
    gfxm::vec3 contact_b;
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
        float v = (d11 * d20 - d01 * d21) / denom;
        float w = (d00 * d21 - d01 * d20) / denom;
        float u = 1.0f - v - w;

        contact_a = aAw * u + aBw * v + aCw * w;
        contact_b = bAw * u + bBw * v + bCw * w;
    }

    return EPA_Result{
        .valid = true,
        .normal = face.normal,
        .depth = face.distance,
        .contact_a = contact_a,
        .contact_b = contact_b
    };
}

bool EPA_addPoint(EPA_Context& ctx, const GJK_SupportPoint& Ps) {
    ctx.hole_edges.clear();
    for(int j = 0; j < ctx.faces.count;) {
        const EPA_Face& face = ctx.faces[j];
        if(gfxm::dot(Ps.M - ctx.points[face.a].M, face.normal) > .0f) {
            // Removing this face
            EPA_Edge face_edges[3] = {
                EPA_Edge{ face.a, face.b },
                EPA_Edge{ face.b, face.c },
                EPA_Edge{ face.c, face.a }
            };

            for (auto& e : face_edges) {
                const EPA_Edge edge_to_find{e.b, e.a};
                int found_idx = -1;
                for(int k = 0; k < ctx.hole_edges.count; ++k) {
                    if(ctx.hole_edges[k] == edge_to_find) {
                        found_idx = k;
                        break;
                    }
                }
                if(found_idx >= 0) {
                    ctx.hole_edges.erase(found_idx);
                } else {
                    ctx.hole_edges.push_back(e);
                }
            }

            ctx.faces.erase(j);
        } else {
            ++j;
        }
    }

    int last_idx = ctx.points.count;
    ctx.points.push_back(Ps);
    for(int j = 0; j < ctx.hole_edges.count; ++j) {
        const auto& e = ctx.hole_edges[j];
        EPA_Face face = { 0 };
        face.a = e.a;
        face.b = e.b;
        face.c = last_idx;

        gfxm::vec3 AB = ctx.points[face.b].M - ctx.points[face.a].M;
        gfxm::vec3 AC = ctx.points[face.c].M - ctx.points[face.a].M;
        gfxm::vec3 N = gfxm::normalize(gfxm::cross(AB, AC));

        face.normal = N;
        face.distance = gfxm::dot(N, ctx.points[face.a].M);

        if (ctx.faces.count == ctx.faces.capacity()) {
            LOG_ERR("EPA ran out of face slots: " << ctx.faces.capacity());
            return false;
        }
        ctx.faces.push_back(face);
    }
    return true;
}

EPA_Result EPA(const SphereShapeGJK& A, const SphereShapeGJK& B, EPA_Context& ctx, const GJK_Simplex& simplex) {
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

        GJK_SupportPoint Ps = GJK_supportMinkowski(A, B, face_closest.normal);
        float dist = gfxm::dot(face_closest.normal, Ps.M);

        if(dist - face_closest.distance < EPS) {
            EPA_debugDrawFaces(A.pos, ctx, 0xFFFFFF00);
            return EPA_finalize(ctx, face_closest);
        }

        if (!EPA_addPoint(ctx, Ps)) {
            break;
        }
    }

    EPA_debugDrawFaces(A.pos, ctx, 0xFF0000FF);

    return EPA_finalize(ctx, face_closest);
    /*return EPA_Result{
        .valid = false,
        .normal = gfxm::vec3(0,0,0),
        .depth = .0f,
        .contact_a = gfxm::vec3(0,0,0),
        .contact_b = gfxm::vec3(0,0,0)
    };*/
}

