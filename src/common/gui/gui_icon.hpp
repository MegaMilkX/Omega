#pragma once

#include <vector>
#include <stdint.h>
#include "math/gfxm.hpp"
#include "gui/gui_draw.hpp"

class GuiIcon {
    friend GuiIcon* guiLoadIcon(const char* svg_path);
    friend void guiInit(Font* font);

    struct Shape {
        std::vector<gfxm::vec3> vertices;
        std::vector<uint32_t> indices;
        uint32_t color;
    };
    std::vector<Shape> shapes;

public:
    void draw(const gfxm::rect& rc, uint32_t color) const {
        float w = rc.max.x - rc.min.x;
        float h = rc.max.y - rc.min.y;
        gfxm::mat4 tr 
            = gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(rc.min.x, rc.min.y, .0f))
            * gfxm::scale(gfxm::mat4(1.f), gfxm::vec3(w, h, 1.0f));
        for (int i = 0; i < shapes.size(); ++i) {
            guiDrawTrianglesIndexed(
                shapes[i].vertices.data(),
                shapes[i].vertices.size(),
                (uint32_t*)shapes[i].indices.data(),
                shapes[i].indices.size(),
                color
            ).model_transform = tr;
        }
    }
};