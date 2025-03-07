#pragma once

#include "gui/elements/element.hpp"


class GuiTimelineTrackListItem : public GuiElement {
    GuiTextBuffer caption;
public:
    GuiTimelineTrackListItem() {}
    void setCaption(const char* cap) {
        caption.replaceAll(getFont(), cap, strlen(cap));
    }
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        return false;
    }
    void onLayout(const gfxm::vec2& extents, uint64_t flags) override {
        rc_bounds = gfxm::rect(gfxm::vec2(0, 0), extents);
        client_area = rc_bounds;
        caption.prepareDraw(getFont(), false);
    }
    void onDraw() override {
        guiDrawRectRound(
            client_area, 15, GUI_COL_BUTTON,
            GUI_DRAW_CORNER_NW | GUI_DRAW_CORNER_SW
        );
        caption.draw(
            getFont(),
            gfxm::vec2(client_area.min.x + GUI_MARGIN, caption.findCenterOffsetY(client_area)),
            GUI_COL_TEXT, GUI_COL_TEXT
        );
    }
};
class GuiTimelineTrackList : public GuiElement {
    std::vector<std::unique_ptr<GuiTimelineTrackListItem>> items;
public:
    float track_height = 20.0f;
    float track_margin = 1.0f;

    GuiTimelineTrackList() {}
    GuiTimelineTrackListItem* addItem(const char* caption) {
        auto ptr = new GuiTimelineTrackListItem();
        ptr->setCaption(caption);
        items.push_back(std::unique_ptr<GuiTimelineTrackListItem>(ptr));
        addChild(ptr);
        return ptr;
    }
    void onHitTest(GuiHitResult& hit, int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return;
        }
        for (auto& i : items) {
            i->hitTest(hit, x, y);
            if (hit.hasHit()) {
                return;
            }
        }
        hit.add(GUI_HIT::CLIENT, this);
        return;
    }
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        return false;
    }
    void onLayout(const gfxm::vec2& extents, uint64_t flags) override {
        rc_bounds = gfxm::rect(gfxm::vec2(0, 0), extents);
        client_area = rc_bounds;
        for (int i = 0; i < items.size(); ++i) {
            float y_offs = i * (track_height + track_margin);
            gfxm::rect rc(
                client_area.min + gfxm::vec2(.0f, y_offs),
                gfxm::vec2(client_area.max.x, client_area.min.y + y_offs + track_height)
            );
            items[i]->layout_position = rc.min;
            items[i]->layout(gfxm::rect_size(rc), flags);
        }
    }
    void onDraw() override {
        for (auto& i : items) {
            i->draw();
        }
    }
};

