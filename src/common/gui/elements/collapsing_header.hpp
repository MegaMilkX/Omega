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
        setSize(gui::fill(), gui::em(2));
        //setMaxSize(gui::perc(100), gui::em(2));
        //setMinSize(gui::perc(100), gui::em(2));

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
            close_btn->hitTest(hit, x, y);
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
    void onLayout(const gfxm::vec2& extents, uint64_t flags) override {
        Font* font = getFont();

        const float text_box_height = font->getLineHeight() * 2.0f;
        setHeight(text_box_height);
        rc_bounds = gfxm::rect(
            gfxm::vec2(0, 0),
            gfxm::vec2(extents.x, text_box_height)
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
            close_btn->layout_position = rc.min;
            close_btn->layout(gfxm::rect_size(rc), flags);
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
    GuiElement* content_box = 0;
public:
    void* user_ptr = 0;

    std::function<void(void)> on_remove;

    GuiCollapsingHeader(const char* caption = "CollapsingHeader", bool remove_btn = false, bool enable_background = true, void* user_ptr = 0)
    : user_ptr(user_ptr) {
        setSize(gui::perc(100), gui::content());
        setStyleClasses({ "control", "collapsing-header" });

        header = new GuiCollapsingHeaderHeader(caption, remove_btn, enable_background);
        header->setOwner(this);
        pushBack(header);

        content_box = new GuiElement();
        content_box->setSize(gui::perc(100), gui::content());
        content_box->setStyleClasses({ "collapsing-header-content" });
        content_box->setHidden(true);
        pushBack(content_box);

        content = content_box;
    }

    void setOpen(bool value) {
        is_open = value;
        if (is_open) {
            content_box->setHidden(false);
        } else {
            content_box->setHidden(true);
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
                    content_box->setHidden(false);
                } else {
                    is_open = false;
                    content_box->setHidden(true);
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
