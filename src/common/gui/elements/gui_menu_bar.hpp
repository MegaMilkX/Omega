#pragma once

#include "gui/elements/gui_element.hpp"
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

    GuiHitResult hitTest(int x, int y) override {
        return GuiElement::hitTest(x, y);
    }
    void onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override;
    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) override {
        bounding_rect = rc;
        bounding_rect.min.x = cursor.x;

        caption.prepareDraw(guiGetCurrentFont(), false);
        bounding_rect.max.x = bounding_rect.min.x + caption.getBoundingSize().x + GUI_MARGIN * 2.f;
        
        client_area = bounding_rect;
    }
    void onDraw() {
        uint32_t color = GUI_COL_BG;
        if (isHovered() || is_open) {
            color = GUI_COL_BUTTON;
        }
        guiDrawRect(client_area, color);
        gfxm::vec2 text_pos = guiCalcTextPosInRect(gfxm::rect(gfxm::vec2(0, 0), caption.getBoundingSize()), client_area, 0, gfxm::rect(0, 0, 0, 0), guiGetCurrentFont()->font);
        caption.draw(text_pos, GUI_COL_TEXT, GUI_COL_TEXT);
    }
};

class GuiMenuBar : public GuiElement {
    std::vector<std::unique_ptr<GuiMenuItem>> items;
    bool is_active = false;
    GuiMenuItem* open_elem = 0;
public:
    GuiMenuBar* addItem(GuiMenuItem* item) {
        assert(item->getOwner() == nullptr && item->getParent() == nullptr);
        item->id = items.size();
        items.push_back(std::unique_ptr<GuiMenuItem>(item));
        addChild(item);
        item->setOwner(this);
        return this;
    }
    GuiHitResult hitTest(int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }
        for (int i = 0; i < childCount(); ++i) {
            auto c = getChild(i);
            GuiHitResult hit = c->hitTest(x, y);
            if (hit.hasHit()) {
                return hit;
            }
        }
        return GuiHitResult{ GUI_HIT::CLIENT, this };
    }
    void onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
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
                } break;
            case GUI_NOTIFY::MENU_ITEM_HOVER: {
                if(is_active) {
                    int id = params.getB<int>();
                    if (open_elem && open_elem->id != params.getA<int>()) {
                        open_elem->close();
                        open_elem = nullptr;
                    }
                    if (!open_elem && items[id]->hasList()) {
                        open_elem = items[id].get();
                        open_elem->open();
                    }
                }
                }break;
            case GUI_NOTIFY::MENU_COMMAND:
                is_active = false;
                open_elem = nullptr;
                forwardMessageToOwner(msg, params);
                break;
            }
            break;
        }
    }
    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) override {
        bounding_rect = rc;
        bounding_rect.max.y = bounding_rect.min.y + guiGetCurrentFont()->font->getLineHeight() * 1.5f;
        client_area = bounding_rect;
        gfxm::vec2 cur = client_area.min + gfxm::vec2(guiGetCurrentFont()->font->getLineHeight(), .0f);
        float max_h = guiGetCurrentFont()->font->getLineHeight() * 1.5f;
        for (int i = 0; i < childCount(); ++i) {
            auto c = getChild(i);
            c->layout(cur, client_area, flags);
            cur.x += (c->getBoundingRect().max.x - c->getBoundingRect().min.x);
            if (max_h < (c->getBoundingRect().max.y - c->getBoundingRect().min.y)) {
                max_h = (c->getBoundingRect().max.y - c->getBoundingRect().min.y);
            }
        }
        bounding_rect.max.y = bounding_rect.min.y + max_h;
        client_area = bounding_rect;
    }
    void onDraw() {
        guiDrawRect(client_area, GUI_COL_BG);
        for (int i = 0; i < childCount(); ++i) {
            auto c = getChild(i);
            c->draw();
        }
    }
};
