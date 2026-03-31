#include "renderer.hpp"

#include <vector>


namespace xui {

    enum DRAW_PRIM {
        DRAW_PRIM_LINE_STRIP,
        DRAW_PRIM_LINES,
        DRAW_PRIM_TRIANGLE_STRIP,
        DRAW_PRIM_TRIANGLE_FAN,
        DRAW_PRIM_TRIANGLES,
        DRAW_PRIM_TRIANGLES_INDEXED,
        DRAW_PRIM_TEXT,
        DRAW_PRIM_TEXT_HIGHLIGHT
    };
    struct DrawCmd {
        DRAW_PRIM primitive;
        int vertex_count;
        int vertex_first;
        uint32_t color;
        GLuint tex0;
        GLuint tex1;

        gfxm::vec2 offset;
    };

    class GLRenderer : public IRenderer {
        std::vector<DrawCmd> draw_commands;

        GLuint tex_white = 0;
        GLuint tex_black = 0;

        GLuint prog_default = 0;
        GLuint prog_text = 0;
        std::vector<Vertex>     vertices;
        std::vector<TextVertex> text_vertices;

    public:
        GLRenderer();
        ~GLRenderer();

        void drawLineStrip(const Vertex* vertices, int count) override;
        void drawTriangleStrip(const Vertex* vertices, int count) override;
        void drawText(const TextVertex* vertices, int count, const Font* font) override;

        void render(int width, int height, bool clear) override;
    };

}

