#include "host.hpp"

#include "xui/renderer/gl_renderer.hpp"
#include "text_layout/text_layout.hpp"
#include "xui/layout/box_layout.hpp"

#include "gui/style/styles.hpp"


// TEMPORARY
void guiMakeDefaultStyleSheet(gui::style_sheet& sheet);


namespace xui {


    void Host::updateHovered(Element* e, HIT h) {
        auto new_hovered = e;
        if (new_hovered != elem_hovered) {
            if (elem_hovered) {
                elem_hovered->clearDisplayFlags(DISPLAY_FLAG_HOVERED);
            }
            if(new_hovered) {
                new_hovered->setDisplayFlags(DISPLAY_FLAG_HOVERED);
            }
        }
        elem_hovered = new_hovered;
        hit_hovered = h;
    }

    void Host::updatePressed(Element* e, HIT h) {        
        auto new_pressed = e;
        if (new_pressed != elem_pressed) {
            if (elem_pressed) {
                elem_pressed->clearDisplayFlags(DISPLAY_FLAG_PRESSED);
            }
            if(new_pressed) {
                new_pressed->setDisplayFlags(DISPLAY_FLAG_PRESSED);
            }
        }
        elem_pressed = new_pressed;
        hit_pressed = h;
    }

    void Host::bringToTop(Element* elem) {
        Element* subj = elem;
        while (subj) {
            if (subj->invoke(EvtBringToTop{ subj })) {
                break;
            }
            subj = subj->layout_parent;
        }
    }

    void Host::enterIdleState() {
        inp_state = InputState::Idle;
    }
    void Host::enterDragState(Element* elem_pressed) {
        inp_state = InputState::Drag;
    }
    void Host::enterResizeState(Element* elem) {
        Element* subj = elem;
        while (subj) {
            EvtResizeBegin e{ subj, subj->px_pos, subj->px_size };
            if (subj->invokeMutable(e)) {
                elem_resized = e.subj;
                resized_initial_pos = e.initial_pos;
                resized_initial_sz = e.initial_size;
                resized_initial_mouse_pos = mouse_pos;
                break;
            }
            subj = subj->layout_parent;
        }

        inp_state = InputState::Resize;
    }

    void Host::idle_mouseMove() {
        if (!elem_pressed) {
            return;
        }
        elem_pressed->invoke(EvtDrag{ mouse_delta.x, mouse_delta.y });
    }
    void Host::drag_mouseMove() {
        Element* subj = elem_pressed;
        while (subj) {
            if (subj->invoke(EvtMove{ subj, mouse_delta.x, mouse_delta.y })) {
                break;
            }
            subj = subj->layout_parent;
        }
    }
    void Host::resize_mouseMove() {
        gfxm::vec2 delta = gfxm::vec2(mouse_pos.x, mouse_pos.y) - gfxm::vec2(resized_initial_mouse_pos.x, resized_initial_mouse_pos.y);
        gfxm::rect rc(
            resized_initial_pos.x,
            resized_initial_pos.y,
            (resized_initial_pos + resized_initial_sz).x,
            (resized_initial_pos + resized_initial_sz).y
        );
        switch (hit_pressed) {
        case HIT::LEFT:
            rc.min.x += delta.x;
            rc.min.x = gfxm::_min(rc.min.x, rc.max.x);
            break;
        case HIT::RIGHT:
            rc.max.x += delta.x;
            rc.max.x = gfxm::_max(rc.max.x, rc.min.x);
            break;
        case HIT::TOP:
            rc.min.y += delta.y;
            rc.min.y = gfxm::_min(rc.min.y, rc.max.y);
            break;
        case HIT::BOTTOM:
            rc.max.y += delta.y;
            rc.max.y = gfxm::_max(rc.max.y, rc.min.y);
            break;
        case HIT::TOPLEFT:
            rc.min.x += delta.x;
            rc.min.x = gfxm::_min(rc.min.x, rc.max.x);
            rc.min.y += delta.y;
            rc.min.y = gfxm::_min(rc.min.y, rc.max.y);
            break;
        case HIT::BOTTOMLEFT:
            rc.min.x += delta.x;
            rc.min.x = gfxm::_min(rc.min.x, rc.max.x);
            rc.max.y += delta.y;
            rc.max.y = gfxm::_max(rc.max.y, rc.min.y);
            break;
        case HIT::TOPRIGHT:
            rc.max.x += delta.x;
            rc.max.x = gfxm::_max(rc.max.x, rc.min.x);
            rc.min.y += delta.y;
            rc.min.y = gfxm::_min(rc.min.y, rc.max.y);
            break;
        case HIT::BOTTOMRIGHT:
            rc.max.x += delta.x;
            rc.max.x = gfxm::_max(rc.max.x, rc.min.x);
            rc.max.y += delta.y;
            rc.max.y = gfxm::_max(rc.max.y, rc.min.y);
            break;
        default:
            assert(false);
        }
        elem_resized->invoke(EvtResize{ elem_resized, rc });
    }

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

    void Host::captureMouse(Element* elem) {
        elem_mouse_captor = elem;
    }

    void Host::releaseMouseCapture(Element* elem) {
        if (elem_mouse_captor != elem) {
            return;
        }
        elem_mouse_captor = nullptr;
    }

    void Host::mouseMove(int x, int y) {
        mouse_delta = gfxm::ivec2(x - mouse_pos.x, y - mouse_pos.y);
        mouse_pos = gfxm::ivec2(x, y);
        hitTest(x, y);

        if (elem_hovered) {
            auto rc = elem_hovered->getGlobalRect();
            int lclx = x - rc.min.x;
            int lcly = y - rc.min.y;
            elem_hovered->invoke(EvtMouseMove{ lclx, lcly });
        }

        switch (inp_state) {
        case InputState::Idle:
            idle_mouseMove();
            break;
        case InputState::Drag:
            drag_mouseMove();
            break;
        case InputState::Resize:
            resize_mouseMove();
            break;
        default:
            assert(false);
        }
    }
    void Host::mouseEvent(MouseButton btn, KeyEvent evt, int value) {
        hitTest(mouse_pos.x, mouse_pos.y);

        if (evt == KeyUp) {
            if (elem_pressed && elem_pressed == elem_hovered) {
                auto v = elem_pressed->getGlobalPos();
                int lclx = mouse_pos.x - v.x;
                int lcly = mouse_pos.y - v.y;
                elem_pressed->invoke(EvtClick{ lclx, lcly });
            }
            updatePressed(nullptr, HIT::NOWHERE);
            enterIdleState();
        }

        if (evt == KeyDown && elem_hovered) {
            updatePressed(elem_hovered, hit_hovered);

            if (hit_pressed == HIT::CAPTION) {
                enterDragState(elem_pressed);
            } else if (isResizingBorder(hit_pressed)) {
                enterResizeState(elem_pressed);
            }

            bringToTop(elem_pressed);
        }
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
        if(elem_hovered) {
            elem_hovered->clearDisplayFlags(DISPLAY_FLAG_HOVERED);
            elem_hovered = nullptr;
            hit_hovered = HIT::NOWHERE;
        }

        if (!root) {
            return;
        }
        HitResult hit_result;
        root->hitTest(hit_result, x, y);

        if (hit_result.hasHit()) {
            auto hit = hit_result.hits.back();
            updateHovered(hit.elem, hit.hit);
        }
    }

    void Host::draw(IRenderer* r) {
        if (!root) {
            return;
        }
        
        root->draw(r);
        /*
        if(elem_hovered) {
            r->drawRectBorder(
                elem_hovered->getGlobalRect(),
                0xCC00FF00,
                1, 1, 1, 1
            );
        }
        if (elem_pressed) {
            r->drawRectBorder(
                elem_pressed->getGlobalRect(),
                0xCCFF00FF,
                1, 1, 1, 1
            );
        }*/
    }

    void Host::render(IRenderer* renderer, int width, int height, bool clear) {
        if(1) {
            static float time = .0f;
            time += 0.001f;
            const int test_width = width;
            Font* font = default_font.get();

            std::string str("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
            str += "\n\x02\x9F\x9F\x9F\xFF// DONE: Separate three phase BoxLayout\x03";
            str += "\n// TODO: Generate proper UV values for rounded rectangle";
            str += "\n\x02\x9F\x9F\x9F\xFF// DONE: STX/ETX for color\x03";
            str += "\n\x02\x9F\x9F\x9F\xFF// DONE: Word wrapping\x03";
            str += "\n\x02\x9F\x9F\x9F\xFF// DONE: Text alignment\x03";
            str += "\n// TODO: BoxLayout alignment";
            str += "\n\x02\x9F\x9F\x9F\xFF// DONE: Element::event_table\x03";
            str += "\n// TODO: Window resizing";
            str += "\n// TODO: Nested render targets. Might require rc_visual_bounds";

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

