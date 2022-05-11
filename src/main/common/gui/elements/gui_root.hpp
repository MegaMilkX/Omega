#pragma once

#include "common/gui/elements/gui_element.hpp"


class GuiRoot : public GuiElement {
public:
    GuiHitResult hitTest(int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }

        for (int i = 0; i < children.size(); ++i) {
            auto h = children[i]->hitTest(x, y);
            if (h.hit != GUI_HIT::NOWHERE) {
                return h;
            }
        }

        return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
    }

    void onMessage(GUI_MSG msg, uint64_t a_param, uint64_t b_param) override {
        switch (msg) {
        case GUI_MSG::PAINT: {
        } break;
        }
    }

    void onLayout(const gfxm::rect& rect, uint64_t flags) override {
        this->bounding_rect = rect;
        this->client_area = bounding_rect;
        
        gfxm::rect rc = client_area;
        for (auto& ch : children) {
            GUI_DOCK dock_pos = ch->getDockPosition();
            gfxm::rect new_rc = rc;
            if (dock_pos == GUI_DOCK::NONE) {
                new_rc = gfxm::rect(
                    rc.min + ch->pos,
                    rc.min + ch->pos + ch->size
                );
            } else if (dock_pos == GUI_DOCK::LEFT) {
                new_rc.max.x = rc.min.x + ch->size.x;
                rc.min.x = new_rc.max.x;
            } else if (dock_pos == GUI_DOCK::RIGHT) {
                new_rc.min.x = rc.max.x - ch->size.x;
                rc.max.x = new_rc.min.x;
            } else if (dock_pos == GUI_DOCK::TOP) {
                new_rc.max.y = rc.min.y + ch->size.y;
                rc.min.y = new_rc.max.y;
            } else if (dock_pos == GUI_DOCK::BOTTOM) {
                new_rc.min.y = rc.max.y - ch->size.y;
                rc.max.y = new_rc.min.y;
            } else if (dock_pos == GUI_DOCK::FILL) {

            }
            ch->onLayout(new_rc, 0);
        }
    }

    void onDraw() override {
        int sw = 0, sh = 0;
        platformGetWindowSize(sw, sh);
        for (auto& ch : children) {
            glScissor(
                client_area.min.x,
                sh - client_area.max.y,
                client_area.max.x - client_area.min.x,
                client_area.max.y - client_area.min.y
            );
            ch->onDraw();
        }

        //guiDrawRectLine(bounding_rect);
    }
};