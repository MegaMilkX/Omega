#include "renderer.hpp"

#include <vector>


namespace xui {


    class GLRenderer : public IRenderer {
        GLuint tex_white = 0;
        GLuint tex_black = 0;

        GLuint prog_default = 0;
        GLuint prog_text = 0;
        std::vector<Vertex>     vertices;
        std::vector<TextVertex> text_vertices;

        std::stack<gfxm::rect> scissor_stack;
        gfxm::rect current_clip_rect;

    public:
        GLRenderer();
        ~GLRenderer();

        void drawLineStrip(const Vertex* vertices, int count) override;
        void drawTriangleStrip(const Vertex* vertices, int count, uint64_t texture = 0) override;
        void drawText(const TextVertex* vertices, int count, const Font* font) override;

        void render(const DrawCmd* commands, int count, int width, int height, bool clear) override;
    };

}

