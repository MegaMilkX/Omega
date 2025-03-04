#pragma once

#include "gui/elements/element.hpp"
#include "gui/gui_font.hpp"
#include "gui/gui_text_buffer.hpp"


class GuiMenuListItem;
class GuiMenuList;
class GuiMenuItem : public GuiElement {
    GuiTextBuffer caption;
    bool is_open = false;
    std::unique_ptr<GuiMenuList> menu_list;
public:
    int id = 0;

    GuiMenuItem(const char* caption);
    GuiMenuItem(const char* caption, const std::initializer_list<GuiMenuListItem*>& child_items);

    void open();
    void close();
    
    bool hasList() { return menu_list.get() != nullptr; }

    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override;
    void onLayout(const gfxm::vec2& extents, uint64_t flags) override {
        rc_bounds = gfxm::rect(gfxm::vec2(0, 0), extents);

        caption.prepareDraw(getFont(), false);
        rc_bounds.max.x = rc_bounds.min.x + caption.getBoundingSize().x + GUI_MARGIN * 2.f;
        
        client_area = rc_bounds;
    }
    void onDraw() {
        uint32_t color = GUI_COL_BG;
        if (isHovered() || is_open) {
            color = GUI_COL_BUTTON;
        }
        guiDrawRect(client_area, color);
        caption.draw(getFont(), client_area, GUI_VCENTER | GUI_HCENTER, GUI_COL_TEXT, GUI_COL_TEXT);
    }
};

class GuiMenuBar : public GuiElement {
    std::vector<std::unique_ptr<GuiMenuItem>> items;
    bool is_active = false;
    GuiMenuItem* open_elem = 0;
public:
    GuiMenuBar() {
        setSize(gui::perc(100), gui::em(2));
    }
    GuiMenuBar* addItem(GuiMenuItem* item) {
        assert(item->getOwner() == nullptr && item->getParent() == nullptr);
        item->id = items.size();
        std::unique_ptr<GuiMenuItem> uptritem(item);
        items.push_back(std::move(uptritem));
        addChild(item);
        item->setOwner(this);
        return this;
    }
    void onHitTest(GuiHitResult& hit, int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return;
        }
        for (int i = 0; i < childCount(); ++i) {
            auto c = getChild(i);
            c->hitTest(hit, x, y);
            if (hit.hasHit()) {
                return;
            }
        }
        hit.add(GUI_HIT::CLIENT, this);
        return;
    }
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::CLOSE_MENU:
            is_active = false;
            open_elem = 0;
            return true;
        case GUI_MSG::NOTIFY:
            switch (params.getA<GUI_NOTIFY>()) {
            case GUI_NOTIFY::MENU_ITEM_CLICKED: {
                int id = params.getB<int>();
                if(items[id]->hasList()) {
                    if (!is_active) {
                        is_active = true; 
                        open_elem = items[id].get();
                        open_elem->open();
                    } else {
                        is_active = false;
                        if (open_elem) {
                            open_elem->close();
                            open_elem = 0;
                        }
                    }
                }
                } return true;
            case GUI_NOTIFY::MENU_ITEM_HOVER: {
                if(is_active) {
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
                }
                }return true;
            case GUI_NOTIFY::MENU_COMMAND:
                is_active = false;
                open_elem = nullptr;
                forwardMessageToOwner(msg, params);
                return true;
            }
            break;
        }
        return false;
    }
    void onLayout(const gfxm::vec2& extents, uint64_t flags) override {
        Font* font = getFont();
        rc_bounds = gfxm::rect(gfxm::vec2(0, 0), extents);
        rc_bounds.max.y = rc_bounds.min.y + font->getLineHeight() * 2.0f;
        client_area = rc_bounds;
        gfxm::vec2 cur = client_area.min + gfxm::vec2(font->getLineHeight(), .0f);
        float max_h = font->getLineHeight() * 1.5f;
        for (int i = 0; i < childCount(); ++i) {
            auto c = getChild(i);
            gfxm::rect rc = client_area;
            rc.min = cur;
            c->layout_position = rc.min;
            c->layout(gfxm::rect_size(rc), flags);
            cur.x += (c->getBoundingRect().max.x - c->getBoundingRect().min.x);
            if (max_h < (c->getBoundingRect().max.y - c->getBoundingRect().min.y)) {
                max_h = (c->getBoundingRect().max.y - c->getBoundingRect().min.y);
            }
        }
        rc_bounds.max.y = rc_bounds.min.y + max_h;
        client_area = rc_bounds;
    }
    void onDraw() {
        guiDrawRect(client_area, GUI_COL_BG);
        for (int i = 0; i < childCount(); ++i) {
            auto c = getChild(i);
            c->draw();
        }
    }
};
