#include "gui_root.hpp"
#include "gui/elements/gui_menu_bar.hpp"

GuiMenuBar* GuiRoot::createMenuBar() {
    if (menu_bar) {
        return menu_bar.get();
    } else {
        menu_bar.reset(new GuiMenuBar);
        menu_bar->addFlags(GUI_FLAG_FRAME | GUI_FLAG_PERSISTENT);
        addChild(menu_bar.get());
        return menu_bar.get();
    }
}
/*
void GuiRoot::onHitTest(GuiHitResult& hit, int x, int y) {
    // TODO: This is not called currently
    // NOTE: No, it is called
    if (!gfxm::point_in_rect(rc_bounds, gfxm::vec2(x, y))) {
        return;
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
        children_copy[0]->onHitTest(hit, x, y);
        return;
    }

    if(menu_bar){
        menu_bar->onHitTest(hit, x, y);
        if (hit.hasHit()) {
            return;
        }
    }

    GuiElement* last_hovered = 0;
    GUI_HIT hit_ = GUI_HIT::NOWHERE;
    for (int i = 0; i < children_copy.size(); ++i) {
        auto elem = children_copy[i];

        if (!elem->isEnabled()) {
            continue;
        }

        elem->onHitTest(hit, x, y);
        if (!hit.hasHit()) {
            continue;
        }
        last_hovered = hit.hits.back().elem;
        hit_ = hit.hits.back().hit;



        break;
    }

    return;
}*/

/*
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
        gfxm::vec2 size = gfxm::vec2(
            gfxm::_min(ch->max_size.x, gfxm::_max(ch->min_size.x, ch->size.x)),
            gfxm::_min(ch->max_size.y, gfxm::_max(ch->min_size.y, ch->size.y))
        );

        gfxm::rect new_rc = rc;
        new_rc = gfxm::rect(
            ch->pos,
            ch->pos + size
        );        

        ch->layout(new_rc, GUI_LAYOUT_DRAW_SHADOW);
    }
}*/
/*
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
}*/
