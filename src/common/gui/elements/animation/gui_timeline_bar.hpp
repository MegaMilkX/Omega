#pragma once

#include "gui/elements/element.hpp"


class GuiTimelineBar : public GuiElement {
    int cursor_frame = 0;
    bool is_pressed = false;
    float frame_screen_width = 20.0f;
    int numbered_frame_divider = 5;
    int mid_frame_divider = 0;
    int skip_divider = 1;
public:
    gfxm::vec2 content_offset = gfxm::vec2(0, 0);
    void setFrameWidth(float w) {
        frame_screen_width = w;
        guiTimelineCalcDividers(w, numbered_frame_divider, mid_frame_divider, skip_divider);
    }
    void setCursor(int frame, bool send_notification = true) {
        cursor_frame = frame;
        cursor_frame = std::max(0, cursor_frame);
        assert(getOwner());
        if (getOwner() && send_notification) {
            getOwner()->sendMessage(GUI_MSG::NOTIFY, (uint64_t)GUI_NOTIFY::TIMELINE_JUMP, cursor_frame);
        }
    }
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::LBUTTON_DOWN:
            is_pressed = true;
            setCursor((guiGetMousePos().x - client_area.min.x - 10.f + frame_screen_width * .5f + content_offset.x) / frame_screen_width);
            guiCaptureMouse(this);
            return true;
        case GUI_MSG::LBUTTON_UP:
            is_pressed = false;
            guiCaptureMouse(0);
            return true;
        case GUI_MSG::MOUSE_MOVE:
            if (is_pressed) {
                setCursor((guiGetMousePos().x - client_area.min.x - 10.f + frame_screen_width * .5f + content_offset.x) / frame_screen_width);
            }
            return true;
        }
        return false;
    }
    void onLayout(const gfxm::vec2& extents, uint64_t flags) override {
        rc_bounds = gfxm::rect(0, 0, extents.x, extents.y);
        client_area = rc_bounds;
    }
    void onDraw() override {
        guiDrawRect(client_area, GUI_COL_BG_INNER);
        guiDrawPushScissorRect(client_area);

        float bar_width = frame_screen_width * numbered_frame_divider * 2.f;
        float bar_width2 = bar_width * 2.f;
        float client_width = client_area.max.x - client_area.min.x;
        int bar_count = (ceilf(client_width / (float)bar_width) + 1) / 2 + 1;
        for (int i = 0; i < bar_count; ++i) {
            gfxm::vec2 offs_rem = gfxm::vec2(fmodf(content_offset.x, bar_width), fmodf(content_offset.y, bar_width));
            int a = content_offset.x / bar_width;
            float offs = bar_width;
            if ((a % 2) == 0) {
                offs = .0f;
            }
            guiDrawRect(
                gfxm::rect(
                    client_area.min + gfxm::vec2(i * bar_width2 + 10.0f - offs_rem.x + offs, .0f),
                    gfxm::vec2(client_area.min.x + bar_width + i * bar_width2 + 10.0f - offs_rem.x + offs, client_area.max.y)
                ), GUI_COL_BG_INNER_ALT
            );
        }
        GuiTextBuffer text;
        Font* font = getFont();
        int v_line_count = client_width / frame_screen_width + 1;
        for (int i = 0; i < v_line_count; ++i) {
            int frame_id = i + (int)(content_offset.x / frame_screen_width);
            if ((frame_id % skip_divider)) {
                continue;
            }
            gfxm::vec2 offs_rem = gfxm::vec2(fmodf(content_offset.x, frame_screen_width), fmodf(content_offset.y, frame_screen_width));
            float offs_x = client_area.min.x + 10.0f + i * frame_screen_width - offs_rem.x;
            float bar_height = (client_area.max.y - client_area.min.y) * .5f;
            
            if (mid_frame_divider > 0 && (frame_id % mid_frame_divider) == 0) {
                bar_height = (client_area.max.y - client_area.min.y) * .75f;
            }

            uint64_t color = GUI_COL_BG;
            if ((frame_id % numbered_frame_divider) == 0) {
                bar_height = (client_area.max.y - client_area.min.y) * 1.f;

                color = GUI_COL_BUTTON;
                std::string snum = MKSTR(frame_id);
                text.replaceAll(font, snum.c_str(), snum.length());
                text.prepareDraw(font, false);
                
                float text_pos_x = offs_x;
                if (client_area.max.x < text_pos_x + text.getBoundingSize().x) {
                    text_pos_x -= text.getBoundingSize().x;
                }
                text.draw(font, gfxm::vec2(text_pos_x, client_area.min.y), GUI_COL_TEXT, GUI_COL_TEXT);
            }
            guiDrawLine(
                gfxm::rect(
                    gfxm::vec2(offs_x, client_area.max.y),
                    gfxm::vec2(offs_x, client_area.max.y - bar_height)
                ), 1.f, color
            );
        }
        guiDrawLine(
            gfxm::rect(
                gfxm::vec2(client_area.min.x, client_area.max.y),
                gfxm::vec2(client_area.max.x, client_area.max.y)
            ), 1.f, GUI_COL_BUTTON
        );

        // Draw timeline cursor
        guiDrawDiamond(
            gfxm::vec2(client_area.min.x + 10.0f + cursor_frame * frame_screen_width - content_offset.x, client_area.min.y),
            5.f, GUI_COL_TIMELINE_CURSOR, GUI_COL_TIMELINE_CURSOR, GUI_COL_TIMELINE_CURSOR
        );
        guiDrawLine(gfxm::rect(
            gfxm::vec2(client_area.min.x + 10.0f + cursor_frame * frame_screen_width - content_offset.x, client_area.min.y),
            gfxm::vec2(client_area.min.x + 10.0f + cursor_frame * frame_screen_width - content_offset.x, client_area.max.y)
        ), 1.f, GUI_COL_TIMELINE_CURSOR);

        guiDrawPopScissorRect();
    }
};

