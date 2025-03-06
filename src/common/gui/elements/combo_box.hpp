#pragma once

#include "gui/elements/input.hpp"
#include "gui/elements/menu_list.hpp"


class GuiComboBoxCtrl : public GuiElement {
    GuiTextBuffer text_content;

    gfxm::vec2 pos_content;

    gfxm::vec2 mouse_pos;
    bool pressing = false;

    bool is_open = false;
    GuiIcon* current_icon = 0;

    std::unique_ptr<GuiMenuList> menu_list;
public:
    GuiComboBoxCtrl(const char* text = "ComboBox") {
        setSize(0, 0);
        setMaxSize(0, 0);
        setMinSize(0, 0);

        text_content.putString(getFont(), text, strlen(text));

        current_icon = guiLoadIcon("svg/entypo/triangle-down.svg");
        
        menu_list.reset(new GuiMenuList());
        guiGetRoot()->addChild(menu_list.get());
        menu_list->setOwner(this);
        menu_list->addItem(new GuiMenuListItem("Item 1"));
        menu_list->addItem(new GuiMenuListItem("Item 2"));
        menu_list->addItem(new GuiMenuListItem("Item 3"));
        menu_list->addItem(new GuiMenuListItem("Item 4"));
        menu_list->setHidden(true);
        menu_list->addFlags(GUI_FLAG_MENU_SKIP_OWNER_CLICK);
    }

    GuiMenuList* getMenuList() { return menu_list.get(); }

    void setValue(const char* val) {
        text_content.replaceAll(getFont(), val, strlen(val));
    }

    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::CLOSE_MENU: {
            is_open = false;
            menu_list->close();
            return GuiElement::onMessage(msg, params);
        }
        case GUI_MSG::NOTIFY: {
            switch (params.getB<GUI_NOTIFY>()) {
            case GUI_NOTIFY::MENU_COMMAND:
                is_open = false;
                menu_list->close();
                return true;
            }
            break;
        }
        case GUI_MSG::LCLICK:
        case GUI_MSG::DBL_LCLICK:
            if (!is_open) {
                is_open = true;
                menu_list->open();
                auto gbr = getGlobalBoundingRect();
                menu_list->pos = gui_vec2(gbr.min.x, gbr.max.y);
                menu_list->min_size = gui_vec2(client_area.max.x - client_area.min.x, 0);
                menu_list->max_size = gui_vec2(client_area.max.x - client_area.min.x, 0);
            } else {
                is_open = false;
                menu_list->close();
            }
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

        text_content.prepareDraw(guiGetDefaultFont(), true);

        gfxm::rect text_rc = guiLayoutPlaceRectInsideRect(
            client_area, text_content.getBoundingSize(), GUI_LEFT | GUI_VCENTER,
            gfxm::rect(GUI_MARGIN, 0, 0, 0)
        );
        pos_content = text_rc.min;

    }
    void onDraw() override {
        Font* font = getFont();
        uint32_t col_box = GUI_COL_HEADER;
        if (isHovered()) {
            col_box = GUI_COL_BUTTON;
        }
        if (is_open) {
            guiDrawRectRound(client_area, GUI_PADDING * 2.f, col_box, GUI_DRAW_CORNER_TOP);
        } else {
            guiDrawRectRound(client_area, GUI_PADDING * 2.f, col_box);
        }
        float fontH = font->getLineHeight();
        float fontLG = font->getLineGap();
        if (current_icon) {
            current_icon->draw(gfxm::rect(gfxm::vec2(client_area.max.x - fontH * 2.f, pos_content.y), gfxm::vec2(client_area.max.x - fontH, pos_content.y + fontH)), GUI_COL_TEXT);
        }
        text_content.draw(font, pos_content, GUI_COL_TEXT, GUI_COL_ACCENT);
    }
};

class GuiComboBox : public GuiElement {
    GuiLabel label;
    GuiComboBoxCtrl ctrl;
public:
    GuiComboBox(const char* caption = "ComboBox", const char* text = "Select an item...")
        : label(caption), ctrl(text) {
        setSize(gui::perc(100), gui::em(2));

        label.setOwner(this);
        label.setParent(this);
        ctrl.setOwner(this);
        ctrl.setParent(this);

        content = ctrl.getMenuList();
    }

    void setValue(const char* val) {
        ctrl.setValue(val);
    }

    void onHitTest(GuiHitResult& hit, int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return;
        }

        ctrl.hitTest(hit, x, y);
        if (hit.hasHit()) {
            return;
        }

        hit.add(GUI_HIT::CLIENT, this);
        return;
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

        gfxm::rect rc_left, rc_right;
        rc_left = client_area;
        guiLayoutSplitRect2XRatio(rc_left, rc_right, .25f);
        label.layout_position = rc_left.min;
        label.layout(gfxm::rect_size(rc_left), flags);
        ctrl.layout_position = rc_right.min;
        ctrl.layout(gfxm::rect_size(rc_right), flags);
    }
    void onDraw() override {
        label.draw();
        ctrl.draw();
    }
};
