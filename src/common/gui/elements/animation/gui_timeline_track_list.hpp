#pragma once

#include "gui/elements/gui_element.hpp"


class GuiTimelineTrackListItem : public GuiElement {
    GuiTextBuffer caption;
public:
    GuiTimelineTrackListItem()
    : caption(guiGetDefaultFont()) {}
    void setCaption(const char* cap) {
        caption.replaceAll(cap, strlen(cap));
    }
    GuiHitResult onHitTest(int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }
        return GuiHitResult{ GUI_HIT::CLIENT, this };
    }
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        return false;
    }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        rc_bounds = rc;
        client_area = rc_bounds;
        caption.prepareDraw(guiGetCurrentFont(), false);
    }
    void onDraw() override {
        guiDrawRectRound(
            client_area, 15, GUI_COL_BUTTON,
            GUI_DRAW_CORNER_NW | GUI_DRAW_CORNER_SW
        );
        caption.draw(
            gfxm::vec2(client_area.min.x + GUI_MARGIN, caption.findCenterOffsetY(client_area)),
            GUI_COL_TEXT, GUI_COL_TEXT
        );
    }
};
class GuiTimelineTrackList : public GuiElement {
    std::vector<std::unique_ptr<GuiTimelineTrackListItem>> items;
public:
    GuiTimelineTrackList() {}
    GuiTimelineTrackListItem* addItem(const char* caption) {
        auto ptr = new GuiTimelineTrackListItem();
        ptr->setCaption(caption);
        items.push_back(std::unique_ptr<GuiTimelineTrackListItem>(ptr));
        addChild(ptr);
        return ptr;
    }
    GuiHitResult onHitTest(int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }
        for (auto& i : items) {
            GuiHitResult hit = i->onHitTest(x, y);
            if (hit.hasHit()) {
                return hit;
            }
        }
        return GuiHitResult{ GUI_HIT::CLIENT, this };
    }
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        return false;
    }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        rc_bounds = rc;
        client_area = rc_bounds;
        float track_height = 30.0f;
        float track_margin = 1.0f;
        for (int i = 0; i < items.size(); ++i) {
            float y_offs = i * (track_height + track_margin);
            gfxm::rect rc(
                client_area.min + gfxm::vec2(.0f, y_offs),
                gfxm::vec2(client_area.max.x, client_area.min.y + y_offs + track_height)
            );
            items[i]->layout(rc, flags);
        }
    }
    void onDraw() override {
        for (auto& i : items) {
            i->draw();
        }
    }
};

