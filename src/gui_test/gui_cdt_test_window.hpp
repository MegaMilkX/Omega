#pragma once

#include "gui//elements/gui_window.hpp"

#include "nanosvg/nanosvg.h"
#include "nanosvg/nanosvgrast.h"

#include "CDT/include/CDT.h"

#include "math/bezier.hpp"

#include "mesh3d/constrained_delaunay.hpp"

class GuiCdtTest : public GuiElement {
    struct Edge;
    struct Point {
        uint32_t index;
        gfxm::vec2 p;
        std::set<Edge*> edges;
    };
    struct Triangle;
    struct Edge {
        Edge(uint32_t p0, uint32_t p1, bool is_constraining, bool is_super = false)
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
        Triangle* tri_l = 0;
        Triangle* tri_r = 0;
    };
    struct Triangle {
        uint32_t a;
        uint32_t b;
        uint32_t c;
        uint32_t color;
        Edge* ea;
        Edge* eb;
        Edge* ec;
    };
    struct Path {
        std::vector<gfxm::vec3> points;
    };

    struct Shape {
        std::vector<gfxm::vec3> points;
        std::vector<Edge> edges;
        std::vector<CDT::V2d<double>> cdt_points;
        CDT::EdgeVec cdt_edges;
        std::vector<gfxm::vec3> vertices3;
        std::vector<uint32_t> indices;
        gfxm::rect bounding_rect;
        gfxm::vec2 super_triangle[3];
        uint32_t color;
    };
    std::vector<Shape> shapes;

    cdtDbgContext cdtdbg;
    struct NSVGimage* svg = 0;

    gfxm::vec2 circumcenter(const gfxm::vec2& a, const gfxm::vec2& b, const gfxm::vec2& c) {
        float D = (a.x * (b.y - c.y) + b.x * (c.y - a.y) + c.x * (a.y - b.y)) *2.f;
        float x = (1.f / D) * ((a.x * a.x + a.y * a.y) * (b.y - c.y) + (b.x * b.x + b.y * b.y) * (c.y - a.y) + (c.x * c.x + c.y * c.y) * (a.y - b.y));
        float y = (1.f / D) * ((a.x * a.x + a.y * a.y) * (c.x - b.x) + (b.x * b.x + b.y * b.y) * (a.x - c.x) + (c.x * c.x + c.y * c.y) * (b.x - a.x));

        return gfxm::vec2(x, y);
    }
    bool lineLineIntersect2d(const gfxm::vec2& A, const gfxm::vec2& B, const gfxm::vec2& C, const gfxm::vec2& D) {
        auto ccw = [](const gfxm::vec2& A, const gfxm::vec2& B, const gfxm::vec2& C)->bool {
            return (C.y - A.y) * (B.x - A.x) > (B.y - A.y) * (C.x - A.x);
        };
        return ccw(A, C, D) != ccw(B, C, D) && ccw(A, B, C) != ccw(A, B, D);
    }
    gfxm::vec2 lineLineIntersectPoint2d(const gfxm::vec2& A, const gfxm::vec2& B, const gfxm::vec2& C, const gfxm::vec2& D) {
        float a = B.y - A.y;
        float b = A.x - B.x;
        float c = a * A.x + b * A.y;

        float a1 = D.y - C.y;
        float b1 = C.x - D.x;
        float c1 = a1 * C.x + b1 * C.y;
        float det = a * b1 - a1 * b;
        if (det == .0f) {
            return gfxm::vec2(FLT_MAX, FLT_MAX);
        } else {
            return gfxm::vec2(
                (b1 * c - b * c1) / det,
                (a * c1 - a1 * c) / det
            );
        }
    }


    inline void pathRemoveDuplicates(std::vector<gfxm::vec3>& points, std::vector<Edge>& edges) {
        if (points.size() < 2) {
            return;
        }
        int key = 1;
    
        auto erasePoint = [&](int k) {
            points.erase(points.begin() + key);
            for (int i = 0; i < edges.size(); ++i) {
                if (edges[i].p0 > k) {
                    edges[i].p0--;
                }
                if (edges[i].p1 > k) {
                    edges[i].p1--;
                }
            }
        };
    
        gfxm::vec3 ref = gfxm::normalize(points[1] - points[0]);
        while (key < points.size() - 1) {
            gfxm::vec3& p0 = points[key];
            gfxm::vec3& p1 = points[key + 1];
            if (gfxm::length(p1 - p0) <= FLT_EPSILON) {
                points.erase(points.begin() + key);
                continue;
            } else {
                ++key;
            }
        }
    }
public:
    GuiCdtTest() {
        svg = nsvgParseFromFile("svg/test-logo.svg", "px", 72);
        if (svg) {
            float ratio = svg->width / svg->height;
            float wmul = 500.0f / svg->width;
            float hmul = 500.0f / svg->height;
            hmul *= 1.f / ratio;
            for (NSVGshape* shape = svg->shapes; shape != 0; shape = shape->next) {
                uint32_t color = 0xFFFFFFFF;
                if (shape->fill.type == NSVG_PAINT_NONE) {
                    // TODO: Strokes
                    continue;
                } else if(shape->fill.type == NSVG_PAINT_LINEAR_GRADIENT) {
                    if (shape->fill.gradient->nstops > 0) {
                        color = shape->fill.gradient->stops[0].color;
                    }
                } else if(shape->fill.type == NSVG_PAINT_RADIAL_GRADIENT) {
                    if (shape->fill.gradient->nstops > 0) {
                        color = shape->fill.gradient->stops[0].color;
                    }
                } else {
                    color = shape->fill.color;
                }
                shapes.push_back(Shape());
                auto& shape_ = shapes.back();
                //shape_.color = shape->fill.color;
                shape_.color = color;// 0xFFFFFFFF;
                for (NSVGpath* path = shape->paths; path != 0; path = path->next) {
                    Path path_;
                    for (int i = 0; i < path->npts - 1; i += 3) {
                        float* p = &path->pts[i * 2];
                        bezierCubicRecursive(
                            gfxm::vec3(p[0] * wmul, p[1] * hmul, .0f),
                            gfxm::vec3(p[2] * wmul, p[3] * hmul, .0f),
                            gfxm::vec3(p[4] * wmul, p[5] * hmul, .0f),
                            gfxm::vec3(p[6] * wmul, p[7] * hmul, .0f),
                            [&path_](const gfxm::vec3& pt) {
                                path_.points.push_back(pt);
                            }
                        );
                    }
                    simplifyPath(path_.points);
                
                    uint32_t base_index = shape_.points.size();
                    gfxm::vec3 pt0 = gfxm::vec3(path_.points[0].x, path_.points[0].y, .0f);
                    gfxm::vec3 pt_last = gfxm::vec3(path_.points[path_.points.size() - 1].x, path_.points[path_.points.size() - 1].y, .0f);

                    int end = path_.points.size();
                    if (gfxm::length(pt0 - pt_last) <= FLT_EPSILON) {
                        end = path_.points.size() - 1;
                    }

                    shape_.points.push_back(pt0);
                
                    for (int i = 1; i < end; ++i) {
                        gfxm::vec3 pt1 = gfxm::vec3(path_.points[i].x, path_.points[i].y, .0f);

                        uint32_t ip0 = shape_.points.size() - 1;
                        uint32_t ip1 = shape_.points.size();
                        Edge edge = Edge(ip0, ip1, false);
                        shape_.points.push_back(pt1);
                        shape_.edges.push_back(edge);
                        //edges_original.push_back(edge);
                    }
                    //if (path->closed) {
                        Edge edge = Edge(shape_.points.size() - 1, base_index, false);
                        shape_.edges.push_back(edge);
                        //edges_original.push_back(edge);
                    //}
                }
                std::vector<gfxm::vec2> points2d;
                points2d.resize(shape_.points.size());
                for (int i = 0; i < shape_.points.size(); ++i) {
                    points2d[i] = gfxm::vec2(shape_.points[i].x, shape_.points[i].y);
                }
                pathRemoveDuplicates(shape_.points, shape_.edges);
                cdtMakeSuperTriangle(
                    points2d.data(), points2d.size(),
                    &shape_.super_triangle[0],
                    &shape_.super_triangle[1],
                    &shape_.super_triangle[2],
                    &shape_.bounding_rect
                );
            }
            for (int i = 1; i < shapes.size(); ++i) {
                uint32_t base_index = cdtdbg.points.size();
                for (int j = 0; j < shapes[i].edges.size(); ++j) {
                    auto& e = shapes[i].edges[j];
                    cdtdbg.edges.push_back(cdtEdge(e.p0, e.p1, false));
                }
                for (int j = 1; j < shapes[i].points.size(); ++j) {
                    auto& p = shapes[i].points[j];
                    cdtPoint pt;
                    pt.p.x = p.x;
                    pt.p.y = p.y;
                    pt.index = cdtdbg.points.size();
                    cdtdbg.points.push_back(pt);
                }
            }
        
            {
                cdtDbgInit(&cdtdbg);
                //cdtInit(&cdtshape, shapes[0].super_triangle[0], shapes[0].super_triangle[1], shapes[0].super_triangle[2]);
            }
        
            for (int j = 0; j < shapes.size(); ++j) {
                if (shapes[j].points.empty()) {
                    assert(false);
                    continue;
                }
                shapes[j].cdt_points.resize(shapes[j].points.size());
                for (int i = 0; i < shapes[j].points.size(); ++i) {
                    CDT::V2d<double> p;
                    p.x = shapes[j].points[i].x;
                    p.y = shapes[j].points[i].y;
                    shapes[j].cdt_points[i] = p;
                }
                for (int i = 0; i < shapes[j].edges.size(); ++i) {
                    CDT::Edge edge(shapes[j].edges[i].p0, shapes[j].edges[i].p1);
                    shapes[j].cdt_edges.push_back(edge);
                }

                CDT::Triangulation<double> cdt;
                cdt.insertVertices(shapes[j].cdt_points);
                cdt.insertEdges(shapes[j].cdt_edges);
                cdt.eraseOuterTrianglesAndHoles();
                shapes[j].indices.resize(cdt.triangles.size() * 3);
                for (int i = 0; i < cdt.triangles.size(); ++i) {
                    shapes[j].indices[i * 3] = cdt.triangles[i].vertices[0];
                    shapes[j].indices[i * 3 + 1] = cdt.triangles[i].vertices[1];
                    shapes[j].indices[i * 3 + 2] = cdt.triangles[i].vertices[2];
                }
                shapes[j].vertices3.resize(cdt.vertices.size());
                for (int i = 0; i < cdt.vertices.size(); ++i) {
                    shapes[j].vertices3[i].x = cdt.vertices[i].x;
                    shapes[j].vertices3[i].y = cdt.vertices[i].y;
                    shapes[j].vertices3[i].z = .0f;
                }
            }
        
        } else {
            LOG_ERR("Failed to open svg file");
        }
    }
    void onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::KEYDOWN:
            // TODO:
            cdtDbgStep(&cdtdbg);
            break;
        }
    }
    GuiHitResult hitTest(int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }

        return GuiHitResult{ GUI_HIT::CLIENT, this };
    }
    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) override {
        bounding_rect = rc;
        client_area = bounding_rect;
        // TODO
        this->setContentViewTranslation(gfxm::vec3(-client_area.min.x, -client_area.min.y, .0f));
    }
    void onDraw() override {
        if (guiGetFocusedWindow() == this) {
            guiDrawRectLine(client_area, GUI_COL_TIMELINE_CURSOR);
        }
        guiDrawPushScissorRect(client_area);
        guiPushViewTransform(this->getContentViewTransform());
        for (int j = 0; j < shapes.size(); ++j) {
            //if (shapes[j].indices.empty()) {
            //    continue;
            //}
            guiDrawTrianglesIndexed(
                shapes[j].vertices3.data(),
                shapes[j].vertices3.size(),
                shapes[j].indices.data(),
                shapes[j].indices.size(),
                shapes[j].color
            ).model_transform = gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(500.f, 250.f, .0f));// .model_transform = gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(500.f, 200.f, .0f));
        }
        int i = 0;
        for (; i < cdtdbg.shape.triangles.size(); ++i) {
            auto& tri = cdtdbg.shape.triangles[i];
            gfxm::vec2 p[] = {
                gfxm::vec2(cdtdbg.shape.vertices[tri->a].p.x, cdtdbg.shape.vertices[tri->a].p.y),
                gfxm::vec2(cdtdbg.shape.vertices[tri->b].p.x, cdtdbg.shape.vertices[tri->b].p.y),
                gfxm::vec2(cdtdbg.shape.vertices[tri->c].p.x, cdtdbg.shape.vertices[tri->c].p.y)
            };
            gfxm::vec3 v[] = {
                gfxm::vec3(p[0].x, p[0].y, .0f),
                gfxm::vec3(p[1].x, p[1].y, .0f),
                gfxm::vec3(p[2].x, p[2].y, .0f)
            };
            
            guiDrawTriangleStrip(v, 3, tri->color);
        }
        
        guiDrawText(gfxm::vec2(100, 100), MKSTR("Tri count: " << cdtdbg.shape.triangles.size()).c_str(), guiGetDefaultFont(), .0f, GUI_COL_WHITE);
        
        for (int i = 0; i < cdtdbg.shape.edges.size(); ++i) {
            gfxm::vec2 p0(cdtdbg.shape.vertices[cdtdbg.shape.edges[i]->p0].p.x, cdtdbg.shape.vertices[cdtdbg.shape.edges[i]->p0].p.y);
            gfxm::vec2 p1(cdtdbg.shape.vertices[cdtdbg.shape.edges[i]->p1].p.x, cdtdbg.shape.vertices[cdtdbg.shape.edges[i]->p1].p.y);
            
            guiDrawLine(
                gfxm::rect(
                    p0, p1
                ), cdtdbg.shape.edges[i]->color
            );
        }
        guiDrawDiamond(cdtdbg.last_v2, 5.f, GUI_COL_RED, GUI_COL_RED, GUI_COL_RED);
        for (auto& e : cdtdbg.shape.vertices[cdtdbg.last_vertex_inserted].edges) {
            guiDrawLine(gfxm::rect(
                cdtdbg.shape.vertices[e->p0].p, cdtdbg.shape.vertices[e->p1].p
            ), GUI_COL_RED);
        }
        guiPopViewTransform();
        guiDrawPopScissorRect();
    }
};
class GuiCdtTestWindow : public GuiWindow {
    std::unique_ptr<GuiCdtTest> cdt_test;
public:
    GuiCdtTestWindow()
        :GuiWindow("CdtTest") {
        cdt_test.reset(new GuiCdtTest);
        cdt_test->setOwner(this);
        addChild(cdt_test.get());
    }
};
