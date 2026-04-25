#pragma once

#include <stack>
#include "typeface/font.hpp"


namespace xui {

    struct Vertex {
        gfxm::vec3 pos;
        gfxm::vec2 uv;
        uint32_t rgba;
    };

    struct TextVertex {
        gfxm::vec3 pos;
        gfxm::vec2 uv;
        uint32_t rgba;
        float uv_lookup;
    };
    
    enum DRAW_PRIM {
        DRAW_PRIM_LINE_STRIP,
        DRAW_PRIM_LINES,
        DRAW_PRIM_TRIANGLE_STRIP,
        DRAW_PRIM_TRIANGLE_FAN,
        DRAW_PRIM_TRIANGLES,
        DRAW_PRIM_TRIANGLES_INDEXED,
        DRAW_PRIM_TEXT,
        DRAW_PRIM_TEXT_HIGHLIGHT,

        DRAW_PRIM_CLIP_PUSH,
        DRAW_PRIM_CLIP_POP
    };
    struct DrawCmd {
        DRAW_PRIM primitive;
        union {
            // drawing
            struct {
                int vertex_count;
                int vertex_first;
                uint32_t color;
                uint64_t tex0;
                uint64_t tex1;

                gfxm::vec2 offset;
            };
            // DRAW_PRIM_CLIP_PUSH
            struct {
                int rc_x;
                int rc_y;
                int rc_w;
                int rc_h;
            };
        };
        DrawCmd() {}
    };

    class IRenderer {
        Font* default_font = nullptr;
        std::vector<DrawCmd> draw_commands;
        std::stack<gfxm::vec2> offset_stack;
        gfxm::vec2 current_offset = gfxm::vec2(0, 0);
    public:
        virtual ~IRenderer() {}

        virtual void drawLineStrip(const Vertex* vertices, int count) = 0;
        virtual void drawTriangleStrip(const Vertex* vertices, int count, uint64_t texture = 0) = 0;
        virtual void drawText(const TextVertex* vertices, int count, const Font* font) = 0;
        virtual void render(const DrawCmd* commands, int count, int width, int height, bool clear) = 0;

        DrawCmd& emplaceCmd();
        void render(int width, int height, bool clear);

        void setDefaultFont(Font* fnt) { default_font = fnt; }
        void pushOffset(const gfxm::vec2& offs);
        void popOffset();
        const gfxm::vec2& getOffset();

        void pushClipRect(int x, int y, int w, int h);
        void popClipRect();
        void drawRectLine(const gfxm::rect& rc, uint32_t col);
        void drawRect(const gfxm::rect& rc, uint32_t col, uint64_t texture = 0);
        void drawRectBorder(
            const gfxm::rect& rc, uint32_t col,
            float thickness_left, float thickness_top, float thickness_right, float thickness_bottom
        );
        void drawRectRound(const gfxm::rect& rc, uint32_t col, float rnw, float rne, float rsw, float rse);
        void drawRectRoundBorder(
            const gfxm::rect& rc, uint32_t col,
            float rnw, float rne, float rsw, float rse,
            float thickness_left, float thickness_top, float thickness_right, float thickness_bottom
        );
    };

}