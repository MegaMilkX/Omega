#pragma once

#include "element.hpp"

class GuiContainer : public GuiElement {
public:
    void onHitTest(GuiHitResult& hit, int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return;
        }
        for (auto& ch : children) {
            ch->hitTest(hit, x, y);
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

        float y = client_area.min.y;
        for (auto& ch : children) {
            float width = client_area.max.x - client_area.min.x;
            float height = ch->size.y.value;
            if (height == .0f) {
                height = client_area.max.y - client_area.min.y;
            }
            gfxm::vec2 pos = gfxm::vec2(client_area.min.x, y);
            ch->size.x = client_area.max.x - client_area.min.x;
            //gfxm::rect rect(pos, pos + gfxm::vec2(width, height));
            y += ch->size.y.value + GUI_PADDING;

            ch->layout_position = pos;
            ch->layout(gfxm::vec2(width, height), 0);
        }
    }

    virtual void onDraw() {
        guiDrawPushScissorRect(client_area);
        for (auto& ch : children) {
            ch->draw();
        }
        guiDrawPopScissorRect();
    }
};