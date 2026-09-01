#include "sweep.hpp"

#include "collision/convex_mesh.hpp"


// Returns x0 only
static bool solveQuadratic_x0(float a, float b, float c, float& x0) {
    float disc = b * b - 4.0f * a * c;
    if (disc < .0f) {
        return false;
    }

    float sqrt_disc = gfxm::sqrt(disc);
    x0 = (-b - sqrt_disc) / (2.0f * a);
    return true;
}
static bool solveQuadratic(float a, float b, float c, float& x0, float& x1) {
    float disc = b * b - 4.0f * a * c;
    if (disc < .0f) {
        return false;
    }

    float sqrt_disc = gfxm::sqrt(disc);
    x0 = (-b - sqrt_disc) / (2.0f * a);
    x1 = (-b + sqrt_disc) / (2.0f * a);
    return true;
}


bool sweepSphereSphere(
    float sphere_radius, const gfxm::vec3& sphere_pos,
    const gfxm::vec3& from, const gfxm::vec3& to, float sweep_radius,
    SweepContactPoint& scp
) {
    gfxm::vec3 A = from;
    gfxm::vec3 B = to;
    gfxm::vec3 P = sphere_pos;
    
    auto AB = B - A;
    auto AP = P - A;
    float lenSqrAB = AB.length2();
    float t = (AP.x * AB.x + AP.y * AB.y + AP.z * AB.z) / lenSqrAB;
    t = gfxm::clamp(t, .0f, 1.f);
    gfxm::vec3 closest_pt_on_line = gfxm::lerp(A, B, t);

    gfxm::vec3 norm = closest_pt_on_line - sphere_pos;
    float center_distance = gfxm::length(norm);
    float radius_distance = sphere_radius + sweep_radius;
    float distance = center_distance - radius_distance;
    if (distance <= FLT_EPSILON) {
        gfxm::vec3 normal_a = norm / center_distance;
        gfxm::vec3 normal_b = -normal_a;
        gfxm::vec3 pt_a = normal_a * sphere_radius + sphere_pos;
        gfxm::vec3 pt_b = normal_b * sweep_radius + closest_pt_on_line;

        float sideA = (sphere_pos - closest_pt_on_line).length();
        float sideB = sphere_radius + sweep_radius;
        float offs = gfxm::sqrt(gfxm::pow2(sideB) - gfxm::pow2(sideA));
        gfxm::vec3 sweep_stop_pos = closest_pt_on_line - gfxm::normalize(closest_pt_on_line - from) * offs;
        scp.sweep_contact_pos = sweep_stop_pos;
        scp.contact = sweep_stop_pos + gfxm::normalize(sphere_pos - sweep_stop_pos) * sweep_radius;
        scp.distance_traveled = (to - from).length() * t - offs;
        scp.normal = gfxm::normalize(scp.contact - sphere_pos);
        scp.type = CONTACT_POINT_TYPE::DEFAULT;
        return true;
    }
    return false;
}

bool sweepSphereTriangle(
    const gfxm::vec3& from, const gfxm::vec3& to, float sweep_radius,
    const gfxm::vec3& p0, const gfxm::vec3& p1, const gfxm::vec3& p2,
    SweepContactPoint& out_scp
) {
    // Find triangle normal
    gfxm::vec3 pts[3] = {
        p0, p1, p2
    };
    gfxm::vec3 triangle_edges[3] = {
        p1 - p0, p2 - p1, p0 - p2
    };
    gfxm::vec3 PN = gfxm::cross(triangle_edges[0], triangle_edges[1]);
    PN = gfxm::normalize(PN);
    float side = 1.f;
    {
        float d = gfxm::dot((from - p0), PN);
        if (d < .0f) {
            PN = -PN;
            side = -side;
        }
    }

    // Test with triangle face
    const float PD = gfxm::dot(PN, p0 + PN * sweep_radius);
    const gfxm::vec3 V = (to - from);
    const float Vlen = V.length();
    float denom = gfxm::dot(PN, V);
    bool is_parallel = false;
    float t = .0f;
    if (abs(denom) <= FLT_EPSILON) {
        is_parallel = true;
    } else {
        gfxm::vec3 vec = PN * PD - from;
        t = gfxm::dot(vec, PN) / denom;
    }

    // TODO: Should handle parallel cases
    if (!is_parallel) {
        float distance_to_plane = sweep_radius;
        float distance_from_to_plane = gfxm::dot(PN, from - p0) / (PN.x * PN.x + PN.y * PN.y + PN.z * PN.z);
        gfxm::vec3 from_on_plane = from - PN * distance_from_to_plane;
        if (distance_from_to_plane <= sweep_radius) {
            t = .0f;
            distance_to_plane = distance_from_to_plane;
        }

        gfxm::vec3 intersection_pt = from + V * t;
        gfxm::vec3 c0 = gfxm::cross(intersection_pt - p0 + PN, triangle_edges[0]);
        gfxm::vec3 c1 = gfxm::cross(intersection_pt - p1 + PN, triangle_edges[1]);
        gfxm::vec3 c2 = gfxm::cross(intersection_pt - p2 + PN, triangle_edges[2]);
        bool is_inside = gfxm::dot(c0, PN) * side <= 0 && gfxm::dot(c1, PN) * side <= 0 && gfxm::dot(c2, PN) * side <= 0;
        if (is_inside && t >= .0f && t <= 1.f) {
            out_scp.sweep_contact_pos = intersection_pt;
            out_scp.distance_traveled = (intersection_pt - from).length();
            out_scp.contact = intersection_pt - PN * distance_to_plane;
            out_scp.normal = PN;
            out_scp.type = CONTACT_POINT_TYPE::TRIANGLE_FACE;
            //dbgDrawSphere(out_scp.contact, .1f, 0xFF00FFFF);
            //dbgDrawText(intersection_pt, "face");
            return true;
        }
    }

    // Triangle corners
    float t_corner = FLT_MAX;
    float closest_corner_dist = FLT_MAX;
    int corner_id = -1;
    for (int i = 0; i < 3; ++i) {
        const gfxm::vec3& corner = pts[i];
        float d = gfxm::dot(corner - from, gfxm::normalize(to - from));
        float tclosest = d / Vlen;

        // Closest point on an infinite line
        gfxm::vec3 closest_line = from + V * tclosest;
        float tclosest_segment = gfxm::clamp(tclosest, .0f, 1.f);
        // Closest point on the from-to segment
        gfxm::vec3 closest_segment = from + V * tclosest_segment;
        float dist = gfxm::length(pts[i] - closest_segment);
        if (dist > sweep_radius) {
            continue;
        }

        gfxm::vec3 m = from - corner;
        float a = gfxm::dot(V, V);
        float b = 2.0f * gfxm::dot(m, V);
        float c = gfxm::dot(m, m) - sweep_radius * sweep_radius;

        float t_entry = .0f;
        if (!solveQuadratic_x0(a, b, c, t_entry)) {
            continue;
        }

        t_entry = gfxm::clamp(t_entry, .0f, 1.f);
        if (t_entry < t_corner) {
            t_corner = t_entry;
            corner_id = i;
            float dist_ = gfxm::length(pts[i] - (from + V * t_entry));
            closest_corner_dist = dist_;
        }
    }

    // Triangle edges
    float closest_edge_dist = sweep_radius;
    gfxm::vec3 pt_on_edge;
    int edge_idx = -1;
    float t_edge = FLT_MAX;
    for (int i = 0; i < 3; ++i) {
        float r = sweep_radius;
        gfxm::vec3 v = to - from;
        gfxm::vec3 d = pts[(i + 1) % 3] - pts[i];
        gfxm::vec3 w = from - pts[i];

        float dd = gfxm::dot(d, d);
        if (dd < 1e-8f) {
            // Segment is degenerate,
            // will be covered by corner checks
            continue;
        }

        float vd = gfxm::dot(v, d);
        float wd = gfxm::dot(w, d);

        float a = gfxm::dot(v, v) - (vd * vd) / dd;
        float b = 2.f * (gfxm::dot(v, w) - (vd * wd) / dd);
        float c = gfxm::dot(w, w) - (wd * wd) / dd - r * r;

        float t1 = .0f;
        float t2 = .0f;
        if (!solveQuadratic(a, b, c, t1, t2)) {
            continue;
        }
        float t = FLT_MAX;
        if(t2 < t1) std::swap(t1, t2);
        if (c <= .0f) {
            // The hit is at t == 0
            t = .0f;
        } else if(t1 >= .0f && t1 <= 1.f) {
            t = t1;
        } else if(t2 >= .0f && t2 <= 1.f) {
            t = t2;
        }

        if (t == FLT_MAX) {
            continue;
        }

        gfxm::vec3 C = from + V * t;
        float s = gfxm::dot(C - pts[i], d) / dd;
        if (s < .0f || s > 1.f) {
            continue;
        }

        if (t < t_edge) {
            t_edge = t;
            edge_idx = i;
            pt_on_edge = pts[i] + d * s;//pts[i] + d * gfxm::dot(gfxm::normalize(d), V * t);
        }
    }

    if (corner_id >= 0 && t_corner < t_edge) {
        gfxm::vec3 pt = from + V * t_corner;
        out_scp.sweep_contact_pos = pt;
        out_scp.distance_traveled = t_corner * V.length();
        out_scp.contact = pts[corner_id];
        out_scp.normal = gfxm::normalize(pt - pts[corner_id]);
        out_scp.type = CONTACT_POINT_TYPE::TRIANGLE_CORNER;
        //dbgDrawText(from, std::format("CORNER t: {:.3f}, c: {}", t_corner, corner_id).c_str(), 0xFFFF00FF);
        //dbgDrawText(pt, "corner");
        return true;
    } else if (edge_idx >= 0) {
        gfxm::vec3 pt = from + V * t_edge;
        out_scp.sweep_contact_pos = pt;
        out_scp.distance_traveled = t_edge * V.length();
        out_scp.contact = pt_on_edge;
        out_scp.normal = gfxm::normalize(pt - pt_on_edge);
        out_scp.type = CONTACT_POINT_TYPE::TRIANGLE_EDGE;
        //dbgDrawText(from, std::format("\nEDGE t: {:.3f}, c: {}", t_edge, edge_idx).c_str(), 0xFFFF0000);
        //dbgDrawText(pt, "edge");
        return true;
    }

    return false;
}


static bool sweepCylinderVertex(
    const gfxm::vec3& capA, const gfxm::vec3& dir,
    const gfxm::vec3& velocity, const gfxm::vec3& P,
    float radius, float& t, gfxm::vec3& N_out
) {
    gfxm::vec3 rc = capA - P;
    gfxm::vec3 VcrossD = gfxm::cross(velocity, dir);
    gfxm::vec3 RCcrossD = gfxm::cross(rc, dir);
    float a = gfxm::dot(VcrossD, VcrossD);
    float b = 2.0f * gfxm::dot(VcrossD, RCcrossD);
    float c = gfxm::dot(RCcrossD, RCcrossD) - (radius * radius * gfxm::dot(dir, dir));
    if (solveQuadratic_x0(a, b, c, t)) {
        if (t < .0f || t > 1.f) return false; // outside the 
        gfxm::vec3 movedCapA = capA + velocity * t;
        float tAxis = gfxm::dot(P - movedCapA, dir) / gfxm::dot(dir, dir);
        if (tAxis < 0.0f || tAxis > 1.0f) return false; // Outside the "tube"
        gfxm::vec3 ptOnAxis = movedCapA + dir * tAxis;
        N_out = gfxm::normalize(ptOnAxis - P);
        return true;
    }
    return false;
}

static bool sweepCylinderEdge(
    const gfxm::vec3& capA, const gfxm::vec3& capDir,
    const gfxm::vec3& velo,
    const gfxm::vec3& edgeA, const gfxm::vec3& edgeDir,
    float radius, float& t, gfxm::vec3& N_out, gfxm::vec3& CP_out
) {
    gfxm::vec3 crossDirs = gfxm::cross(capDir, edgeDir);
    float crossLen2 = gfxm::dot(crossDirs, crossDirs);
    if (crossLen2 < 1e-6f) {
        float dd = gfxm::dot(capDir, capDir);
        if (dd < 1e-9f) {
            return false; // capsule axis too short, degenerate
        }

        gfxm::vec3 w0 = capA - edgeA;
        gfxm::vec3 w0_perp = w0 - capDir * (gfxm::dot(w0, capDir) / dd);
        gfxm::vec3 velo_perp = velo - capDir * (gfxm::dot(velo, capDir) / dd);
        
        float a = gfxm::dot(velo_perp, velo_perp);
        float b = 2.0f * gfxm::dot(velo_perp, w0_perp);
        float c = gfxm::dot(w0_perp, w0_perp) - radius * radius;
        float t_hit = 1.0f;
        if (!solveQuadratic_x0(a, b, c, t_hit)) {
            return false;
        }
        if(t_hit < .0f || t_hit > 1.f) {
            return false;
        }

        gfxm::vec3 movedCapA = capA + velo * t_hit;
        gfxm::vec3 perp = w0_perp + velo_perp * t_hit;

        float dvv = gfxm::dot(edgeDir, edgeDir);
        if(dvv < 1e-9f) return false;
        float tc = gfxm::dot(movedCapA - edgeA, edgeDir) / dvv;
        if(tc < .0f || tc > 1.f) return false; // outside edge segment

        gfxm::vec3 ptOnEdge = edgeA + edgeDir * tc;
        float sc = gfxm::dot(ptOnEdge - movedCapA, capDir) / dd;
        if(sc < .0f || sc > 1.f) return false; // outside cylinder segment, for capsule sphere caps to handle

        t = t_hit;
        N_out = gfxm::normalize(perp);
        CP_out = ptOnEdge;
        return true;
    }

    const float invCrossLen2 = 1.0f / crossLen2;
    gfxm::vec3 startDiff = capA - edgeA;
    float a = powf(gfxm::dot(velo, crossDirs), 2.0f) * invCrossLen2;
    float b = 2.0f * gfxm::dot(velo, crossDirs) * gfxm::dot(startDiff, crossDirs) * invCrossLen2;
    float c = powf(gfxm::dot(startDiff, crossDirs), 2.0f) * invCrossLen2 - radius * radius;
    if (solveQuadratic_x0(a, b, c, t)) {
        if (t < .0f || t > 1.f) return false; // TODO: Check if it's redundant

        gfxm::vec3 movedCapA = capA + velo * t;
        
        gfxm::vec3 u = capDir;
        gfxm::vec3 v = edgeDir;
        gfxm::vec3 w = movedCapA - edgeA;
        
        float duv = gfxm::dot(u, v);
        float duu = gfxm::dot(u, u);
        float dvv = gfxm::dot(v, v);
        float duw = gfxm::dot(u, w);
        float dvw = gfxm::dot(v, w);
        
        float denom = duu * dvv - duv * duv;
        if (std::abs(denom) < 1e-6f) return false;
        
        float sc = (duv * dvw - dvv * duw) / denom; // parameter along cylinder axis
        float tc = (duu * dvw - duv * duw) / denom; // parameter along edge line
        
        if (tc < 0.0f || tc > 1.0f || sc < 0.0f || sc > 1.0f) return false;
        
        gfxm::vec3 ptOnCylinderAxis = movedCapA + capDir * sc;
        gfxm::vec3 ptOnEdge = edgeA + edgeDir * tc;
        
        N_out = gfxm::normalize(ptOnCylinderAxis - ptOnEdge);
        CP_out = ptOnEdge;

        return true;
    }
    return false;
}

// Not a real cylinder, used for mid section of the capsule
static bool sweepCylinderTriangle(
    const gfxm::vec3& capA, const gfxm::vec3& capB, const gfxm::vec3& velo,
    const gfxm::vec3& p0, const gfxm::vec3& p1, const gfxm::vec3& p2,
    float radius, float& t_out, gfxm::vec3& contact_out, gfxm::vec3& N_out
) {
    gfxm::vec3 capDir = capB - capA;
    float t_min = 1.0f;
    bool hit = false;

    gfxm::vec3 points[3] = { p0, p1, p2 };
    for (const auto& p : points) {
        float t = 1.0f;
        gfxm::vec3 N;
        if (sweepCylinderVertex(capA, capDir, velo, p, radius, t, N)) {
            if (t < t_min) {
                t_min = t;
                contact_out = p;
                N_out = N;
                hit = true;
            }
        }
    }

    gfxm::vec3 edges[3][2] = { { p0, p1 }, { p1, p2 }, { p2, p0 } };
    for (const auto& edge : edges) {
        gfxm::vec3 edgeDir = edge[1] - edge[0];
        float t = 1.0f;
        gfxm::vec3 N;
        gfxm::vec3 CP;
        if (sweepCylinderEdge(capA, capDir, velo, edge[0], edgeDir, radius, t, N, CP)) {
            if (t < t_min) {
                t_min = t;
                contact_out = CP;
                N_out = N;
                hit = true; 
            }
        }
    }

    if (hit) {
        t_out = t_min;
        return true;
    }
    return false;
}

bool sweepCapsuleTriangle(
    const gfxm::vec3& A, const gfxm::vec3& B, float radius,
    const gfxm::vec3& V,
    const gfxm::vec3& p0, const gfxm::vec3& p1, const gfxm::vec3& p2,
    SweepContactPoint& out_scp
) {
    float max_dist2 = V.length2();
    gfxm::vec3 A_from = A;
    gfxm::vec3 A_to = A + V;
    gfxm::vec3 B_from = B;
    gfxm::vec3 B_to = B + V;

    bool has_hit = false;
    float best_distance = FLT_MAX;
    SweepContactPoint scp{};
    if (sweepSphereTriangle(A_from, A_to, radius, p0, p1, p2, scp)
        && scp.distance_traveled < best_distance) {
        best_distance = scp.distance_traveled;
        out_scp = scp;
        out_scp.sweep_contact_pos = A + (B - A) * .5f + gfxm::normalize(V) * scp.distance_traveled;
        has_hit = true;
    }
    if (sweepSphereTriangle(B_from, B_to, radius, p0, p1, p2, scp)
        && scp.distance_traveled < best_distance) {
        best_distance = scp.distance_traveled;
        out_scp = scp;
        out_scp.sweep_contact_pos = A + (B - A) * .5f + gfxm::normalize(V) * scp.distance_traveled;
        has_hit = true;
    }
    
    float t = 1.0f;
    gfxm::vec3 CP;
    gfxm::vec3 N;
    if (sweepCylinderTriangle(A_from, B_from, V, p0, p1, p2, radius, t, CP, N)) {
        if(t >= .0f && t <= 1.f) {
            float dist = gfxm::sqrt(max_dist2) * t;
            if (dist < best_distance) {
                best_distance = dist;
                out_scp.distance_traveled = dist;
                out_scp.sweep_contact_pos = A + (B - A) * .5f + V * t;
                out_scp.contact = CP;
                out_scp.normal = N;
                has_hit = true;
            }
        }
    }

    return has_hit;
}


#include "collision/collision_triangle_mesh.hpp"

bool sweepSphereTriangleMesh(
    const gfxm::vec3& from, const gfxm::vec3& to, float sweep_radius,
    const CollisionTriangleMesh* mesh,
    SweepContactPoint& scp
) {

    struct TriangleMeshSweepContext {
        gfxm::vec3 direction;
        SweepContactPoint pt;
        bool hasHit = false;
    } ctx;
    ctx.direction = gfxm::normalize(to - from);
    ctx.hasHit = false;
    ctx.pt.distance_traveled = INFINITY;
    mesh->sweepSphereTest(from, to, sweep_radius, &ctx, [](void* context, const SweepContactPoint& scp) {
        TriangleMeshSweepContext* ctx = (TriangleMeshSweepContext*)context;
        // Ignore surfaces we're moving away from
        // TODO: Not sure if this should be filtered unconditionally
        // maybe make it an option?
        if (gfxm::dot(ctx->direction, scp.normal) >= .0f) {
            return;
        }
        ctx->hasHit = true;    
        /*
        if (scp.distance_traveled <= ctx->pt.distance_traveled) {
            if (scp.distance_traveled <= FLT_EPSILON) {
                if (ctx->pt.type == CONTACT_POINT_TYPE::TRIANGLE_FACE && scp.type != CONTACT_POINT_TYPE::TRIANGLE_FACE) {
                    return;
                }
            }

            ctx->pt = scp;
        }*/
        if (scp.distance_traveled < ctx->pt.distance_traveled) {
            ctx->pt = scp;
        }
    });
    scp = ctx.pt;
    return ctx.hasHit;
}

bool sweepCapsuleTriangleMesh(
    const gfxm::vec3& capA, const gfxm::vec3& capB, float radius, const gfxm::vec3& V,
    const CollisionTriangleMesh* mesh, SweepContactPoint& scp
) {
    struct Context {
        gfxm::vec3 direction;
        SweepContactPoint pt;
        bool hasHit = false;
    } ctx;
    ctx.direction = gfxm::normalize(V);
    ctx.hasHit = false;
    ctx.pt.distance_traveled = INFINITY;
    mesh->sweepCapsule(capA, capB, radius, V, &ctx, [](void* context, const SweepContactPoint& scp) {
        Context* ctx = (Context*)context;
        // Ignore surfaces we're moving away from
        // TODO: Not sure if this should be filtered unconditionally
        // maybe make it an option?
        if (gfxm::dot(ctx->direction, scp.normal) >= .0f) {
            return;
        }
        ctx->hasHit = true;

        if (scp.distance_traveled < ctx->pt.distance_traveled) {
            ctx->pt = scp;
        }
    });
    scp = ctx.pt;
    return ctx.hasHit;
}

bool sweepCapsuleConvexMesh(
    const gfxm::vec3& capA, const gfxm::vec3& capB, float radius, const gfxm::vec3& V,
    const phyConvexMesh* mesh, SweepContactPoint& scp
) {
    struct Context {
        gfxm::vec3 direction;
        SweepContactPoint pt;
        bool hasHit = false;
    } ctx;
    ctx.direction = gfxm::normalize(V);
    ctx.hasHit = false;
    ctx.pt.distance_traveled = INFINITY;
    mesh->sweepCapsule(capA, capB, radius, V, &ctx, [](void* context, const SweepContactPoint& scp) {
        Context* ctx = (Context*)context;
        // Ignore surfaces we're moving away from
        // TODO: Not sure if this should be filtered unconditionally
        // maybe make it an option?
        if (gfxm::dot(ctx->direction, scp.normal) >= .0f) {
            return;
        }
        ctx->hasHit = true;

        if (scp.distance_traveled < ctx->pt.distance_traveled) {
            ctx->pt = scp;
        }
    });
    scp = ctx.pt;
    return ctx.hasHit;
}

bool sweepSphereConvexMesh(
    const gfxm::vec3& from,
    const gfxm::vec3& to,
    float sweep_radius,
    const phyConvexMesh* mesh,
    SweepContactPoint& scp
) {
    struct Context {
        SweepContactPoint pt;
        bool hasHit = false;
    } ctx;
    ctx.hasHit = false;
    ctx.pt.distance_traveled = INFINITY;
    mesh->sweptSphereTest(from, to, sweep_radius, &ctx, [](void* context, const SweepContactPoint& scp) {   
        Context* ctx = (Context*)context;
        ctx->hasHit = true;
        if (scp.distance_traveled <= ctx->pt.distance_traveled) {
            // TODO: ?
            /*
            if (scp.distance_traveled <= FLT_EPSILON) {
                if (ctx->pt.type == CONTACT_POINT_TYPE::TRIANGLE_FACE && scp.type != CONTACT_POINT_TYPE::TRIANGLE_FACE) {
                    return;
                }
            }*/
            ctx->pt = scp;
        }
    });
    scp = ctx.pt;
    return ctx.hasHit;
}


#include "collision/shape/heightfield.hpp"

bool sweepSphereHeightfield(
    const gfxm::vec3& from,
    const gfxm::vec3& to,
    float sweep_radius,
    const phyHeightfieldShape* heightfield,
    SweepContactPoint& scp
) {
    float minx = gfxm::_min(from.x - sweep_radius, to.x - sweep_radius);
    float maxx = gfxm::_max(from.x + sweep_radius, to.x + sweep_radius);
    float minz = gfxm::_min(from.z - sweep_radius, to.z - sweep_radius);
    float maxz = gfxm::_max(from.z + sweep_radius, to.z + sweep_radius);
    float miny = gfxm::_min(from.y - sweep_radius, to.y - sweep_radius);
    float maxy = gfxm::_max(from.y + sweep_radius, to.y + sweep_radius);

    float cell_width = heightfield->getCellWidth();
    float cell_depth = heightfield->getCellDepth();

    int sminx = gfxm::clamp(minx / cell_width, .0f, heightfield->getSampleCountX() - 1);
    int smaxx = gfxm::clamp(1 + maxx / cell_width, .0f, heightfield->getSampleCountX() - 1);
    int sminz = gfxm::clamp(minz / cell_depth, .0f, heightfield->getSampleCountZ() - 1);
    int smaxz = gfxm::clamp(1 + maxz / cell_depth, .0f, heightfield->getSampleCountZ() - 1);

    if (sminx == smaxx || sminz == smaxz) {
        return false;
    }

    {
        gfxm::vec3 p0(minx, .0f, minz);
        gfxm::vec3 p1(maxx, .0f, minz);
        gfxm::vec3 p2(maxx, .0f, maxz);
        gfxm::vec3 p3(minx, .0f, maxz);

        dbgDrawLine(p0, p1, 0xFFFF00FF);
        dbgDrawLine(p1, p2, 0xFFFF00FF);
        dbgDrawLine(p2, p3, 0xFFFF00FF);
        dbgDrawLine(p3, p0, 0xFFFF00FF);
    }
    {
        gfxm::vec3 p0(sminx * cell_width, .0f, sminz * cell_depth);
        gfxm::vec3 p1(smaxx * cell_width, .0f, sminz * cell_depth);
        gfxm::vec3 p2(smaxx * cell_width, .0f, smaxz * cell_depth);
        gfxm::vec3 p3(sminx * cell_width, .0f, smaxz * cell_depth);

        dbgDrawLine(p0, p1, 0xFF0000FF);
        dbgDrawLine(p1, p2, 0xFF0000FF);
        dbgDrawLine(p2, p3, 0xFF0000FF);
        dbgDrawLine(p3, p0, 0xFF0000FF);
    }

    int scount_x = heightfield->getSampleCountX();

    const float* samples = heightfield->getData();
    float min_distance = gfxm::length(to - from);
    bool hasHit = false;
    for (int z = sminz; z < smaxz; ++z) {
        for (int x = sminx; x < smaxx; ++x) {
            float h0 = samples[x + z * scount_x];
            float h1 = samples[x + (z + 1) * scount_x];
            float h2 = samples[x + 1 + (z + 1) * scount_x];
            float h3 = samples[x + 1 + z * scount_x];

            float hmin = gfxm::_min(gfxm::_min(h0, h1), gfxm::_min(h2, h3));
            float hmax = gfxm::_max(gfxm::_max(h0, h1), gfxm::_max(h2, h3));

            if ((hmin < miny && hmax < miny)
                || (hmin > maxy && hmax > maxy)
            ) {
                continue;
            }

            gfxm::vec3 p0(x * cell_width, h0, z * cell_depth);
            gfxm::vec3 p1(x * cell_width, h1, (z + 1) * cell_depth);
            gfxm::vec3 p2((x + 1) * cell_width, h2, (z + 1) * cell_depth);
            gfxm::vec3 p3((x + 1) * cell_width, h3, z * cell_depth);
            
            dbgDrawLine(p0, p1, 0xFFFFFFFF);
            dbgDrawLine(p1, p2, 0xFFFFFFFF);
            dbgDrawLine(p3, p0, 0xFFFFFFFF);

            dbgDrawLine(p2, p3, 0xFFFFFFFF);
            dbgDrawLine(p3, p0, 0xFFFFFFFF);
            dbgDrawLine(p0, p2, 0xFFFFFFFF);
            
            SweepContactPoint scp2;
            if (sweepSphereTriangle(from, to, sweep_radius, p0, p1, p2, scp2)) {
                if (min_distance > scp2.distance_traveled) {
                    min_distance = scp2.distance_traveled;
                    scp = scp2;
                    hasHit = true;
                    scp.prop = phySurfaceProp{ COLLISION_SURFACE_GRAVEL };
                }
            }
            if (sweepSphereTriangle(from, to, sweep_radius, p2, p3, p0, scp2)) {
                if (min_distance > scp2.distance_traveled) {
                    min_distance = scp2.distance_traveled;
                    scp = scp2;
                    hasHit = true;
                    scp.prop = phySurfaceProp{ COLLISION_SURFACE_GRAVEL };
                }
            }
        }
    }
    
    return hasHit;
}

bool sweepCapsuleHeightfield(
    const gfxm::vec3& capA, const gfxm::vec3& capB, float radius, const gfxm::vec3& V,
    const phyHeightfieldShape* heightfield,
    SweepContactPoint& scp,
    const gfxm::mat4& shape_transform
) {
    gfxm::aabb bbox = gfxm::make_aabb_sphere(capA, radius);
    bbox = gfxm::aabb_union(bbox, gfxm::make_aabb_sphere(capB, radius));
    bbox = gfxm::aabb_union(bbox, gfxm::make_aabb_sphere(capA + V, radius));
    bbox = gfxm::aabb_union(bbox, gfxm::make_aabb_sphere(capB + V, radius));
    float minx = bbox.from.x;
    float maxx = bbox.to.x;
    float minz = bbox.from.z;
    float maxz = bbox.to.z;
    float miny = bbox.from.y;
    float maxy = bbox.to.y;

    float cell_width = heightfield->getCellWidth();
    float cell_depth = heightfield->getCellDepth();

    int sminx = gfxm::clamp(minx / cell_width, .0f, heightfield->getSampleCountX() - 1);
    int smaxx = gfxm::clamp(1 + maxx / cell_width, .0f, heightfield->getSampleCountX() - 1);
    int sminz = gfxm::clamp(minz / cell_depth, .0f, heightfield->getSampleCountZ() - 1);
    int smaxz = gfxm::clamp(1 + maxz / cell_depth, .0f, heightfield->getSampleCountZ() - 1);

    if (sminx == smaxx || sminz == smaxz) {
        return false;
    }

    {
        gfxm::vec3 p0(minx, .0f, minz);
        gfxm::vec3 p1(maxx, .0f, minz);
        gfxm::vec3 p2(maxx, .0f, maxz);
        gfxm::vec3 p3(minx, .0f, maxz);
        gfxm::vec3 wP0 = shape_transform * gfxm::vec4(p0, 1.f);
        gfxm::vec3 wP1 = shape_transform * gfxm::vec4(p1, 1.f);
        gfxm::vec3 wP2 = shape_transform * gfxm::vec4(p2, 1.f);
        gfxm::vec3 wP3 = shape_transform * gfxm::vec4(p3, 1.f);

        dbgDrawLine(wP0, wP1, 0xFFFF00FF);
        dbgDrawLine(wP1, wP2, 0xFFFF00FF);
        dbgDrawLine(wP2, wP3, 0xFFFF00FF);
        dbgDrawLine(wP3, wP0, 0xFFFF00FF);
    }
    {
        gfxm::vec3 p0(sminx * cell_width, .0f, sminz * cell_depth);
        gfxm::vec3 p1(smaxx * cell_width, .0f, sminz * cell_depth);
        gfxm::vec3 p2(smaxx * cell_width, .0f, smaxz * cell_depth);
        gfxm::vec3 p3(sminx * cell_width, .0f, smaxz * cell_depth);
        gfxm::vec3 wP0 = shape_transform * gfxm::vec4(p0, 1.f);
        gfxm::vec3 wP1 = shape_transform * gfxm::vec4(p1, 1.f);
        gfxm::vec3 wP2 = shape_transform * gfxm::vec4(p2, 1.f);
        gfxm::vec3 wP3 = shape_transform * gfxm::vec4(p3, 1.f);

        dbgDrawLine(wP0, wP1, 0xFF0000FF);
        dbgDrawLine(wP1, wP2, 0xFF0000FF);
        dbgDrawLine(wP2, wP3, 0xFF0000FF);
        dbgDrawLine(wP3, wP0, 0xFF0000FF);
    }

    int scount_x = heightfield->getSampleCountX();

    const float* samples = heightfield->getData();
    float min_distance = gfxm::length(V);
    bool hasHit = false;
    for (int z = sminz; z < smaxz; ++z) {
        for (int x = sminx; x < smaxx; ++x) {
            float h0 = samples[x + z * scount_x];
            float h1 = samples[x + (z + 1) * scount_x];
            float h2 = samples[x + 1 + (z + 1) * scount_x];
            float h3 = samples[x + 1 + z * scount_x];

            float hmin = gfxm::_min(gfxm::_min(h0, h1), gfxm::_min(h2, h3));
            float hmax = gfxm::_max(gfxm::_max(h0, h1), gfxm::_max(h2, h3));

            if ((hmin < miny && hmax < miny)
                || (hmin > maxy && hmax > maxy)
            ) {
                continue;
            }

            gfxm::vec3 p0(x * cell_width, h0, z * cell_depth);
            gfxm::vec3 p1(x * cell_width, h1, (z + 1) * cell_depth);
            gfxm::vec3 p2((x + 1) * cell_width, h2, (z + 1) * cell_depth);
            gfxm::vec3 p3((x + 1) * cell_width, h3, z * cell_depth);
            gfxm::vec3 wP0 = shape_transform * gfxm::vec4(p0, 1.f);
            gfxm::vec3 wP1 = shape_transform * gfxm::vec4(p1, 1.f);
            gfxm::vec3 wP2 = shape_transform * gfxm::vec4(p2, 1.f);
            gfxm::vec3 wP3 = shape_transform * gfxm::vec4(p3, 1.f);
            
            dbgDrawLine(wP0, wP1, 0xFFFFFFFF);
            dbgDrawLine(wP1, wP2, 0xFFFFFFFF);
            dbgDrawLine(wP3, wP0, 0xFFFFFFFF);

            dbgDrawLine(wP2, wP3, 0xFFFFFFFF);
            dbgDrawLine(wP3, wP0, 0xFFFFFFFF);
            dbgDrawLine(wP0, wP2, 0xFFFFFFFF);

            SweepContactPoint scp2;
            if (sweepCapsuleTriangle(capA, capB, radius, V, p0, p1, p2, scp2)) {
                if (min_distance > scp2.distance_traveled) {
                    min_distance = scp2.distance_traveled;
                    scp = scp2;
                    hasHit = true;
                    // TODO: !!!
                    scp.prop = phySurfaceProp{ COLLISION_SURFACE_GRAVEL };
                }
            }
            if (sweepCapsuleTriangle(capA, capB, radius, V, p2, p3, p0, scp2)) {
                if (min_distance > scp2.distance_traveled) {
                    min_distance = scp2.distance_traveled;
                    scp = scp2;
                    hasHit = true;
                    // TODO: !!!
                    scp.prop = phySurfaceProp{ COLLISION_SURFACE_GRAVEL };
                }
            }
        }
    }
    
    return hasHit;
}

