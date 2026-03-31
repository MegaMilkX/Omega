#include "renderer.hpp"


namespace xui {

    void IRenderer::pushOffset(const gfxm::vec2& offs) {
        gfxm::vec2 cur_offs = gfxm::vec2(0, 0);
        if (!offset_stack.empty()) {
            cur_offs = offset_stack.top();
        }
        offset_stack.push(cur_offs + offs);
    }
    void IRenderer::popOffset() {
        offset_stack.pop();
        if (!offset_stack.empty()) {
            current_offset = offset_stack.top();
        } else {
            current_offset = gfxm::vec2(0, 0);
        }
    }
    const gfxm::vec2& IRenderer::getOffset() {
        if (!offset_stack.empty()) {
            current_offset = offset_stack.top();
        }
        return current_offset;
    }

    void IRenderer::drawRectLine(const gfxm::rect& rc, uint32_t col) {
        Vertex vertices[5] = {
            Vertex{ gfxm::vec3( rc.min.x, rc.min.y, .0f ), gfxm::vec2(0, 0), col},
            Vertex{ gfxm::vec3( rc.max.x, rc.min.y, .0f ), gfxm::vec2(0, 0), col},
            Vertex{ gfxm::vec3( rc.max.x, rc.max.y, .0f ), gfxm::vec2(0, 0), col},
            Vertex{ gfxm::vec3( rc.min.x, rc.max.y, .0f ), gfxm::vec2(0, 0), col},
            Vertex{ gfxm::vec3( rc.min.x, rc.min.y, .0f ), gfxm::vec2(0, 0), col},
        };
        drawLineStrip(vertices, sizeof(vertices) / sizeof(vertices[0]));
    }
    void IRenderer::drawRectRound(const gfxm::rect& rc, uint32_t col, float rnw, float rne, float rsw, float rse) {
        const bool bNW = true;
        const bool bNE = true;
        const bool bSW = true;
        const bool bSE = true;

        int segments = 16;
        std::vector<Vertex> vertices;

        float rc_width = rc.max.x - rc.min.x;
        float rc_height = rc.max.y - rc.min.y;
        float min_side = gfxm::_min(rc_width, rc_height);
        float half_min_side = min_side * .5f;

        rnw = gfxm::_min(rnw, half_min_side);
        rne = gfxm::_min(rne, half_min_side);
        rse = gfxm::_min(rse, half_min_side);
        rsw = gfxm::_min(rsw, half_min_side);

        std::array<gfxm::vec3, 4> corners = {
            gfxm::vec3(rc.min.x + rnw, rc.min.y + rnw, .0f),
            gfxm::vec3(rc.max.x - rne, rc.min.y + rne, .0f),
            gfxm::vec3(rc.max.x - rse, rc.max.y - rse, .0f),
            gfxm::vec3(rc.min.x + rsw, rc.max.y - rsw, .0f)
        };

        float radius = rnw;
        gfxm::vec3 corner_pt = corners[0];
        gfxm::vec3 corner_offset(radius, radius, .0f);
        float radian_start = gfxm::pi;
        if (bNW) {
            for (int i = 0; i <= segments; ++i) {
                float a = radian_start + (i / (float)segments) * gfxm::pi * .5f;
                gfxm::vec3 pt_outer = gfxm::vec3(
                    cosf(a), sinf(a), .0f
                ) * radius + corner_pt;
                gfxm::vec3 pt_inner = corner_pt;
                vertices.push_back(Vertex{ pt_outer, gfxm::vec2(0, 0), col });
                vertices.push_back(Vertex{ pt_inner, gfxm::vec2(0, 0), col });
            }
        } else {
            vertices.push_back(Vertex{ gfxm::vec3(rc.min.x, rc.min.y, .0f), gfxm::vec2(0, 0), col });
            vertices.push_back(Vertex{ corner_pt, gfxm::vec2(0, 0), col });
        }

        radius = rne;
        corner_pt = corners[1];
        corner_offset = gfxm::vec3(-radius, radius, .0f);
        radian_start += gfxm::pi * .5f;
        if (bNE) {
            for (int i = 0; i <= segments; ++i) {
                float a = radian_start + (i / (float)segments) * gfxm::pi * .5f;
                gfxm::vec3 pt_outer = gfxm::vec3(
                    cosf(a), sinf(a), .0f
                ) * radius + corner_pt;
                gfxm::vec3 pt_inner = corner_pt;
                vertices.push_back(Vertex{ pt_outer, gfxm::vec2(0, 0), col });
                vertices.push_back(Vertex{ pt_inner, gfxm::vec2(0, 0), col });
            }
        } else {
            vertices.push_back(Vertex{ gfxm::vec3(rc.max.x, rc.min.y, .0f), gfxm::vec2(0, 0), col });
            vertices.push_back(Vertex{ corner_pt, gfxm::vec2(0, 0), col });
        }

        radius = rse;
        corner_pt = corners[2];
        corner_offset = gfxm::vec3(-radius, -radius, .0f);
        radian_start += gfxm::pi * .5f;
        if (bSE) {
            for (int i = 0; i <= segments; ++i) {
                float a = radian_start + (i / (float)segments) * gfxm::pi * .5f;
                gfxm::vec3 pt_outer = gfxm::vec3(
                    cosf(a), sinf(a), .0f
                ) * radius + corner_pt;
                gfxm::vec3 pt_inner = corner_pt;
                vertices.push_back(Vertex{ pt_outer, gfxm::vec2(0, 0), col });
                vertices.push_back(Vertex{ pt_inner, gfxm::vec2(0, 0), col });
            }
        } else {
            vertices.push_back(Vertex{ gfxm::vec3(rc.max.x, rc.max.y, .0f), gfxm::vec2(0, 0), col });
            vertices.push_back(Vertex{ corner_pt, gfxm::vec2(0, 0), col });
        }

        radius = rsw;
        corner_pt = corners[3];
        corner_offset = gfxm::vec3(radius, -radius, .0f);
        radian_start += gfxm::pi * .5f;
        if (bSW) {
            for (int i = 0; i <= segments; ++i) {
                float a = radian_start + (i / (float)segments) * gfxm::pi * .5f;
                gfxm::vec3 pt_outer = gfxm::vec3(
                    cosf(a), sinf(a), .0f
                ) * radius + corner_pt;
                gfxm::vec3 pt_inner = corner_pt;
                vertices.push_back(Vertex{ pt_outer, gfxm::vec2(0, 0), col });
                vertices.push_back(Vertex{ pt_inner, gfxm::vec2(0, 0), col });
            }
        } else {
            vertices.push_back(Vertex{ gfxm::vec3(rc.min.x, rc.max.y, .0f), gfxm::vec2(0, 0), col });
            vertices.push_back(Vertex{ corner_pt, gfxm::vec2(0, 0), col });
        }
        vertices.push_back(vertices[0]);
        vertices.push_back(vertices[1]);
        vertices.push_back(vertices[1]); // strip reset
        vertices.push_back(Vertex{ corners[3], gfxm::vec2(0, 0), col });
        vertices.push_back(Vertex{ corners[1], gfxm::vec2(0, 0), col });
        vertices.push_back(Vertex{ corners[2], gfxm::vec2(0, 0), col });

        drawTriangleStrip(vertices.data(), vertices.size());
    }
    void IRenderer::drawRectRoundBorder(
        const gfxm::rect& rc, uint32_t col,
        float rnw, float rne, float rsw, float rse,
        float thickness_left, float thickness_top, float thickness_right, float thickness_bottom
    ) {
        int segments = 16;
        std::vector<Vertex> vertices;

        float rc_width = rc.max.x - rc.min.x;
        float rc_height = rc.max.y - rc.min.y;
        float min_side = gfxm::_min(rc_width, rc_height);
        float half_min_side = min_side * .5f;

        rnw = gfxm::_min(rnw, half_min_side);
        rne = gfxm::_min(rne, half_min_side);
        rse = gfxm::_min(rse, half_min_side);
        rsw = gfxm::_min(rsw, half_min_side);

        float radius = rnw;
        float inner_radius_a = radius - thickness_left;
        float inner_radius_b = radius - thickness_top;
        gfxm::vec3 corner_pt(rc.min.x + radius, rc.min.y + radius, .0f);
        gfxm::vec3 inner_corner_pt = corner_pt + gfxm::vec3(thickness_left, thickness_top, .0f);
        float radian_start = gfxm::pi;
        for (int i = 0; i <= segments; ++i) {
            float a = radian_start + (i / (float)segments) * gfxm::pi * .5f;
            gfxm::vec3 pt_outer = gfxm::vec3(
                cosf(a), sinf(a), .0f
            ) * radius + corner_pt;
            gfxm::vec3 pt_inner = gfxm::vec3(
                cosf(a) * inner_radius_a, sinf(a) * inner_radius_b, .0f
            ) + corner_pt;

            vertices.push_back(Vertex{ pt_outer, gfxm::vec2(0, 0), col });
            vertices.push_back(Vertex{ pt_inner, gfxm::vec2(0, 0), col });
        }

        radius = rne;
        inner_radius_a = radius - thickness_top;
        inner_radius_b = radius - thickness_right;
        corner_pt = gfxm::vec3(rc.max.x - radius, rc.min.y + radius, .0f);
        radian_start += gfxm::pi * .5f;
        for (int i = 0; i <= segments; ++i) {
            float a = radian_start + (i / (float)segments) * gfxm::pi * .5f;
            gfxm::vec3 pt_outer = gfxm::vec3(
                cosf(a), sinf(a), .0f
            ) * radius + corner_pt;
            gfxm::vec3 pt_inner = gfxm::vec3(
                cosf(a) * inner_radius_b, sinf(a) * inner_radius_a, .0f
            ) + corner_pt;

            vertices.push_back(Vertex{ pt_outer, gfxm::vec2(0, 0), col });
            vertices.push_back(Vertex{ pt_inner, gfxm::vec2(0, 0), col });
        }

        radius = rse;
        inner_radius_a = radius - thickness_right;
        inner_radius_b = radius - thickness_bottom;
        corner_pt = gfxm::vec3(rc.max.x - radius, rc.max.y - radius, .0f);
        radian_start += gfxm::pi * .5f;
        for (int i = 0; i <= segments; ++i) {
            float a = radian_start + (i / (float)segments) * gfxm::pi * .5f;
            gfxm::vec3 pt_outer = gfxm::vec3(
                cosf(a), sinf(a), .0f
            ) * radius + corner_pt;
            gfxm::vec3 pt_inner = gfxm::vec3(
                cosf(a) * inner_radius_a, sinf(a) * inner_radius_b, .0f
            ) + corner_pt;

            vertices.push_back(Vertex{ pt_outer, gfxm::vec2(0, 0), col });
            vertices.push_back(Vertex{ pt_inner, gfxm::vec2(0, 0), col });
        }

        radius = rsw;
        inner_radius_a = radius - thickness_bottom;
        inner_radius_b = radius - thickness_left;
        corner_pt = gfxm::vec3(rc.min.x + radius, rc.max.y - radius, .0f);
        radian_start += gfxm::pi * .5f;
        for (int i = 0; i <= segments; ++i) {
            float a = radian_start + (i / (float)segments) * gfxm::pi * .5f;
            gfxm::vec3 pt_outer = gfxm::vec3(
                cosf(a), sinf(a), .0f
            ) * radius + corner_pt;
            gfxm::vec3 pt_inner = gfxm::vec3(
                cosf(a) * inner_radius_b, sinf(a) * inner_radius_a, .0f
            ) + corner_pt;

            vertices.push_back(Vertex{ pt_outer, gfxm::vec2(0, 0), col });
            vertices.push_back(Vertex{ pt_inner, gfxm::vec2(0, 0), col });
        }
        vertices.push_back(vertices[0]);
        vertices.push_back(vertices[1]);

        drawTriangleStrip(
            vertices.data(),
            vertices.size()
        );
    }

}