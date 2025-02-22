#pragma once

#include "gui/elements/element.hpp"
#include "gui/elements/list_toolbar_button.hpp"


class GuiCollapsingHeaderHeader : public GuiElement {
    GuiIcon* icon = 0;
    GuiTextBuffer caption;
    bool enable_remove_btn = false;
    bool enable_background = true;
    bool is_open = false;
    gfxm::vec2 pos_caption;
    std::unique_ptr<GuiListToolbarButton> close_btn;
public:
    GuiCollapsingHeaderHeader(const char* cap = "CollapsingHeader", bool remove_btn = false, bool enable_background = true)
        : enable_remove_btn(remove_btn), enable_background(enable_background) {
        setSize(0, 0);
        setMaxSize(0, 0);
        setMinSize(0, 0);

        icon = guiLoadIcon("svg/entypo/triangle-right.svg");
        caption.putString(getFont(), cap, strlen(cap));

        close_btn.reset(new GuiListToolbarButton(guiLoadIcon("svg/entypo/cross.svg"), GUI_MSG::COLLAPSING_HEADER_REMOVE));
        close_btn->setOwner(this);
        close_btn->setParent(this);
    }

    void setOpen(bool value) {
        is_open = value;
        if (is_open) {
            icon = guiLoadIcon("svg/entypo/triangle-down.svg");
        } else {
            icon = guiLoadIcon("svg/entypo/triangle-right.svg");
        }
    }

    void onHitTest(GuiHitResult& hit, int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return;
        }
        
        if (enable_remove_btn) {
            close_btn->onHitTest(hit, x, y);
            if (hit.hasHit()) {
                return;
            }
        }

        hit.add(GUI_HIT::CLIENT, this);
        return;
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
        Font* font = getFont();

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
            close_btn->layout(rc, flags);
        }
        
    }
    void onDraw() override {
        uint32_t col_box = GUI_COL_BUTTON;
        if (isHovered()) {
            col_box = GUI_COL_BUTTON_HOVER;
        }
        if (enable_background) {
            float radius = gui_to_px(gui::em(.5f), getFont(), getClientSize().y);
            guiDrawRectRound(client_area, radius, col_box);
        }

        Font* font = getFont();
        float fontH = font->getLineHeight();
        float fontLG = font->getLineGap();
        if (icon) {
            icon->draw(gfxm::rect(pos_caption, pos_caption + gfxm::vec2(fontH, fontH)), GUI_COL_TEXT);
        }
        caption.draw(font, pos_caption + gfxm::vec2(fontH + GUI_MARGIN, .0f), GUI_COL_TEXT, GUI_COL_ACCENT);

        if (enable_remove_btn) {
            close_btn->draw();
        }
    }
};

class GuiCollapsingHeader : public GuiElement {
    bool is_open = false;
    GuiCollapsingHeaderHeader* header = 0;
public:
    void* user_ptr = 0;

    std::function<void(void)> on_remove;

    GuiCollapsingHeader(const char* caption = "CollapsingHeader", bool remove_btn = false, bool enable_background = true, void* user_ptr = 0)
    : user_ptr(user_ptr) {
        setSize(0, 0);
        setMaxSize(0, 0);
        setMinSize(0, 0);
        overflow = GUI_OVERFLOW_FIT;
        setStyleClasses({ "control", "collapsing-header"});

        header = new GuiCollapsingHeaderHeader(caption, remove_btn, enable_background);
        guiAdd(this, this, header, GUI_FLAG_PERSISTENT | GUI_FLAG_FRAME);

        addFlags(GUI_FLAG_HIDE_CONTENT);
    }

    void setOpen(bool value) {
        is_open = value;
        if (is_open) {
            removeFlags(GUI_FLAG_HIDE_CONTENT);
        } else {
            addFlags(GUI_FLAG_HIDE_CONTENT);
        }
        header->setOpen(value);
    }

    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::COLLAPSING_HEADER_REMOVE: {
            guiSendMessage(getOwner(), GUI_MSG::COLLAPSING_HEADER_REMOVE, this, 0, 0);
            if (on_remove) {
                on_remove();
            }
            return true;
        }
        case GUI_MSG::NOTIFY: {
            switch (params.getA<GUI_NOTIFY>()) {
            case GUI_NOTIFY::COLLAPSING_HEADER_TOGGLE: {
                if (!is_open) {
                    is_open = true;
                    removeFlags(GUI_FLAG_HIDE_CONTENT);
                } else {
                    is_open = false;
                    addFlags(GUI_FLAG_HIDE_CONTENT);
                }
                return true;
            }
            }
            break;
        }
        }

        return GuiElement::onMessage(msg, params);
    }
};
