#pragma once

#include "gui_host.hpp"
#include "gui/gui_draw.hpp"

#include "gpu/gpu_texture_2d.hpp"

enum GuiHostInputState {
    GuiHostInput_Idle,
    GuiHostInput_Moving
};

enum GuiMouseButton {
    GuiMouseButtonNone,
    GuiMouseButtonLeft,
    GuiMouseButtonRight,
    GuiMouseButtonMiddle,
};
inline GuiMouseButton guiMsgToMouseBtnCode(GUI_MSG msg) {
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
inline bool guiMsgToMouseBtnState(GUI_MSG msg) {
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

class GuiRootHost : public GuiHost {
    gfxm::vec2 extents;
    gfxm::vec2 mouse_pos;

    struct Window {
        gfxm::rect rc;
        GuiElement* elem;
        Window(GuiElement* e, float minx, float miny, float maxx, float maxy)
            : elem(e), rc(minx, miny, maxx, maxy) {}
    };

    std::vector<std::unique_ptr<Window>> windows;

    gfxm::vec2 default_window_pos = gfxm::vec2(50, 50);
    const gfxm::rect frame_thickness = gfxm::rect(2.f, 30.f, 2.f, 2.f);

    const long long DOUBLE_CLICK_TIME = 500;
    struct {
        GuiMouseButton btn = GuiMouseButtonNone;
        GuiElement* elem = nullptr;
        std::chrono::time_point<std::chrono::system_clock> tp = {};
    } last_click_data;

    Window* hovered_window = nullptr;
    GuiElement* hovered_elem = nullptr;
    GuiElement* pressed_elem = nullptr;
    //GuiElement* focused_elem = nullptr;
    GUI_HIT hovered_hit = GUI_HIT::NOWHERE;
    GuiHitResult hit_result;
    GuiHostInputState input_state = GuiHostInput_Idle;
    Window* moved_window = nullptr;

    int modifier_keys_state = 0;

    RHSHARED<gpuTexture2d> background_texture;

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

    void updateHovered(const GuiHitResult& hit) {
        GuiElement* new_hovered = nullptr;

        if (!hit_result.hits.empty()) {
            new_hovered = hit_result.hits.back().elem;
            hovered_hit = hit_result.hits.back().hit;
        } else {
            new_hovered = nullptr;
            hovered_hit = GUI_HIT::NOWHERE;
        }

        // TODO: guiIsHovered() should call GuiHost::isHovered(GuiElement*)

        if (hovered_elem != new_hovered) {
            if (hovered_elem) {
                hovered_elem->sendMessage(GUI_MSG::MOUSE_LEAVE, 0, 0);
                hovered_elem->removeFlags(GUI_FLAG_HOVERED);
            }
            hovered_elem = new_hovered;
            if (hovered_elem) {
                hovered_elem->addFlags(GUI_FLAG_HOVERED);
                hovered_elem->sendMessage(GUI_MSG::MOUSE_ENTER, 0, 0);
            }
        }
    }
    
    void tryFocus(GuiElement* elem) {
        guiSetFocusedWindow(elem);
        /*
        auto e = elem;
        while (e) {
            if (e->hasFlags(GUI_FLAG_FOCUSABLE)) {
                break;
            }
            e = e->getParent();
        }

        if (focused_elem == e) {
            return;
        }

        if (focused_elem) {
            focused_elem->sendMessage(GUI_MSG::UNFOCUS, 0, 0);
        }
        focused_elem = e;
        if(focused_elem) {
            focused_elem->sendMessage(GUI_MSG::FOCUS, 0, 0);
        }*/
    }

    bool onLeftMouseButton(bool pressed) {
        if (pressed) {
            if (hovered_window) {
                windowToFront(hovered_window);
                if (hovered_hit == GUI_HIT::CAPTION) {
                    input_state = GuiHostInput_Moving;
                    moved_window = hovered_window;
                }
            }
            if (hovered_elem) {
                pressed_elem = hovered_elem;
                hovered_elem->setStyleDirty();
                hovered_elem->sendMessage(GUI_MSG::LBUTTON_DOWN, GUI_MSG_PARAMS());
                tryFocus(hovered_elem);
            }
            return hovered_window || hovered_elem;
        } else {
            input_state = GuiHostInput_Idle;
            moved_window = nullptr;

            if (hovered_elem) {
                hovered_elem->setStyleDirty();
                hovered_elem->sendMessage(GUI_MSG::LBUTTON_UP, 0, 0);
            }

            if (pressed_elem) {
                if(pressed_elem == hovered_elem) {
                    std::chrono::time_point<std::chrono::system_clock> now
                        = std::chrono::system_clock::now();
                    long long time_since_last_click
                        = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_click_data.tp).count();

                    if (pressed_elem == last_click_data.elem
                        && time_since_last_click <= DOUBLE_CLICK_TIME
                        && last_click_data.btn == GuiMouseButtonLeft
                    ) {
                        last_click_data.tp = std::chrono::time_point<std::chrono::system_clock>();
                        last_click_data.btn = GuiMouseButtonLeft;
                        pressed_elem->sendMessage<uint32_t, uint32_t>(
                            GUI_MSG::DBL_LCLICK,
                            mouse_pos.x, mouse_pos.y
                        );
                    } else {
                        last_click_data.elem = pressed_elem;
                        last_click_data.tp = now;
                        last_click_data.btn = GuiMouseButtonLeft;
                        pressed_elem->sendMessage<uint32_t, uint32_t>(
                            GUI_MSG::LCLICK,
                            mouse_pos.x, mouse_pos.y
                        );
                    }
                }

                pressed_elem->setStyleDirty();
                pressed_elem = nullptr;
            }
        }
    }

    bool onMouseButton(GuiMouseButton btn, bool pressed) {
        switch (btn) {
        case GuiMouseButtonLeft:
            return onLeftMouseButton(pressed);
        case GuiMouseButtonRight:
            if (pressed) {
                if (hovered_elem) {
                    hovered_elem->setStyleDirty();
                    hovered_elem->sendMessage(GUI_MSG::RBUTTON_DOWN, GUI_MSG_PARAMS());
                }
            } else {
                if (hovered_elem) {
                    hovered_elem->setStyleDirty();
                    hovered_elem->sendMessage(GUI_MSG::RBUTTON_UP, 0, 0);
                }
            }
            break;
        case GuiMouseButtonMiddle:
            if (pressed) {
                if (hovered_elem) {
                    hovered_elem->setStyleDirty();
                    hovered_elem->sendMessage(GUI_MSG::MBUTTON_DOWN, GUI_MSG_PARAMS());
                }
            } else {
                if (hovered_elem) {
                    hovered_elem->setStyleDirty();
                    hovered_elem->sendMessage(GUI_MSG::MBUTTON_UP, 0, 0);
                }
            }
            break;
        }
        return false;
    }

    bool onMouseMove(int x, int y) {
        gfxm::vec2 prev_mouse_pos = mouse_pos;
        mouse_pos = gfxm::vec2(x, y);
        gfxm::vec2 mouse_delta = mouse_pos - prev_mouse_pos;
        
        if(input_state == GuiHostInput_Idle) {
            hit_result.clear();
            hitTest(hit_result, x, y);

            updateHovered(hit_result);
            if (hovered_elem) {
                auto rc = hovered_elem->getGlobalClientArea();
                gfxm::vec2 mouse_local = mouse_pos - rc.min;
                hovered_elem->sendMessage<int32_t, int32_t>(GUI_MSG::MOUSE_MOVE, mouse_local.x, mouse_local.y);
                return true;
            }
            return false;
        } else if (input_state == GuiHostInput_Moving) {
            moved_window->rc.min += mouse_delta;
            moved_window->rc.max += mouse_delta;
            return true;
        }
        return false;
    }
public:
    GuiRootHost() {
        background_texture = resGet<gpuTexture2d>("images/wallhaven-xee3wz_1920x1080.png");
    }

    bool isMouseCaptured() override {
        return hovered_hit != GUI_HIT::NOWHERE || hit_result.hasHit(); // TODO: or mouse captured by an element
    }
    void insert(GuiElement* e) override {
        float width = e->size.x.value;
        float height = e->size.y.value;
        width = gfxm::_max(width, frame_thickness.min.x + frame_thickness.max.x);
        height = gfxm::_max(height, frame_thickness.min.y + frame_thickness.max.y);
        /*if (width == .0f || e->size.x.unit == gui_unit::gui_content || e->size.x.unit == gui_unit::gui_fill) {
            width = 500.f;
        }
        if (height == .0f || e->size.y.unit == gui_unit::gui_content || e->size.y.unit == gui_unit::gui_fill) {
            height = 300.f;
        }*/
        auto ptr = new Window(
            e, default_window_pos.x, default_window_pos.y,
            default_window_pos.x + width, default_window_pos.y + height
        );
        windows.push_back(
            std::unique_ptr<Window>(
                ptr
            )
        );

        //default_window_pos += gfxm::vec2(20, 20);
        default_window_pos += gfxm::vec2(600, 200);

        e->_setHost(this);
    }
    void remove(GuiElement* e) override {
        for (int i = 0; i < windows.size(); ++i) {
            if (windows[i]->elem == e) {
                windows.erase(windows.begin() + i);
                break;
            }
        }
    }

    void hitTest(GuiHitResult& hit, int x, int y) override {
        hovered_window = nullptr;

        for (int i = windows.size() - 1; i >= 0; --i) {
            auto& w = windows[i];
            if (!gfxm::point_in_rect(w->rc, gfxm::vec2(x, y))) {
                continue;
            }

            hovered_window = w.get();
            
            w->elem->hitTest(hit, x, y);
            if (hit.hasHit()) {
                return;
            }

            hit.add(GUI_HIT::CAPTION, w->elem);
            return;
        }
    }
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) {
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
        case GUI_MSG::MOUSE_SCROLL:
            if (hovered_elem) {
                hovered_elem->sendMessage(msg, params);
                return true;
            }
            return false;
        }
        return false;
    }
    void layout(const gfxm::vec2& sz) override {
        extents = sz;

        for (int i = 0; i < windows.size(); ++i) {
            auto& w = windows[i];
            auto e = w->elem;

            gfxm::vec2 wnd_size = w->rc.max - w->rc.min;
            gfxm::vec2 client_size = wnd_size - (frame_thickness.min + frame_thickness.max);
            e->layout_position = w->rc.min + frame_thickness.min;

            e->apply_style();
            e->layout(client_size, GUI_LAYOUT_NO_BORDER | GUI_LAYOUT_NO_TITLE | GUI_LAYOUT_WIDTH_PASS);
            e->layout(client_size, GUI_LAYOUT_NO_BORDER | GUI_LAYOUT_NO_TITLE | GUI_LAYOUT_HEIGHT_PASS);
            e->layout(client_size, GUI_LAYOUT_NO_BORDER | GUI_LAYOUT_NO_TITLE | GUI_LAYOUT_POSITION_PASS);
            e->update_selection_range(0);

            if (client_size.x < e->getBoundingRect().size().x) {
                w->rc.max.x = w->rc.min.x + frame_thickness.min.x + e->getBoundingRect().size().x + frame_thickness.max.x;
            }
            if (client_size.y < e->getBoundingRect().size().y) {
                w->rc.max.y = w->rc.min.y + frame_thickness.min.y + e->getBoundingRect().size().y + frame_thickness.max.y;
            }
        }
    }
    void draw() override {
        //guiDrawRectTextured(gfxm::rect(0, 0, extents.x, extents.y), background_texture.get(), 0xFFFFFFFF);
        //guiDrawRectRound(gfxm::rect(0, 0, extents.x, extents.y), 20.f, 20.f, 20.f, 20.f, 0xFF884466);

        const float WND_CORNER_RADIUS = 10.f;

        for (int i = 0; i < windows.size(); ++i) {
            auto& w = windows[i];
            auto e = w->elem;

            gfxm::rect rc_shadow = w->rc;
            rc_shadow.min += gfxm::vec2(7, 7);
            rc_shadow.max += gfxm::vec2(7, 7);
            //guiDrawRect(rc_shadow, 0x66000000);
            guiDrawRectRound(
                rc_shadow,
                WND_CORNER_RADIUS, WND_CORNER_RADIUS, .0f, .0f,
                0x66000000
            );
            
            uint32_t col_accent = 0xffa4b702;
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

        // DEBUG
        if(0) {
            if (hovered_elem) {
                guiDrawRectLine(hovered_elem->getGlobalBoundingRect(), 0xFFFFFFFF);
            }
            if (pressed_elem) {
                gfxm::rect rc = pressed_elem->getGlobalBoundingRect();
                gfxm::expand(rc, 1);
                guiDrawRectLine(rc, 0xFF0000FF);
            }
            if (auto focused_elem = guiGetFocusedWindow()) {
                gfxm::rect rc = focused_elem->getGlobalBoundingRect();
                gfxm::expand(rc, 2);
                guiDrawRectLine(rc, 0xFF00FFFF);
            }
        }
    }
};