#pragma once

#include "gui/gui_system.hpp"
#include "gui/elements/element.hpp"
#include "gui/gui_icon.hpp"


class GuiMenuList;
class GuiMenuListItem : public GuiElement {
    GuiTextBuffer caption;
    bool is_open = false;
    std::unique_ptr<GuiMenuList> menu_list;
    GuiIcon* icon_arrow = 0;
public:
    std::function<void(void)> on_click;

    int id = 0;
    int command_identifier = 0;
    void* user_ptr = 0;

    void open();
    void close();

    bool hasList() { return menu_list.get() != nullptr; }

    GuiMenuListItem(const char* cap, std::function<void(void)> on_click)
        : on_click(on_click) {
        setSize(gui::perc(100), 0);
        caption.replaceAll(getFont(), cap, strlen(cap));
        icon_arrow = guiLoadIcon("svg/entypo/triangle-right.svg");
    }
    GuiMenuListItem(const char* cap = "MenuListItem", int cmd = 0)
        : command_identifier(cmd) {
        setSize(gui::perc(100), 0);
        caption.replaceAll(getFont(), cap, strlen(cap));
        icon_arrow = guiLoadIcon("svg/entypo/triangle-right.svg");
    }
    GuiMenuListItem(const char* cap, const std::initializer_list<GuiMenuListItem*>& child_items);
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) {
        switch (msg) {
        case GUI_MSG::CLOSE_MENU:
            return true;
        case GUI_MSG::MOUSE_ENTER:
            notifyOwner(GUI_NOTIFY::MENU_ITEM_HOVER, id);
            break;
        case GUI_MSG::LCLICK:
        case GUI_MSG::DBL_LCLICK:
            if (hasList()) {
                open();
                //notifyOwner(GUI_NOTIFY::MENU_ITEM_CLICKED, id);
            } else {
                notifyOwner(GUI_NOTIFY::MENU_COMMAND, command_identifier);
                if (on_click) {
                    on_click();
                }
            }
            return true;
        case GUI_MSG::NOTIFY:
            switch (params.getA<GUI_NOTIFY>()) {
            case GUI_NOTIFY::MENU_COMMAND:
                close();
                forwardMessageToOwner(msg, params);
                return true;
            }
            break;
        }

        return GuiElement::onMessage(msg, params);
    }
    void onLayout(const gfxm::vec2& extents, uint64_t flags) override {
        Font* font = getFont();
        caption.prepareDraw(font, false);
        const float h = font->getLineHeight() * 2.0f;
        const float w = gfxm::_max(extents.x, caption.getBoundingSize().x + GUI_MARGIN * 2.f);
        rc_bounds = gfxm::rect(gfxm::vec2(0, 0), gfxm::vec2(w, h));
        client_area = rc_bounds;
    }
    void onDraw() override {
        Font* font = getFont();
        if (isHovered()) {
            guiDrawRect(client_area, GUI_COL_BUTTON);
        }
        {
            gfxm::rect rc = client_area;
            rc.min.x += GUI_MARGIN;
            caption.draw(font, rc, GUI_LEFT | GUI_VCENTER, GUI_COL_TEXT, GUI_COL_ACCENT);
            //caption.draw(client_area.min + gfxm::vec2(GUI_MARGIN, guiGetCurrentFont()->font->getLineHeight() * .25f), GUI_COL_TEXT, GUI_COL_ACCENT);
        }
        float fontH = font->getLineHeight();
        if (hasList() && icon_arrow) {
            icon_arrow->draw(guiLayoutPlaceRectInsideRect(client_area, gfxm::vec2(fontH, fontH), GUI_RIGHT | GUI_VCENTER, gfxm::rect(GUI_MARGIN, GUI_MARGIN, GUI_MARGIN, GUI_MARGIN)), GUI_COL_TEXT);
        }
    }
};
class GuiMenuList : public GuiElement {
    std::vector<std::unique_ptr<GuiMenuListItem>> items;
    GuiMenuListItem* open_elem = 0;
public:
    void open() {
        setHidden(false);
    }
    void close() {
        setHidden(true);
        if (open_elem) {
            open_elem->close();
        }
    }

    GuiMenuList() {
        addFlags(
            GUI_FLAG_TOPMOST
            | GUI_FLAG_FLOATING
            | GUI_FLAG_MENU_POPUP
        );
        overflow = GUI_OVERFLOW_FIT;
    }
    GuiMenuList* addItem(GuiMenuListItem* item) {
        item->id = items.size();
        items.push_back(std::unique_ptr<GuiMenuListItem>(item));
        addChild(item);
        item->setOwner(this);
        return this;
    }
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::CLOSE_MENU:
            if (getOwner()) {
                guiPostMessage(getOwner(), GUI_MSG::CLOSE_MENU, GUI_MSG_PARAMS());
            }
            setHidden(true);
            return true;
        case GUI_MSG::NOTIFY:
            switch (params.getA<GUI_NOTIFY>()) {
            case GUI_NOTIFY::MENU_ITEM_HOVER: {
                int id = params.getB<int>();
                if (items[id]->hasList()) {
                    if (open_elem && open_elem->id != params.getA<int>()) {
                        open_elem->close();
                        open_elem = nullptr;
                    }
                    if (!open_elem && items[id]->hasList()) {
                        open_elem = items[id].get();
                        open_elem->open();
                    }
                }
                }return true;
            }
            break;
        }
        return false;
    }
    void onDraw() override {
        guiDrawRectShadow(rc_bounds);
        guiDrawRect(rc_bounds, GUI_COL_HEADER);
        guiDrawRectLine(rc_bounds, GUI_COL_BUTTON);
        GuiElement::onDraw();
        /*
        for (int i = 0; i < childCount(); ++i) {
            auto ch = getChild(i);
            ch->draw();
        }*/
    }
};
inline GuiMenuListItem::GuiMenuListItem(const char* cap, const std::initializer_list<GuiMenuListItem*>& child_items) {
    setSize(0, 0);
    caption.replaceAll(getFont(), cap, strlen(cap));
    menu_list.reset(new GuiMenuList);
    menu_list->setOwner(this);
    menu_list->setHidden(true);
    menu_list->addFlags(GUI_FLAG_MENU_SKIP_OWNER_CLICK);
    guiGetRoot()->addChild(menu_list.get());
    for (auto ch : child_items) {
        menu_list->addItem(ch);
    }
    icon_arrow = guiLoadIcon("svg/entypo/triangle-right.svg");
}
inline void GuiMenuListItem::open() {
    menu_list->open();
    menu_list->pos = gui_vec2(client_area.max.x, client_area.min.y);
    menu_list->size = gui_vec2(200, 200);
    is_open = true;
    guiBringWindowToTop(menu_list.get());
}
inline void GuiMenuListItem::close() {
    menu_list->close();
    is_open = false;
}
