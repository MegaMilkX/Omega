#pragma once

#include "gui/elements/gui_element.hpp"
#include "gui/gui_system.hpp"


template<bool HORIZONTAL>
class GuiScrollBar : public GuiElement {
    gfxm::rect rc_thumb;

    bool hovered = false;
    bool hovered_thumb = false;
    bool pressed_thumb = false;

    float thumb_h = 10.0f;
    float current_scroll = .0f;
    float max_scroll = .0f;

    float owner_content_max_scroll = .0f;

    float scroll_from = .0f;
    float scroll_to = 100.f;
    float page_length = 50.f;
    float scroll_pos = .0f;

    float thumb_len_px = .0f;
    float thumb_pos_px = .0f;

    gfxm::vec2 mouse_pos;
    float mouse_press_offset;
public:
    GuiScrollBar() {
        if (HORIZONTAL) {
            setSize(.0f, 10.f);
        } else {
            setSize(10.f, .0f);
        }
    }

    void setScrollBounds(float from, float to) {
        scroll_from = from;
        scroll_to = to;
    }
    void setScrollPageLength(float len) {
        page_length = len;
    }
    void setScrollPosition(float pos) {
        scroll_pos = pos;
    }

    void setScrollData(float page_height, float total_content_height) {
        int full_page_count = (int)(total_content_height / page_height);
        int page_count = full_page_count + 1;
        float diff = page_height * page_count - total_content_height;
        owner_content_max_scroll = full_page_count * page_height - diff;

        float ratio = page_height / total_content_height;
        if (!HORIZONTAL) {
            thumb_h = (client_area.max.y - client_area.min.y) * ratio;
            thumb_h = gfxm::_max(10.0f, thumb_h);
            thumb_h = gfxm::_min((client_area.max.y - client_area.min.y), thumb_h);
        } else {
            thumb_h = (client_area.max.x - client_area.min.x) * ratio;
            thumb_h = gfxm::_max(10.0f, thumb_h);
            thumb_h = gfxm::_min((client_area.max.x - client_area.min.x), thumb_h);
        }
    }

    void setOffset(float offset) {
        //current_scroll = max_scroll * (offset / owner_content_max_scroll);
    }

    GuiHitResult onHitTest(int x, int y) override {
        if (!HORIZONTAL) {
            if (gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
                return GuiHitResult{ GUI_HIT::VSCROLL, this };
            }
        } else {
            if (gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
                return GuiHitResult{ GUI_HIT::HSCROLL, this };
            }
        }
        return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
    }
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::MOUSE_MOVE: {
            gfxm::vec2 cur_mouse_pos = gfxm::vec2(params.getA<int32_t>(), params.getB<int32_t>());
            if (gfxm::point_in_rect(rc_thumb, cur_mouse_pos)) {
                hovered_thumb = true;
            } else {
                hovered_thumb = false;
            }

            if (pressed_thumb) {
                float total_scroll_len = scroll_to == scroll_from ? 1.f : (scroll_to - scroll_from);
                if (!HORIZONTAL) {
                    float cursor_ratio = (cur_mouse_pos.y - client_area.min.y - mouse_press_offset) / (client_area.max.y - client_area.min.y);
                    scroll_pos = cursor_ratio * total_scroll_len;
                    scroll_pos = gfxm::_max(scroll_from, gfxm::_min(scroll_to - page_length, scroll_pos));
                    notifyOwner<float>(GUI_NOTIFY::SCROLL_V, scroll_from + scroll_pos);
                } else {
                    float cursor_ratio = (cur_mouse_pos.x - client_area.min.x - mouse_press_offset) / (client_area.max.x - client_area.min.x);
                    scroll_pos = cursor_ratio * total_scroll_len;
                    scroll_pos = gfxm::_max(scroll_from, gfxm::_min(scroll_to - page_length, scroll_pos));
                    notifyOwner<float>(GUI_NOTIFY::SCROLL_H, scroll_from + scroll_pos);
                }
            } else {
                if (!HORIZONTAL) {
                    mouse_press_offset = (mouse_pos.y - thumb_pos_px);
                } else {
                    mouse_press_offset = (mouse_pos.x - thumb_pos_px);
                }
            }
            mouse_pos = cur_mouse_pos;
            } return true;
        case GUI_MSG::MOUSE_ENTER:
            hovered = true;
            return true;
        case GUI_MSG::MOUSE_LEAVE: {
            hovered = false;
            hovered_thumb = false;
        } return true;
        case GUI_MSG::LBUTTON_DOWN:
            if (hovered_thumb) {
                pressed_thumb = true;
                guiCaptureMouse(this);
            }
            return true;
        case GUI_MSG::LBUTTON_UP:
            if (pressed_thumb) {
                guiCaptureMouse(0);
                if (hovered_thumb) {
                    // TODO: Button clicked
                    LOG_WARN("Thumb released while hovering");
                }
            }
            pressed_thumb = false;
            return true;
        }

        return GuiElement::onMessage(msg, params);
    }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        float total_scroll_len = scroll_to == scroll_from ? 1.f : (scroll_to - scroll_from);
        float thumb_ratio = gfxm::_min(1.f, page_length / total_scroll_len);
        float thumb_pos_ratio = scroll_pos / total_scroll_len;
        if (!HORIZONTAL) {
            gfxm::rect rc_scroll_v = gfxm::rect(
                gfxm::vec2(rc.max.x - 10.0f, rc.min.y),
                gfxm::vec2(rc.max.x, rc.max.y)
            );
            rc_bounds = rc_scroll_v;
            client_area = this->rc_bounds;

            float bar_len_px = client_area.max.y - client_area.min.y;
            thumb_len_px = bar_len_px * thumb_ratio;
            thumb_pos_px = client_area.min.y + bar_len_px * thumb_pos_ratio;

            rc_thumb = gfxm::rect(
                gfxm::vec2(client_area.min.x, thumb_pos_px),
                gfxm::vec2(client_area.max.x, thumb_pos_px + thumb_len_px)
            );
        } else {
            gfxm::rect rc_scroll_h = gfxm::rect(
                gfxm::vec2(rc.min.x, rc.max.y - 10.0f),
                gfxm::vec2(rc.max.x, rc.max.y)
            );
            rc_bounds = rc_scroll_h;
            client_area = rc_bounds;

            float bar_len_px = client_area.max.x - client_area.min.x;
            thumb_len_px = bar_len_px * thumb_ratio;
            thumb_pos_px = client_area.min.x + bar_len_px * thumb_pos_ratio;

            rc_thumb = gfxm::rect(
                gfxm::vec2(thumb_pos_px, client_area.min.y),
                gfxm::vec2(thumb_pos_px + thumb_len_px, client_area.max.y)
            );
        }
    }
    void onDraw() override {
        if (!isEnabled()) {
            return;
        }
        guiDrawRectRound(client_area, 5.f, GUI_COL_HEADER);

        uint32_t col = GUI_COL_BUTTON;
        if (pressed_thumb) {
            col = GUI_COL_ACCENT;
        } else if (hovered_thumb) {
            col = GUI_COL_BUTTON_HOVER;
        }
        guiDrawRectRound(rc_thumb, 5.f, col);
    }
};

typedef GuiScrollBar<false> GuiScrollBarV;
typedef GuiScrollBar<true> GuiScrollBarH;