#pragma once

#include "gui/elements/element.hpp"


void guiCaptureMouse(GuiElement* e);
void guiReleaseMouseCapture(GuiElement* e);
bool guiDragStartWindowDockable(GuiElement* window);


class GuiWindowLayer : public GuiElement {
    enum GuiHostInputState {
        GuiHostInput_Idle,
        GuiHostInput_Moving,
        GuiHostInput_ResizingTop,
        GuiHostInput_ResizingTopLeft,
        GuiHostInput_ResizingTopRight,
        GuiHostInput_ResizingBottom,
        GuiHostInput_ResizingBottomLeft,
        GuiHostInput_ResizingBottomRight,
        GuiHostInput_ResizingLeft,
        GuiHostInput_ResizingRight,
    };

    enum GuiMouseButton {
        GuiMouseButtonNone,
        GuiMouseButtonLeft,
        GuiMouseButtonRight,
        GuiMouseButtonMiddle,
    };
    GuiMouseButton guiMsgToMouseBtnCode(GUI_MSG msg) {
        GuiMouseButton code = GuiMouseButtonNone;
        switch (msg) {
        case GUI_MSG::LBUTTON_DOWN:
            code = GuiMouseButtonLeft;
            break;
        case GUI_MSG::RBUTTON_DOWN:
            code = GuiMouseButtonRight;
            break;
        case GUI_MSG::MBUTTON_DOWN:
            code = GuiMouseButtonMiddle;
            break;
        case GUI_MSG::LBUTTON_UP:
            code = GuiMouseButtonLeft;
            break;
        case GUI_MSG::RBUTTON_UP:
            code = GuiMouseButtonRight;
            break;
        case GUI_MSG::MBUTTON_UP:
            code = GuiMouseButtonMiddle;
            break;
        }
        return code;
    }
    bool guiMsgToMouseBtnState(GUI_MSG msg) {
        switch (msg) {
        case GUI_MSG::LBUTTON_DOWN:
        case GUI_MSG::RBUTTON_DOWN:
        case GUI_MSG::MBUTTON_DOWN:
            return true;
        case GUI_MSG::LBUTTON_UP:
        case GUI_MSG::RBUTTON_UP:
        case GUI_MSG::MBUTTON_UP:
            return false;
        }
        assert(false);
        return false;
    }

    gfxm::vec2 mouse_pos;

    const gfxm::rect frame_thickness = gfxm::rect(2.f, 30.f, 2.f, 2.f);
    gfxm::vec2 next_window_pos = gfxm::vec2(50, 50);

    struct Window {
        gfxm::rect rc;
        GuiElement* elem;
        Window(GuiElement* e, float minx, float miny, float maxx, float maxy)
            : elem(e), rc(minx, miny, maxx, maxy) {}
    };

    std::vector<std::unique_ptr<Window>> windows;
    Window* hovered_window = nullptr;
    GUI_HIT hovered_hit = GUI_HIT::NOWHERE;
    GuiHitResult hit_result;
    GuiHostInputState input_state = GuiHostInput_Idle;
    Window* moved_window = nullptr;

    int getWindowIdx(Window* wnd) {
        for (int i = 0; i < windows.size(); ++i) {
            if (windows[i].get() == wnd) {
                return i;
            }
        }
        return -1;
    }

    void windowToFront(Window* wnd) {
        int i = getWindowIdx(wnd);
        auto uptr = std::move(windows[i]);
        windows.erase(windows.begin() + i);
        windows.push_back(std::move(uptr));
    }
    
    bool onLeftMouseButton(bool pressed) {
        if (pressed) {
            if (hovered_window) {
                windowToFront(hovered_window);
                switch (hovered_hit) {
                case GUI_HIT::CAPTION:
                    input_state = GuiHostInput_Moving;
                    moved_window = hovered_window;
                    guiDragStartWindowDockable(hovered_window->elem);
                    break;
                case GUI_HIT::TOP:
                    input_state = GuiHostInput_ResizingTop;
                    moved_window = hovered_window;
                    break;
                case GUI_HIT::BOTTOM:
                    input_state = GuiHostInput_ResizingBottom;
                    moved_window = hovered_window;
                    break;
                case GUI_HIT::LEFT:
                    input_state = GuiHostInput_ResizingLeft;
                    moved_window = hovered_window;
                    break;
                case GUI_HIT::RIGHT:
                    input_state = GuiHostInput_ResizingRight;
                    moved_window = hovered_window;
                    break;
                case GUI_HIT::TOPLEFT:
                    input_state = GuiHostInput_ResizingTopLeft;
                    moved_window = hovered_window;
                    break;
                case GUI_HIT::TOPRIGHT:
                    input_state = GuiHostInput_ResizingTopRight;
                    moved_window = hovered_window;
                    break;
                case GUI_HIT::BOTTOMLEFT:
                    input_state = GuiHostInput_ResizingBottomLeft;
                    moved_window = hovered_window;
                    break;
                case GUI_HIT::BOTTOMRIGHT:
                    input_state = GuiHostInput_ResizingBottomRight;
                    moved_window = hovered_window;
                    break;
                }

                guiCaptureMouse(this);
            }
            return hovered_window;
        } else {
            input_state = GuiHostInput_Idle;
            moved_window = nullptr;
            guiReleaseMouseCapture(this);
        }
        return false;
    }
    bool onMouseButton(GuiMouseButton btn, bool pressed) {
        switch (btn) {
        case GuiMouseButtonLeft:
            return onLeftMouseButton(pressed);
        case GuiMouseButtonRight:
            break;
        case GuiMouseButtonMiddle:
            break;
        }
        return false;
    }
    bool onMouseMove(int x, int y) {
        x = x - layout_position.x;
        y = y - layout_position.y;
        gfxm::vec2 prev_mouse_pos = mouse_pos;
        mouse_pos = gfxm::vec2(x, y);
        gfxm::vec2 mouse_delta = mouse_pos - prev_mouse_pos;
        
        switch (input_state) {
        case GuiHostInput_Idle:
            return false;
        case GuiHostInput_Moving:
            moved_window->rc.min += mouse_delta;
            moved_window->rc.max += mouse_delta;
            return true;
        case GuiHostInput_ResizingTop:
            moved_window->rc.min.y = mouse_pos.y;
            return true;
        case GuiHostInput_ResizingTopLeft:
            moved_window->rc.min = mouse_pos;
            return true;
        case GuiHostInput_ResizingTopRight:
            moved_window->rc.min.y = mouse_pos.y;
            moved_window->rc.max.x = mouse_pos.x;
            return true;
        case GuiHostInput_ResizingBottom:
            moved_window->rc.max.y = mouse_pos.y;
            return true;
        case GuiHostInput_ResizingBottomLeft:
            moved_window->rc.max.y = mouse_pos.y;
            moved_window->rc.min.x = mouse_pos.x;
            return true;
        case GuiHostInput_ResizingBottomRight:
            moved_window->rc.max.y = mouse_pos.y;
            moved_window->rc.max.x = mouse_pos.x;
            return true;
        case GuiHostInput_ResizingLeft:
            moved_window->rc.min.x = mouse_pos.x;
            return true;
        case GuiHostInput_ResizingRight:
            moved_window->rc.max.x = mouse_pos.x;
            return true;
        }

        return false;
    }
public:
    GuiWindowLayer() {}

    void pushBackInDragState(GuiElement* e) {
        GuiElement::pushBack(e);
        hovered_window = windows.back().get();
        input_state = GuiHostInput_Moving;
        moved_window = hovered_window;
        guiDragStartWindowDockable(hovered_window->elem);
        //gfxm::rect rc(e->getGlobalPosition(), e->getGlobalBoundingRect().size());
        //moved_window->rc = rc;
        
        gfxm::vec2 sz = e->getGlobalBoundingRect().size();
        gfxm::vec2 at = mouse_pos - getGlobalPosition() - gfxm::vec2(100, 15);
        moved_window->rc.min = at;
        moved_window->rc.max = at + sz;
    }

    void onInsertChild(GuiElement* e) override {
        float width = e->size.x.value;
        float height = e->size.y.value;
        width = gfxm::_max(width, frame_thickness.min.x + frame_thickness.max.x);
        height = gfxm::_max(height, frame_thickness.min.y + frame_thickness.max.y);

        width = 400;
        height = 300;

        gfxm::vec2 sz = gui_to_px(e->size, e->getFont(), getBoundingRect().size());

        auto ptr = new Window(
            e, next_window_pos.x, next_window_pos.y,
            next_window_pos.x + width, next_window_pos.y + height
        );
        windows.push_back(std::unique_ptr<Window>(ptr));

        next_window_pos += gfxm::vec2(20, 20);
    }
    void onRemoveChild(GuiElement* e) override {
        for (int i = 0; i < windows.size(); ++i) {
            if (windows[i]->elem == e) {
                windows.erase(windows.begin() + i);
                break;
            }
        }
    }
    void onHitTest(GuiHitResult& hit, int x, int y) override {
        hovered_window = nullptr;

        for (int i = windows.size() - 1; i >= 0; --i) {
            auto& w = windows[i];
            auto e = w->elem;

            if (e->isHidden()) {
                continue;
            }

            if (!gfxm::point_in_rect(w->rc, gfxm::vec2(x, y))) {
                continue;
            }

            hovered_window = w.get();

            guiHitTestResizeBorders(hit, w->elem, w->rc, 10.f, x, y, 0b1111);
            if (hit.hasHit()) {
                hovered_hit = hit.hits.back().hit;
                return;
            }
            
            w->elem->hitTest(hit, x, y);
            if (hit.hasHit()) {
                hovered_hit = hit.hits.back().hit;
                return;
            }

            hit.add(GUI_HIT::CAPTION, w->elem);
            hovered_hit = GUI_HIT::CAPTION;
            return;
        }
    }
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::LBUTTON_DOWN:
        case GUI_MSG::RBUTTON_DOWN:
        case GUI_MSG::MBUTTON_DOWN:
        case GUI_MSG::LBUTTON_UP:
        case GUI_MSG::RBUTTON_UP:
        case GUI_MSG::MBUTTON_UP:
            return onMouseButton(guiMsgToMouseBtnCode(msg), guiMsgToMouseBtnState(msg));
        case GUI_MSG::MOUSE_MOVE: {
            return onMouseMove(params.getA<int32_t>(), params.getB<int32_t>());
        }
        }
        return GuiElement::onMessage(msg, params);
    }
    void onLayout(const gfxm::vec2& extents, uint64_t flags) override {
        if (flags & GUI_LAYOUT_WIDTH_PASS) {
            for (int i = 0; i < windows.size(); ++i) {
                auto& w = windows[i];
                auto e = w->elem;

                if (e->isHidden()) {
                    continue;
                }

                if (e->size.x.unit == gui_content) {
                    e->layout(gfxm::vec2(0, 0), flags | GUI_LAYOUT_NO_BORDER | GUI_LAYOUT_NO_TITLE);
                    w->rc.max.x = w->rc.min.x + e->getBoundingRect().size().x + frame_thickness.max.x;
                } else {
                    float wnd_size = w->rc.max.x - w->rc.min.x;
                    float client_size = wnd_size - (frame_thickness.min.x + frame_thickness.max.x);
                    e->layout(gfxm::vec2(client_size, 0), flags | GUI_LAYOUT_NO_BORDER | GUI_LAYOUT_NO_TITLE);
                }
            }
        }

        if (flags & GUI_LAYOUT_HEIGHT_PASS) {
            for (int i = 0; i < windows.size(); ++i) {
                auto& w = windows[i];
                auto e = w->elem;

                if (e->isHidden()) {
                    continue;
                }

                if (e->size.y.unit == gui_content) {
                    e->layout(gfxm::vec2(0, 0), flags | GUI_LAYOUT_NO_BORDER | GUI_LAYOUT_NO_TITLE);
                    w->rc.max.y = w->rc.min.y + e->getBoundingRect().size().y + frame_thickness.max.y;
                } else {
                    float wnd_size = w->rc.max.y - w->rc.min.y;
                    float client_size = wnd_size - (frame_thickness.min.y + frame_thickness.max.y);
                    e->layout(gfxm::vec2(0, client_size), flags | GUI_LAYOUT_NO_BORDER | GUI_LAYOUT_NO_TITLE);
                }
            }
        }
        
        if (flags & GUI_LAYOUT_POSITION_PASS) {
            for (int i = 0; i < windows.size(); ++i) {
                auto& w = windows[i];
                auto e = w->elem;

                if (e->isHidden()) {
                    continue;
                }

                e->layout_position = w->rc.min + frame_thickness.min;
                auto extents = e->getBoundingRect().size();
                e->layout(extents, flags | GUI_LAYOUT_NO_BORDER | GUI_LAYOUT_NO_TITLE);
            }
        }

        return;

        for (int i = 0; i < windows.size(); ++i) {
            auto& w = windows[i];
            auto e = w->elem;

            if (e->isHidden()) {
                continue;
            }

            gfxm::vec2 wnd_size = w->rc.max - w->rc.min;
            gfxm::vec2 client_size = wnd_size - (frame_thickness.min + frame_thickness.max);
            e->layout_position = w->rc.min + frame_thickness.min;

            //e->apply_style();
            e->layout(client_size, flags | GUI_LAYOUT_NO_BORDER | GUI_LAYOUT_NO_TITLE);
            /*
            e->apply_style();
            e->layout(client_size, GUI_LAYOUT_NO_BORDER | GUI_LAYOUT_NO_TITLE | GUI_LAYOUT_WIDTH_PASS);
            e->layout(client_size, GUI_LAYOUT_NO_BORDER | GUI_LAYOUT_NO_TITLE | GUI_LAYOUT_HEIGHT_PASS);
            e->layout(client_size, GUI_LAYOUT_NO_BORDER | GUI_LAYOUT_NO_TITLE | GUI_LAYOUT_POSITION_PASS);
            e->update_selection_range(0);
            */

            if (client_size.x < .0f) {
                w->rc.max.x = w->rc.min.x + frame_thickness.min.x + frame_thickness.max.x;
            }
            if (client_size.y < .0f) {
                w->rc.max.y = w->rc.min.y + frame_thickness.min.y + frame_thickness.max.y;
            }
            /*
            if (client_size.x < e->getBoundingRect().size().x) {
                w->rc.max.x = w->rc.min.x + frame_thickness.min.x + e->getBoundingRect().size().x + frame_thickness.max.x;
            }
            if (client_size.y < e->getBoundingRect().size().y) {
                w->rc.max.y = w->rc.min.y + frame_thickness.min.y + e->getBoundingRect().size().y + frame_thickness.max.y;
            }*/
        }
    }
    void onDraw() override {
        const float WND_CORNER_RADIUS = 10.f;

        for (int i = 0; i < windows.size(); ++i) {
            auto& w = windows[i];
            auto e = w->elem;

            if (e->isHidden()) {
                continue;
            }

            gfxm::rect rc_shadow = w->rc;
            rc_shadow.min += gfxm::vec2(7, 7);
            rc_shadow.max += gfxm::vec2(7, 7);
            //guiDrawRect(rc_shadow, 0x66000000);
            guiDrawRectRound(
                rc_shadow,
                WND_CORNER_RADIUS, WND_CORNER_RADIUS, .0f, .0f,
                0x66000000
            );
            
            uint32_t col_accent = GUI_COL_ACCENT;
            uint32_t col_border_top = GUI_COL_BUTTON_HOVER;
            uint32_t col_border_left = GUI_COL_BUTTON;
            uint32_t col_border_right = GUI_COL_BUTTON;
            uint32_t col_border_bottom = GUI_COL_BUTTON_SHADOW;
            if (i == windows.size() - 1) {
                col_border_top = col_accent;
                col_border_left = col_accent;
                col_border_right = col_accent;
                col_border_bottom = col_accent;
            }

            //guiDrawRectGradient(w->rc, col_accent, col_accent, GUI_COL_BG, GUI_COL_BG);
            guiDrawRectRound(
                w->rc,
                WND_CORNER_RADIUS, WND_CORNER_RADIUS, .0f, .0f,
                GUI_COL_BG
            );
            guiDrawRectRoundBorder(
                w->rc, 
                WND_CORNER_RADIUS, WND_CORNER_RADIUS, .0f, .0f,
                2.f, 2.f, 2.f, 2.f,
                col_border_left, col_border_top, col_border_right, col_border_bottom
            );
            
            gfxm::rect rc_title(
                w->rc.min, gfxm::vec2(w->rc.max.x, w->rc.min.y + 30.f)
            );
            guiDrawText(rc_title, "Window", e->getFont(), GUI_HCENTER | GUI_VCENTER, 0xFFFFFFFF);
            
            e->draw();
        }
    }
};

