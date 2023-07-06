#include "gui_root.hpp"
#include "gui/elements/gui_menu_bar.hpp"

GuiMenuBar* GuiRoot::createMenuBar() {
    if (menu_bar) {
        return menu_bar.get();
    } else {
        menu_bar.reset(new GuiMenuBar);
        return menu_bar.get();
    }
}

GuiHitResult GuiRoot::onHitTest(int x, int y) {
    // TODO: This is not called currently
    // NOTE: No, it is called
    if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
        return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
    }

    std::vector<GuiElement*> children_copy = children;
    std::sort(children_copy.begin(), children_copy.end(), [](const GuiElement* a, const GuiElement* b)->bool {
        if ((a->getFlags() & GUI_FLAG_TOPMOST) == (b->getFlags() & GUI_FLAG_TOPMOST)) {
            return a->getZOrder() > b->getZOrder();
        } else if ((a->getFlags() & GUI_FLAG_BLOCKING) == (b->getFlags() & GUI_FLAG_BLOCKING)) {
            return (a->getFlags() & GUI_FLAG_TOPMOST) > (b->getFlags() & GUI_FLAG_TOPMOST);
        } else {
            return (a->getFlags() & GUI_FLAG_BLOCKING) > (b->getFlags() & GUI_FLAG_BLOCKING);
        }
    });

    if (!children_copy.empty() && (children_copy[0]->getFlags() & GUI_FLAG_BLOCKING)) {
        GuiHitResult hit = children_copy[0]->onHitTest(x, y);
        return hit;
    }

    if(menu_bar){
        GuiHitResult hit = menu_bar->onHitTest(x, y);
        if (hit.hasHit()) {
            return hit;
        }
    }

    GuiElement* last_hovered = 0;
    GUI_HIT hit = GUI_HIT::NOWHERE;
    for (int i = 0; i < children_copy.size(); ++i) {
        auto elem = children_copy[i];

        if (!elem->isEnabled()) {
            continue;
        }

        GuiHitResult hr = elem->onHitTest(x, y);
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

bool GuiRoot::onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) {
    return false;
}

void GuiRoot::onLayout(const gfxm::rect& rect, uint64_t flags) {
    this->rc_bounds = rect;
    this->client_area = rc_bounds;

    std::vector<GuiElement*> children_copy = children;
    std::sort(children_copy.begin(), children_copy.end(), [](const GuiElement* a, const GuiElement* b)->bool {
        if ((a->getFlags() & GUI_FLAG_TOPMOST) == (b->getFlags() & GUI_FLAG_TOPMOST)) {
            return a->getZOrder() < b->getZOrder();
        } else if ((a->getFlags() & GUI_FLAG_BLOCKING) == (b->getFlags() & GUI_FLAG_BLOCKING)) {
            return (a->getFlags() & GUI_FLAG_TOPMOST) < (b->getFlags() & GUI_FLAG_TOPMOST);
        } else {
            return (a->getFlags() & GUI_FLAG_BLOCKING) < (b->getFlags() & GUI_FLAG_BLOCKING);
        }
    });
        
    gfxm::rect rc = client_area;
    if (menu_bar.get()) {
        menu_bar->layout(rc, flags);
        rc.min.y = menu_bar->getClientArea().max.y + GUI_MARGIN;
    }
    for (int i = 0; i < children_copy.size(); ++i) {
        auto ch = children_copy[i];

        GUI_DOCK dock_pos = ch->getDockPosition();
        gfxm::rect new_rc = rc;
        if (dock_pos == GUI_DOCK::NONE) {
            new_rc = gfxm::rect(
                ch->pos,
                ch->pos + ch->size
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
            new_rc = rc;
        }

        ch->layout(new_rc, GUI_LAYOUT_DRAW_SHADOW);
    }
}

void GuiRoot::onDraw() {
    std::vector<GuiElement*> children_copy = children;
    std::sort(children_copy.begin(), children_copy.end(), [](const GuiElement* a, const GuiElement* b)->bool {
        if ((a->getFlags() & GUI_FLAG_TOPMOST) == (b->getFlags() & GUI_FLAG_TOPMOST)) {
            return a->getZOrder() < b->getZOrder();
        } else if ((a->getFlags() & GUI_FLAG_BLOCKING) == (b->getFlags() & GUI_FLAG_BLOCKING)) {
            return (a->getFlags() & GUI_FLAG_TOPMOST) < (b->getFlags() & GUI_FLAG_TOPMOST);
        } else {
            return (a->getFlags() & GUI_FLAG_BLOCKING) < (b->getFlags() & GUI_FLAG_BLOCKING);
        }
    });
    guiDrawPushScissorRect(client_area);
    for (int i = 0; i < children_copy.size(); ++i) {
        auto ch = children_copy[i];
        if (ch->getFlags() & GUI_FLAG_BLOCKING) {
            guiDrawRect(client_area, 0xAA000000);
        }
        ch->draw();
    }
    if (menu_bar.get()) {
        menu_bar->draw();
    }

    //guiDrawRectLine(rc_bounds);
    guiDrawPopScissorRect();
}
