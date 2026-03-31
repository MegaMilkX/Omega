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

    class IRenderer {
        std::stack<gfxm::vec2> offset_stack;
        gfxm::vec2 current_offset = gfxm::vec2(0, 0);
    public:
        virtual ~IRenderer() {}

        virtual void drawLineStrip(const Vertex* vertices, int count) = 0;
        virtual void drawTriangleStrip(const Vertex* vertices, int count) = 0;
        virtual void drawText(const TextVertex* vertices, int count, const Font* font) = 0;
        virtual void render(int width, int height, bool clear) = 0;

        void pushOffset(const gfxm::vec2& offs);
        void popOffset();
        const gfxm::vec2& getOffset();

        void drawRectLine(const gfxm::rect& rc, uint32_t col);
        void drawRectRound(const gfxm::rect& rc, uint32_t col, float rnw, float rne, float rsw, float rse);
        void drawRectRoundBorder(
            const gfxm::rect& rc, uint32_t col,
            float rnw, float rne, float rsw, float rse,
            float thickness_left, float thickness_top, float thickness_right, float thickness_bottom
        );
    };

}