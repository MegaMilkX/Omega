#pragma once

#include "gui/elements/gui_element.hpp"
#include "gui/elements/gui_window_title_bar_button.hpp"


class GuiCollapsingHeaderHeader : public GuiElement {
    GuiIcon* icon = 0;
    GuiTextBuffer caption;
    bool enable_remove_btn = false;
    bool enable_background = true;
    bool is_open = false;
    gfxm::vec2 pos_caption;
    GuiWindowTitleBarButton close_btn = GuiWindowTitleBarButton(guiLoadIcon("svg/entypo/cross.svg"), GUI_MSG::COLLAPSING_HEADER_REMOVE);
public:
    GuiCollapsingHeaderHeader(const char* cap = "CollapsingHeader", bool remove_btn = false, bool enable_background = true)
        : caption(guiGetDefaultFont()), enable_remove_btn(remove_btn), enable_background(enable_background) {
        setSize(0, 0);
        setMaxSize(0, 0);
        setMinSize(0, 0);

        icon = guiLoadIcon("svg/entypo/triangle-right.svg");
        caption.putString(cap, strlen(cap));
        close_btn.setOwner(this);
        close_btn.setParent(this);
    }
    GuiHitResult onHitTest(int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }

        if (enable_remove_btn) {
            auto hit = close_btn.onHitTest(x, y);
            if (hit.hasHit()) {
                return hit;
            }
        }

        return GuiHitResult{ GUI_HIT::CLIENT, this };
    }
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::LCLICK:
        case GUI_MSG::DBL_LCLICK:
            if (!is_open) {                
                is_open = true;
                icon = guiLoadIcon("svg/entypo/triangle-down.svg");
            } else {
                is_open = false;
                icon = guiLoadIcon("svg/entypo/triangle-right.svg");
            }
            notifyOwner(GUI_NOTIFY::COLLAPSING_HEADER_TOGGLE, 0);
            return true;
        }

        return GuiElement::onMessage(msg, params);
    }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        Font* font = guiGetCurrentFont()->font;

        const float text_box_height = font->getLineHeight() * 2.0f;
        setHeight(text_box_height);
        rc_bounds = gfxm::rect(
            rc.min,
            gfxm::vec2(rc.max.x, rc.min.y + text_box_height)
        );
        client_area = rc_bounds;

        caption.prepareDraw(guiGetDefaultFont(), false);

        gfxm::rect text_rc = guiLayoutPlaceRectInsideRect(
            client_area, caption.getBoundingSize(), GUI_LEFT | GUI_VCENTER,
            gfxm::rect(enable_background ? GUI_MARGIN : 0, 0, 0, 0)
        );
        pos_caption = text_rc.min;

        if(enable_remove_btn) {
            float icon_sz = client_area.max.y - client_area.min.y;
            gfxm::rect rc;
            rc.max = client_area.max;
            rc.min = rc.max - gfxm::vec2(icon_sz, icon_sz);
            close_btn.layout(rc, flags);
        }
        
    }
    void onDraw() override {
        uint32_t col_box = GUI_COL_BUTTON;
        if (isHovered()) {
            col_box = GUI_COL_BUTTON_HOVER;
        }
        if (enable_background) {
            guiDrawRectRound(client_area, GUI_PADDING * 2.f, col_box);
        }

        float fontH = guiGetCurrentFont()->font->getLineHeight();
        float fontLG = guiGetCurrentFont()->font->getLineGap();
        if (icon) {
            icon->draw(gfxm::rect(pos_caption, pos_caption + gfxm::vec2(fontH, fontH)), GUI_COL_TEXT);
        }
        caption.draw(pos_caption + gfxm::vec2(fontH + GUI_MARGIN, .0f), GUI_COL_TEXT, GUI_COL_ACCENT);

        if (enable_remove_btn) {
            close_btn.draw();
        }
    }
};

class GuiCollapsingHeader : public GuiElement {
    bool is_open = false;
    GuiCollapsingHeaderHeader header;
public:
    GuiCollapsingHeader(const char* caption = "CollapsingHeader", bool remove_btn = false, bool enable_background = true)
    : header(caption, remove_btn, enable_background) {
        setSize(0, 0);
        setMaxSize(0, 0);
        setMinSize(0, 0);
        header.setOwner(this);
        header.setParent(this);
    }
    GuiHitResult onHitTest(int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }

        auto hit = header.onHitTest(x, y);
        if (hit.hasHit()) {
            return hit;
        }

        if (is_open) {
            for (auto& ch : children) {
                GuiHitResult hit = ch->onHitTest(x, y);
                if (hit.hasHit()) {
                    return hit;
                }
            }
        }

        return GuiHitResult{ GUI_HIT::CLIENT, this };
    }
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::COLLAPSING_HEADER_REMOVE: {
            guiSendMessage(getOwner(), GUI_MSG::COLLAPSING_HEADER_REMOVE, this, 0, 0);
            return true;
        }
        case GUI_MSG::NOTIFY: {
            switch (params.getA<GUI_NOTIFY>()) {
            case GUI_NOTIFY::COLLAPSING_HEADER_TOGGLE: {
                if (!is_open) {                
                    is_open = true;
                } else {
                    is_open = false;
                }
                return true;
            }
            }
            break;
        }
        }

        return GuiElement::onMessage(msg, params);
    }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        Font* font = guiGetCurrentFont()->font;

        const float text_box_height = font->getLineHeight() * 2.0f;
        setHeight(text_box_height);
        rc_bounds = gfxm::rect(
            rc.min,
            gfxm::vec2(rc.max.x, rc.min.y + text_box_height)
        );
        client_area = rc_bounds;

        header.layout(client_area, flags);

        // TODO:
        if (is_open) {
            gfxm::rect rc = client_area;
            rc.min.y = header.getClientArea().max.y;

            layoutContentTopDown(rc);

            client_area.max.y = rc_content.max.y;
            rc_bounds = client_area;
        }
    }
    void onDraw() override {
        header.draw();

        if (is_open) {
            guiDrawPushScissorRect(client_area);
            drawContent();
            guiDrawPopScissorRect();
        }
    }
};
