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

    gfxm::vec2 mouse_pos;
public:
    GuiScrollBar() {}

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

    GuiHitResult hitTest(int x, int y) override {
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
    void onMessage(GUI_MSG msg, uint64_t a_param, uint64_t b_param) override {
        switch (msg) {
        case GUI_MSG::MOUSE_MOVE: {
            gfxm::vec2 cur_mouse_pos = gfxm::vec2(a_param, b_param);
            if (gfxm::point_in_rect(rc_thumb, gfxm::vec2(a_param, b_param))) {
                hovered_thumb = true;
            } else {
                hovered_thumb = false;
            }

            if (pressed_thumb) {
                if (!HORIZONTAL) {
                    current_scroll += cur_mouse_pos.y - mouse_pos.y;
                } else {
                    current_scroll += cur_mouse_pos.x - mouse_pos.x;                    
                }
                current_scroll = gfxm::_min(max_scroll, current_scroll);
                current_scroll = gfxm::_max(.0f, current_scroll);
                if (getOwner()) {
                    getOwner()->onMessage(GUI_MSG::SB_THUMB_TRACK, owner_content_max_scroll * (current_scroll / max_scroll), 0);
                }
            }
            mouse_pos = cur_mouse_pos;
            } break;
        case GUI_MSG::MOUSE_ENTER:
            hovered = true;
            break;
        case GUI_MSG::MOUSE_LEAVE: {
            hovered = false;
            hovered_thumb = false;
        } break;
        case GUI_MSG::LBUTTON_DOWN:
            if (hovered_thumb) {
                pressed_thumb = true;
                guiCaptureMouse(this);
            }
            break;
        case GUI_MSG::LBUTTON_UP:
            if (pressed_thumb) {
                guiCaptureMouse(0);
                if (hovered_thumb) {
                    // TODO: Button clicked
                    LOG_WARN("Thumb released while hovering");
                }
            }
            pressed_thumb = false;
            break;
        }

        GuiElement::onMessage(msg, a_param, b_param);
    }
    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) override {
        if (!HORIZONTAL) {
            gfxm::rect rc_scroll_v = gfxm::rect(
                gfxm::vec2(rc.max.x - 10.0f, rc.min.y),
                gfxm::vec2(rc.max.x, rc.max.y)
            );
            this->bounding_rect = rc_scroll_v;
            this->client_area = this->bounding_rect;

            max_scroll = (client_area.max.y - client_area.min.y) - thumb_h;

            gfxm::vec2 rc_thumb_min = client_area.min + gfxm::vec2(.0f, current_scroll);
            rc_thumb = gfxm::rect(
                rc_thumb_min,
                gfxm::vec2(client_area.max.x, rc_thumb_min.y + thumb_h)
            );
        } else {
            gfxm::rect rc_scroll_h = gfxm::rect(
                gfxm::vec2(rc.min.x, rc.max.y - 10.0f),
                gfxm::vec2(rc.max.x, rc.max.y)
            );
            bounding_rect = rc_scroll_h;
            client_area = bounding_rect;

            max_scroll = (client_area.max.x - client_area.min.x) - thumb_h;

            gfxm::vec2 rc_thumb_min = client_area.min + gfxm::vec2(current_scroll, .0f);
            rc_thumb = gfxm::rect(
                rc_thumb_min,
                gfxm::vec2(rc_thumb_min.x + thumb_h, client_area.max.y)
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