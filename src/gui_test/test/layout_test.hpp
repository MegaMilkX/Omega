#pragma once

#include "gui/elements/window.hpp"


class GuiLayoutTestWindow : public GuiWindow {
    struct RECT {
        uint32_t color;
        gfxm::vec2 pos;
        gfxm::vec2 size;

        gfxm::vec2 current_size;

        void layout(const gfxm::vec2 available_size) {
            //current_size = size;
            current_size = gfxm::vec2(
                size.x == .0f ? available_size.x : size.x,
                size.y == .0f ? available_size.y : size.y
            );
        }
    };
    static const int RECT_COUNT = 5;
    RECT rects[RECT_COUNT] = {
        RECT{
            GUI_COL_BG_DARK,
            gfxm::vec2(0, 0),
            gfxm::vec2(0, 250)
        },
        RECT{
            GUI_COL_RED,
            gfxm::vec2(150, 0),
            gfxm::vec2(200, 50)
        },
        RECT{
            GUI_COL_GREEN,
            gfxm::vec2(160, 200),
            gfxm::vec2(150, 25)
        },
        RECT{
            0xFFFFAAAA,
            gfxm::vec2(200, 160),
            gfxm::vec2(200, 50)
        },
        RECT{
            GUI_COL_ACCENT,
            gfxm::vec2(300, 300),
            gfxm::vec2(150, 25)
        }
    };
public:
    GuiTextBuffer text;
    GuiLayoutTestWindow()
        : GuiWindow("LayoutTest"), text(guiGetDefaultFont()) {
    }
    void onHitTest(GuiHitResult& hit, int x, int y) override {
        for (int i = 0; i < RECT_COUNT; ++i) {
            // TODO: 
        }
        GuiWindow::onHitTest(hit, x, y);
    }
    void onDraw() {
        gfxm::vec2 containing_size = client_area.max - client_area.min;

        int line_elem_idx = 0;
        float line_x = .0f;
        float line_y = .0f;
        float line_height = .0f;
        for (int i = 0; i < RECT_COUNT; ++i) {
            auto& rect = rects[i];
            gfxm::vec2 available_size(containing_size.x - line_x, containing_size.y - line_y);
            rect.layout(available_size);
            gfxm::vec2 pos = gfxm::vec2(line_x, line_y);
            gfxm::vec2 size = rect.current_size;
            uint32_t color = rect.color;

            if (pos.x + size.x >= containing_size.x && line_elem_idx > 0) {
                line_y += line_height;
                line_x = .0f;
                line_height = .0f;
                line_elem_idx = 0;
                --i;
                continue;
            }

            gfxm::rect rc(
                client_area.min + pos,
                client_area.min + pos + size
            );
            guiDrawRectRound(rc, 10.f, color);

            line_x += size.x;
            line_height = gfxm::_max(line_height, size.y);
            ++line_elem_idx;
        }

        text.replaceAll("Hello", strlen("Hello"));
        text.prepareDraw(guiGetCurrentFont(), true);
        text.draw(client_area.min, GUI_COL_TEXT, GUI_COL_ACCENT);
    }
};
