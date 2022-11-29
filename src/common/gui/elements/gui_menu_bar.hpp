#pragma once

#include "gui/elements/gui_element.hpp"
#include "gui/gui_font.hpp"
#include "gui/gui_text_buffer.hpp"


class GuiMenuList;
class GuiMenuItem : public GuiElement {
    GuiTextBuffer caption;
    bool is_open = false;
    std::unique_ptr<GuiMenuList> menu_list;
public:
    GuiMenuItem(const char* cap);
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
    std::vector<std::unique_ptr<GuiElement>> items;
public:
    GuiMenuBar* addItem(GuiElement* elem) {
        items.push_back(std::unique_ptr<GuiElement>(elem));
        addChild(elem);
        elem->setOwner(this);
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
    void onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {}
    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) override {
        bounding_rect = rc;
        bounding_rect.max.y = bounding_rect.min.y + guiGetCurrentFont()->font->getLineHeight() * 2.f;
        client_area = bounding_rect;
        gfxm::vec2 cur = client_area.min + gfxm::vec2(guiGetCurrentFont()->font->getLineHeight(), .0f);
        float max_h = guiGetCurrentFont()->font->getLineHeight() * 2.f;
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
