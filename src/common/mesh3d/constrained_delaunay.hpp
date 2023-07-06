#pragma once

#include <stdint.h>
#include "math/gfxm.hpp"
#include <set>
#include <vector>

struct cdtEdge;
struct cdtPoint {
    uint32_t index;
    gfxm::vec2 p;
    std::set<cdtEdge*> edges;
};
struct cdtTriangle;
struct cdtEdge {
    cdtEdge(uint32_t p0, uint32_t p1, bool is_constraining, bool is_super = false)
        : p0(p0), p1(p1), is_constraining(is_constraining), is_super(is_super) {
        tri_count = 1;
    }
    union {
        struct {
            uint32_t p0;
            uint32_t p1;
        };
        struct {
            uint64_t value;
        };
    };
    bool is_constraining;
    bool is_super;
    int tri_count;
    uint32_t color = 0xFFFFFFFF;
    cdtTriangle* tri_l = 0;
    cdtTriangle* tri_r = 0;
};
struct cdtTriangle {
    uint32_t a;
    uint32_t b;
    uint32_t c;
    uint32_t color;
    cdtEdge* ea;
    cdtEdge* eb;
    cdtEdge* ec;
};

struct cdtShape {
    std::vector<cdtPoint> vertices;
    std::vector<cdtEdge*> edges;
    std::vector<cdtTriangle*> triangles;
};

struct cdtDbgContext {
    cdtShape shape;
    std::vector<cdtPoint> points;
    std::vector<cdtEdge> edges;
    int next_vertex_to_insert = 0;
    int next_edge_to_insert = 0;
    int last_vertex_inserted = 0;
    gfxm::vec2 last_v2;
    gfxm::rect rc_bounds;
    gfxm::vec2 super_triangle[3];
};

void cdtMakeSuperTriangle(const gfxm::vec2* vertices, int count, gfxm::vec2* a, gfxm::vec2* b, gfxm::vec2* c, gfxm::rect* bounds);
void cdtInit(cdtShape* ctx, const gfxm::vec2& super_a, const gfxm::vec2& super_b, const gfxm::vec2& super_c);
void cdtEraseTrianglei(cdtShape* ctx, uint32_t i);
void cdtEraseTriangle(cdtShape* ctx, cdtTriangle* t);
void cdtAddVertex(cdtShape* ctx, const gfxm::vec2& V);
void cdtAddVertices(cdtShape* ctx, const gfxm::vec2& vertices, int count);
void cdtAddEdge(cdtShape* ctx, uint32_t a, uint32_t b);
void cdtAddEdges(cdtShape* ctx, const gfxm::tvec2<uint32_t>* edges, int count);

void cdtDbgInit(cdtDbgContext* dbg);
void cdtDbgStep(cdtDbgContext* dbg);