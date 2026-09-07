#include "tree_view.hpp"


void GuiTreeView::scrollToImpl(GuiTreeItem* item) {
    auto rc_cont = rc_bounds;
    auto rc_item = item->getGlobalBoundingRect();
    rc_item.min = guiConvertToLocal(this, rc_item.min);
    rc_item.max = guiConvertToLocal(this, rc_item.max);
    rc_item.min += -pos_content;
    rc_item.max += -pos_content;

    gfxm::vec2 delta(0, 0);

    if (rc_item.min.y < rc_bounds.min.y) {
        delta.y = rc_bounds.min.y - rc_item.min.y;
    } else if (rc_item.max.y > rc_bounds.max.y) {
        delta.y = rc_bounds.max.y - rc_item.max.y;
    }

    if (rc_item.min.x < rc_bounds.min.x) {
        delta.x = rc_bounds.min.x - rc_item.min.x;
    } else if (rc_item.max.x > rc_bounds.max.x) {
        delta.x = rc_bounds.max.x - rc_item.max.x;
    }

    if (delta.x == 0 && delta.y == 0) {
        return;
    }

    pos_content -= delta;
    target_pos_content = pos_content;
}

GuiTreeView::GuiTreeView() {
    setSize(gui::fill(), 250);
    //setMaxSize(0, 0);
    setMinSize(0, 100);
    addFlags(GUI_FLAG_RESIZE_Y);

    setStyleClasses({ "tree-view" });

    subscribe<GuiEvt_Selected>([this](const GuiEvt_Selected& e) {
        if (e.elem == selected_item) {
            return;
        }

        if (selected_item) {
            selected_item->removeFlags(GUI_FLAG_SELECTED);
            selected_item->invoke(GuiEvt_Deselected{ selected_item });
            selected_item = nullptr;
        }
        selected_item = dynamic_cast<GuiTreeItem*>(e.elem);
        //LOG_DBG("GuiTreeView: selected");
    });
}

GuiTreeItem* GuiTreeView::addItem(const char* name) {
    auto item = new GuiTreeItem(name);
    addChild(item);
    return item;
}

void GuiTreeView::scrollTo(GuiTreeItem* item) {
    scroll_target = item;
    guiScheduleTick(this, 0, GUI_TICK_CUSTOM);
}

void GuiTreeView::onTick(float dt, GUI_TICK_ID id) {
    if (id == GUI_TICK_CUSTOM) {
        if(!scroll_target) return;
        scrollToImpl(scroll_target);
        scroll_target = nullptr;
        return;
    }
    GuiElement::onTick(dt, id);
}