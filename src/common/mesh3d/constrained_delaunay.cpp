#include "constrained_delaunay.hpp"

#include <assert.h>
#include <algorithm>
#include <unordered_map>


static gfxm::vec2 lineLineIntersectPoint2d(const gfxm::vec2& A, const gfxm::vec2& B, const gfxm::vec2& C, const gfxm::vec2& D) {
    float a = B.y - A.y;
    float b = A.x - B.x;
    float c = a * A.x + b * A.y;

    float a1 = D.y - C.y;
    float b1 = C.x - D.x;
    float c1 = a1 * C.x + b1 * C.y;
    float det = a * b1 - a1 * b;
    if (det == .0f) {
        return gfxm::vec2(FLT_MAX, FLT_MAX);
    }
    else {
        return gfxm::vec2(
            (b1 * c - b * c1) / det,
            (a * c1 - a1 * c) / det
        );
    }
}
void cdtMakeSuperTriangle(const gfxm::vec2* vertices, int count, gfxm::vec2* oa, gfxm::vec2* ob, gfxm::vec2* oc, gfxm::rect* obounds) {
    (*obounds) = gfxm::rect(vertices[0], vertices[0]);
    gfxm::rect& rc = (*obounds);
    for (int i = 1; i < count; ++i) {
        gfxm::vec2 p = vertices[i];
        if (p.x < rc.min.x) {
            rc.min.x = p.x;
        } else if (p.x > rc.max.x) {
            rc.max.x = p.x;
        }
        if (p.y < rc.min.y) {
            rc.min.y = p.y;
        } else if (p.y > rc.max.y) {
            rc.max.y = p.y;
        }
    }
    gfxm::expand(rc, 20.0f);
    float rc_w = rc.max.x - rc.min.x;
    float rc_h = rc.max.y - rc.min.y;
    float rc_x_mid = rc.min.x + rc_w * .5f;
    gfxm::vec2 tri[] = {
        rc.min - gfxm::vec2(rc_w, .0f),
        lineLineIntersectPoint2d(gfxm::vec2(rc_x_mid, rc.min.y), gfxm::vec2(rc_x_mid, rc.max.y), rc.max, gfxm::vec2(rc.max.x + rc_w, rc.min.y)),
        gfxm::vec2(rc.max.x, rc.min.y) + gfxm::vec2(rc_w, .0f)
    };
    (*oa) = tri[0];
    (*ob) = tri[1];
    (*oc) = tri[2];
}
void cdtInit(cdtShape* ctx, const gfxm::vec2& super_a, const gfxm::vec2& super_b, const gfxm::vec2& super_c) {
    ctx->vertices.push_back(cdtPoint{ 0, super_a });
    ctx->vertices.push_back(cdtPoint{ 1, super_b });
    ctx->vertices.push_back(cdtPoint{ 2, super_c });
    cdtTriangle* t = new cdtTriangle;

    t->color = 0xAAFFFFFF;
    t->a = 0;
    t->b = 1;
    t->c = 2;
    t->ea = new cdtEdge(t->a, t->b, false, true);
    t->eb = new cdtEdge(t->b, t->c, false, true);
    t->ec = new cdtEdge(t->c, t->a, false, true);
    ctx->vertices[0].edges.insert(t->ea);
    ctx->vertices[0].edges.insert(t->ec);
    ctx->vertices[1].edges.insert(t->ea);
    ctx->vertices[1].edges.insert(t->eb);
    ctx->vertices[2].edges.insert(t->eb);
    ctx->vertices[2].edges.insert(t->ec);
    t->ea->tri_r = t;
    t->eb->tri_r = t;
    t->ec->tri_r = t;
    ctx->edges.push_back(t->ea);
    ctx->edges.push_back(t->eb);
    ctx->edges.push_back(t->ec);
    ctx->triangles.push_back(t);
}
void cdtEraseTrianglei(cdtShape* ctx, uint32_t i) {
    auto t = ctx->triangles[i];
    t->ea->tri_count--;
    (t->ea->tri_l == t) ? (t->ea->tri_l = 0) : (t->ea->tri_r = 0);
    t->eb->tri_count--;
    (t->eb->tri_l == t) ? (t->eb->tri_l = 0) : (t->eb->tri_r = 0);
    t->ec->tri_count--;
    (t->ec->tri_l == t) ? (t->ec->tri_l = 0) : (t->ec->tri_r = 0);
    delete t;
    ctx->triangles.erase(ctx->triangles.begin() + i);
}
void cdtEraseTriangle(cdtShape* ctx, cdtTriangle* t) {
    for (int i = 0; i < ctx->triangles.size(); ++i) {
        if (ctx->triangles[i] == t) {
            cdtEraseTrianglei(ctx, i);
            break;
        }
    }
}

static gfxm::vec2 circumcenter(const gfxm::vec2& a, const gfxm::vec2& b, const gfxm::vec2& c) {
    float D = (a.x * (b.y - c.y) + b.x * (c.y - a.y) + c.x * (a.y - b.y)) *2.f;
    float x = (1.f / D) * ((a.x * a.x + a.y * a.y) * (b.y - c.y) + (b.x * b.x + b.y * b.y) * (c.y - a.y) + (c.x * c.x + c.y * c.y) * (a.y - b.y));
    float y = (1.f / D) * ((a.x * a.x + a.y * a.y) * (c.x - b.x) + (b.x * b.x + b.y * b.y) * (a.x - c.x) + (c.x * c.x + c.y * c.y) * (b.x - a.x));

    return gfxm::vec2(x, y);
}
void cdtAddVertex(cdtShape* ctx, const gfxm::vec2& V) {
    std::vector<uint32_t> tris_to_delete;
    for (int i = 0; i < ctx->triangles.size(); ++i) {
        gfxm::vec2 C = circumcenter(
            ctx->vertices[ctx->triangles[i]->a].p,
            ctx->vertices[ctx->triangles[i]->b].p,
            ctx->vertices[ctx->triangles[i]->c].p
        );
        float R = gfxm::length(ctx->vertices[ctx->triangles[i]->a].p - C);
        float D = gfxm::length(V - C);
        if (D - R <= FLT_EPSILON) {
            tris_to_delete.push_back(i);
        }
    }
    if (tris_to_delete.empty()) {
        assert(false);
        return;
    }
    uint32_t prev_v = ctx->vertices.size() - 1;
    if (prev_v == 2) {
        prev_v = -1;
    }
    std::set<cdtEdge*> free_edges;
    for (int i = 0; i < tris_to_delete.size(); ++i) {
        auto& t = ctx->triangles[tris_to_delete[i]];
        free_edges.insert(t->ea);
        free_edges.insert(t->eb);
        free_edges.insert(t->ec);
    }
    std::sort(tris_to_delete.begin(), tris_to_delete.end(), [](const uint32_t& a, const uint32_t& b) {
        return a > b;
    });
    for (int i = 0; i < tris_to_delete.size(); ++i) {
        cdtEraseTrianglei(ctx, tris_to_delete[i]);
    }
    auto e = free_edges.begin();
    while (e != free_edges.end()) {
        if ((*e)->tri_count == 0 && !(*e)->is_super) { 
            for (int i = 0; i < ctx->edges.size(); ++i) {
                if (ctx->edges[i] == (*e)) {
                    ctx->vertices[ctx->edges[i]->p0].edges.erase(ctx->edges[i]);
                    ctx->vertices[ctx->edges[i]->p1].edges.erase(ctx->edges[i]);
                    delete ctx->edges[i];
                    ctx->edges.erase(ctx->edges.begin() + i);
                    break;
                }
            }
            e = free_edges.erase(e);
        } else {
            e++;
        }
    }
    static int col_seed = 0;
    col_seed++;
    std::set<uint32_t> free_verts;
    for (auto& edge : free_edges) {
        free_verts.insert(edge->p0);
        free_verts.insert(edge->p1);
    }
    std::unordered_map<uint32_t, cdtEdge*> vert_to_edge;
    for (auto& v : free_verts) {
        auto e = new cdtEdge(ctx->vertices.size(), v, false);
        e->color = 0xFFFFFFFF;
        if (v == prev_v) {
            e->is_constraining = true;
        }
        e->tri_count = 2;
        vert_to_edge[v] = e;
        ctx->edges.push_back(e);
    }
    for (auto& edge : free_edges) {
        cdtTriangle* tri = new cdtTriangle;
        tri->a = ctx->vertices.size();
        tri->b = edge->p0;
        tri->c = edge->p1;
        tri->ea = vert_to_edge[edge->p0];
        (tri->ea->tri_l == 0) ? (tri->ea->tri_l = tri) : (tri->ea->tri_r = tri);
        tri->eb = edge;// Edge{ tri.b, tri.c };
        (tri->eb->tri_l == 0) ? (tri->eb->tri_l = tri) : (tri->eb->tri_r = tri);
        edge->tri_count++;
        tri->ec = vert_to_edge[edge->p1];// new Edge(tri.c, tri.a, false);
        (tri->ec->tri_l == 0) ? (tri->ec->tri_l = tri) : (tri->ec->tri_r = tri);
            
        if (tri->ea->p1 == edge->p0) {
            ctx->vertices[edge->p0].edges.insert(tri->ea);
        } else if(tri->ea->p1 == edge->p1) {
            ctx->vertices[edge->p1].edges.insert(tri->ea);
        }
        if (tri->ec->p1 == edge->p0) {
            ctx->vertices[edge->p0].edges.insert(tri->ec);
        } else if (tri->ec->p1 == edge->p1) {
            ctx->vertices[edge->p1].edges.insert(tri->ec);
        }
            

        srand(col_seed);
        uint32_t col = 0xAA000000;
        col |= (rand() % 0x000000FF);
        col |= ((rand() % 0x000000FF) << 8);
        col |= ((rand() % 0x000000FF) << 16);
        tri->color = col;

        ctx->triangles.push_back(tri);
    }
    cdtPoint pt;
    pt.index = ctx->vertices.size();
    pt.p = V;
    for (auto& kv : vert_to_edge) {
        pt.edges.insert(kv.second);
    }
    ctx->vertices.push_back(pt);
}

static bool lineLineIntersect2d(const gfxm::vec2& A, const gfxm::vec2& B, const gfxm::vec2& C, const gfxm::vec2& D) {
    auto ccw = [](const gfxm::vec2& A, const gfxm::vec2& B, const gfxm::vec2& C)->bool {
        return (C.y - A.y) * (B.x - A.x) > (B.y - A.y) * (C.x - A.x);
    };
    return ccw(A, C, D) != ccw(B, C, D) && ccw(A, B, C) != ccw(A, B, D);
}
void cdtAddEdge(cdtShape* ctx, uint32_t a, uint32_t b) {
    auto is_left = [](const gfxm::vec2& A, const gfxm::vec2& B, const gfxm::vec2& C)->bool {
        // Checks if point C is to the left of AB. Which side is the "left" is not important
        return ((B.x - A.x) * (C.y - A.y) > (B.y - A.y) * (C.x - A.x));
    };
    auto get_edge_midpoint = [&](cdtEdge* e)->gfxm::vec2 {
        return ctx->vertices[e->p0].p + (ctx->vertices[e->p1].p - ctx->vertices[e->p0].p) * .5f;
    };

    a += 3;
    b += 3;
    if (gfxm::length(ctx->vertices[b].p - ctx->vertices[a].p) <= FLT_EPSILON) {
        return;
    }
    for (int i = 0; i < ctx->edges.size(); ++i) {
        if ((a == ctx->edges[i]->p0 && b == ctx->edges[i]->p1) || (a == ctx->edges[i]->p1 && b == ctx->edges[i]->p0)) {
            ctx->edges[i]->is_constraining = true;
            ctx->edges[i]->color = 0xFF00FF00;
            return;
        }
    }
    std::vector<cdtEdge*> poly_edges_left;
    std::vector<cdtEdge*> poly_edges_right;
    std::set<cdtEdge*> crossed_edges;
    std::set<cdtTriangle*> crossed_tris;
    for (int i = 0; i < ctx->edges.size(); ++i) {
        gfxm::vec2 A = ctx->vertices[ctx->edges[i]->p0].p;
        gfxm::vec2 B = ctx->vertices[ctx->edges[i]->p1].p;
        gfxm::vec2 C = ctx->vertices[a].p;
        gfxm::vec2 D = ctx->vertices[b].p;
        if (lineLineIntersect2d(A, B, C, D) && ctx->edges[i]->p0 != a && ctx->edges[i]->p0 != b && ctx->edges[i]->p1 != a && ctx->edges[i]->p1 != b) {
            //edges[i]->color = 0xFF0000FF;
            crossed_edges.insert(ctx->edges[i]);
            if (ctx->edges[i]->tri_l) {
                crossed_tris.insert(ctx->edges[i]->tri_l);
            }
            if (ctx->edges[i]->tri_r) {
                crossed_tris.insert(ctx->edges[i]->tri_r);
            }
        }
    }
    for (auto& t : crossed_tris) {
        cdtEdge* other = t->ea;
        if (crossed_edges.count(other) == 0) {
            if (is_left(ctx->vertices[a].p, ctx->vertices[b].p, get_edge_midpoint(other))) {
                poly_edges_left.push_back(other);
            } else {
                poly_edges_right.push_back(other);
            }
        }
        other = t->eb;
        if (crossed_edges.count(other) == 0) {
            if (is_left(ctx->vertices[a].p, ctx->vertices[b].p, get_edge_midpoint(other))) {
                poly_edges_left.push_back(other);
            } else {
                poly_edges_right.push_back(other);
            }
        }
        other = t->ec;
        if (crossed_edges.count(other) == 0) {
            if (is_left(ctx->vertices[a].p, ctx->vertices[b].p, get_edge_midpoint(other))) {
                poly_edges_left.push_back(other);
            } else {
                poly_edges_right.push_back(other);
            }
        }
        cdtEraseTriangle(ctx, t);
    }
    for (int i = 0; i < poly_edges_left.size(); ++i) {
        //poly_edges_left[i]->color = 0xFF00FFFF;
    }
    for (int i = 0; i < poly_edges_right.size(); ++i) {
        //poly_edges_right[i]->color = 0xFFFFFF00;
    }
    if (!poly_edges_left.empty() || !poly_edges_right.empty()) {
        //edges.push_back(new cdtEdge(a, b, false));
        //edges.back()->color = 0xFF00FF00;
    }
    // TODO: triangulate poly_edges
    for (auto& e : ctx->vertices[a].edges) {
        e->color = 0xFF0000FF;
    }
    for (auto& e : ctx->vertices[b].edges) {
        e->color = 0xFF0000FF;
    }
}


void cdtDbgInit(cdtDbgContext* dbg) {
    std::vector<gfxm::vec2> verts(dbg->points.size());
    for (int i = 0; i < dbg->points.size(); ++i) {
        verts[i] = dbg->points[i].p;
    }
    cdtMakeSuperTriangle(
        verts.data(), verts.size(),
        &dbg->super_triangle[0],
        &dbg->super_triangle[1],
        &dbg->super_triangle[2],
        &dbg->bounding_rect
    );
    cdtInit(&dbg->shape, dbg->super_triangle[0], dbg->super_triangle[1], dbg->super_triangle[2]);
    dbg->last_v2 = gfxm::vec2(FLT_MAX, FLT_MAX);
}
void cdtDbgStep(cdtDbgContext* dbg) {
    if (dbg->next_vertex_to_insert < dbg->points.size()) {
        cdtAddVertex(&dbg->shape, dbg->points[dbg->next_vertex_to_insert].p);
        dbg->last_vertex_inserted = dbg->shape.vertices.size() - 1;
        dbg->last_v2 = dbg->points[dbg->next_vertex_to_insert].p;
        dbg->next_vertex_to_insert++;
    } else if(dbg->next_edge_to_insert < dbg->edges.size()) {
        cdtAddEdge(&dbg->shape, dbg->edges[dbg->next_edge_to_insert].p0, dbg->edges[dbg->next_edge_to_insert].p1);
        dbg->next_edge_to_insert++;
    }
}