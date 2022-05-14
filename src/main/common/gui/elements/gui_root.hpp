#pragma once

#include "common/gui/elements/gui_element.hpp"


class GuiRoot : public GuiElement {
public:
    GuiHitResult hitTest(int x, int y) override {
        // TODO: This is not called currently
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }

        std::vector<GuiElement*> children_copy = children;
        std::sort(children_copy.begin(), children_copy.end(), [](const GuiElement* a, const GuiElement* b)->bool {
            if ((a->getFlags() & GUI_FLAG_TOPMOST) == (b->getFlags() & GUI_FLAG_TOPMOST)) {
                return a->getZOrder() > b->getZOrder();
            } else {
                return (a->getFlags() & GUI_FLAG_TOPMOST) > (b->getFlags() & GUI_FLAG_TOPMOST);
            }
        });

        GuiElement* last_hovered = 0;
        GUI_HIT hit = GUI_HIT::NOWHERE;
        for (int i = 0; i < children_copy.size(); ++i) {
            auto elem = children_copy[i];

            if (!elem->isEnabled()) {
                continue;
            }

            GuiHitResult hr = elem->hitTest(x, y);
            last_hovered = hr.elem;
            hit = hr.hit;
            if (hr.hit == GUI_HIT::NOWHERE) {
                continue;
            }


            /*
            if (hr.hit == GUI_HIT::CLIENT) {
                for (int i = 0; i < elem->childCount(); ++i) {
                    stack.push(elem->getChild(i));
                }
            }*/

            if (hit != GUI_HIT::NOWHERE) {
                break;
            }
        }

        return GuiHitResult{ hit, last_hovered };
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

        std::vector<GuiElement*> children_copy = children;
        std::sort(children_copy.begin(), children_copy.end(), [](const GuiElement* a, const GuiElement* b)->bool {
            if ((a->getFlags() & GUI_FLAG_TOPMOST) == (b->getFlags() & GUI_FLAG_TOPMOST)) {
                return a->getZOrder() < b->getZOrder();
            } else {
                return (a->getFlags() & GUI_FLAG_TOPMOST) < (b->getFlags() & GUI_FLAG_TOPMOST);
            }
        });
        
        gfxm::rect rc = client_area;
        for (int i = 0; i < children_copy.size(); ++i) {
            auto ch = children_copy[i];

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

        std::vector<GuiElement*> children_copy = children;
        std::sort(children_copy.begin(), children_copy.end(), [](const GuiElement* a, const GuiElement* b)->bool {
            if ((a->getFlags() & GUI_FLAG_TOPMOST) == (b->getFlags() & GUI_FLAG_TOPMOST)) {
                return a->getZOrder() < b->getZOrder();
            } else {
                return (a->getFlags() & GUI_FLAG_TOPMOST) < (b->getFlags() & GUI_FLAG_TOPMOST);
            }
        });
        for (int i = 0; i < children_copy.size(); ++i) {
            auto ch = children_copy[i];
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