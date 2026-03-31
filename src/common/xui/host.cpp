#include "host.hpp"

#include "xui/renderer/gl_renderer.hpp"
#include "text_layout/text_layout.hpp"
#include "xui/layout/box_layout.hpp"

#include "gui/style/styles.hpp"


// TEMPORARY
void guiMakeDefaultStyleSheet(gui::style_sheet& sheet);


namespace xui {


    Host::Host() {
        //default_font = fontGet("fonts/ProggyClean.ttf", 16, 72);
        default_font = fontGet("fonts/nimbusmono-bold.otf", 16, 72);
        //default_font = fontGet("fonts/OpenSans-Regular.ttf", 64, 72);

        root.reset(new Root);

        // TEMPORARY
        guiMakeDefaultStyleSheet(style_sheet);
    }

    Host::~Host() {
        root.reset();
        default_font.reset();
    }

    void Host::layout(int width, int height) {
        if (!root) {
            return;
        }

        root->applyStyle(this);

        LayoutContext ctx{
            .phase = LAYOUT_PHASE::COMMIT,
            .px_resolved_width = width,
            .px_resolved_height = height
        };
        root->layout(this, ctx);
    }
    void Host::hitTest(int x, int y) {
        elem_hovered = nullptr;

        if (!root) {
            return;
        }
        HitResult hit_result;
        root->hitTest(hit_result, x, y);

        if (hit_result.hasHit()) {
            elem_hovered = hit_result.hits.back().elem;
        }
    }

    void Host::draw(IRenderer* r) {
        if (!root) {
            return;
        }
        
        root->draw(r);

        if(elem_hovered) {
            r->drawRectLine(elem_hovered->getGlobalRect(), 0xCC00FF00);
        }
    }

    void Host::render(IRenderer* renderer, int width, int height, bool clear) {
        static float time = .0f;
        time += .001f;
        int test_width = 1920;//1 + (sinf(time * 3.f) + 1.f) * .5f * 1920;
        /*
        renderer->drawRectRound(
            gfxm::rect(gfxm::vec2(0, 0), gfxm::vec2(width, 1000)), gfxm::hsv2rgb32(.25f, .2f, .05f, 1.f),
            15, 15, 15, 15
        );*/
        
        // Layout test
        if(0) {
            BoxLayout layout;
            layout.m_px_container_width = 1000;
            layout.m_px_container_height = 1000;
            layout.content_margin = em(1, 1);
            layout.m_px_line_height = default_font->getLineHeight();

            auto createBoxes = [this]()->std::vector<BoxLayout::Box> {
                std::vector<BoxLayout::Box> boxes;

                boxes.push_back(
                    BoxLayout::Box{
                        .px_line_height = default_font->getLineHeight(),
                        .size = xvec2(fill(), 40),
                        .color = gfxm::hsv2rgb32(.0f, .3f, .7f, .3f)
                    }
                );
                boxes.push_back(
                    BoxLayout::Box{
                        .px_line_height = default_font->getLineHeight(),
                        .size = xvec2(fill(), 40),
                        .color = gfxm::hsv2rgb32(.0f, .3f, .7f, .3f)
                    }
                );
                boxes.push_back(
                    BoxLayout::Box{
                        .px_line_height = default_font->getLineHeight(),
                        .size = xvec2(40, 40),
                        .color = gfxm::hsv2rgb32(.0f, .3f, .7f, .3f),
                        .same_line = true
                    }
                );

                int n_boxes = 60;
                for (int i = 0; i < n_boxes; ++i) {
                    auto& box = boxes.emplace_back();
                    box.px_line_height = default_font->getLineHeight();
                    box.min_size = px(0, 0);
                    box.size = px(100 + (rand() % 200), 40);
                    box.color = gfxm::hsv2rgb32((rand() % 100) * .01f, .3f, .7f, .3f);
                    box.same_line = rand() % 2;                    
                }
                return boxes;
            };

            static std::vector<BoxLayout::Box> boxes = createBoxes();
            layout.buildWidth(boxes.data(), boxes.size(), nullptr);
            layout.buildHeight(nullptr);
            layout.buildPosition();

            renderer->drawRectRound(
                gfxm::rect(0, 0, layout.m_px_container_width, layout.m_px_container_height), gfxm::hsv2rgb32(.5f, .3f, .2f, .3f),
                5, 5, 5, 5
            );
            for (int i = 0; i < layout.lines.size(); ++i) {
                const auto& line = layout.lines[i];
                for (int j = 0; j < line.boxes.size(); ++j) {
                    const auto& box = line.boxes[j];
                    
                    gfxm::rect rc(box.px_pos_x, box.px_pos_y, box.px_pos_x + box.width.value, box.px_pos_y + box.height.value);
                    renderer->drawRectRound(rc, box.color, 5, 5, 5, 5);
                }
            }
        }
        
        if(0) {
            Font* font = default_font.get();

            std::string str(R"(The quick brown fox jumps over the lazy dog
Hello, TextLayout!
Beep boop
)");
            str += "\nTab \x02\xFF\x66\x77\xFFtest\x03\nABCDEF\t\t\t\t16\t\t\t40\nABC\t\t\t\t\t\t320\t\t\t99";
            str += "\n// TODO: Separate three phase BoxLayout";
            str += "\n// TODO: Generate proper UV values for rounded rectangle";
            str += "\n//\x02\x0F\xCC\x44\xFF DONE\x03: STX/ETX for color";
            str += "\n//\x02\x0F\xCC\x44\xFF DONE\x03: Word wrapping";
            str += "\n// TODO: Alignment";

            TextLayout layout;
            layout.build(str, font, test_width);

            std::vector<TextVertex> vertices;
            for (int i = 0; i < layout.glyphs.size(); ++i) {
                const auto& g = layout.glyphs[i];
                const auto& q = g.makeQuad();
                
                uint32_t color = g.color;
                if(color == 0xFFFFFFFF) {
                    color = gfxm::hsv2rgb32(gfxm::fract(time + i * .025f), .3f, 1.f, 1.f);
                }

                // shadow
                const gfxm::vec3 shadow_offs = gfxm::vec3(1.f, -1.f, .0f);
                vertices.insert(
                    vertices.end(),
                    {
                        TextVertex{ q.pos[0] + shadow_offs, q.uv[0], 0xFF000000, q.lut_values[0] },
                        TextVertex{ q.pos[1] + shadow_offs, q.uv[1], 0xFF000000, q.lut_values[1] },
                        TextVertex{ q.pos[2] + shadow_offs, q.uv[2], 0xFF000000, q.lut_values[2] },
                        TextVertex{ q.pos[1] + shadow_offs, q.uv[1], 0xFF000000, q.lut_values[1] },
                        TextVertex{ q.pos[3] + shadow_offs, q.uv[3], 0xFF000000, q.lut_values[3] },
                        TextVertex{ q.pos[2] + shadow_offs, q.uv[2], 0xFF000000, q.lut_values[2] }
                    }
                );
                // text
                vertices.insert(
                    vertices.end(),
                    {
                        TextVertex{ q.pos[0], q.uv[0], color, q.lut_values[0] },
                        TextVertex{ q.pos[1], q.uv[1], color, q.lut_values[1] },
                        TextVertex{ q.pos[2], q.uv[2], color, q.lut_values[2] },
                        TextVertex{ q.pos[1], q.uv[1], color, q.lut_values[1] },
                        TextVertex{ q.pos[3], q.uv[3], color, q.lut_values[3] },
                        TextVertex{ q.pos[2], q.uv[2], color, q.lut_values[2] }
                    }
                );
            }

            renderer->drawText(vertices.data(), vertices.size(), font);
        }

        renderer->render(width, height, clear);
    }

    bool Host::isHovered(Element* e) const {
        return elem_hovered == e;
    }

    gui::style_sheet& Host::getStyleSheet() {
        return style_sheet;
    }
    Font* Host::getDefaultFont() {
        return default_font.get();
    }
    Font* Host::resolveFont(Element* e) {
        if (!e->style) {
            return getDefaultFont();
        }
        auto font_style = e->style->get_component<gui::style_font>();
        if (!font_style || font_style->font == nullptr) {
            return getDefaultFont();
        }
        return font_style->font.get();
    }
}

