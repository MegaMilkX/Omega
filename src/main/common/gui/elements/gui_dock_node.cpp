#include "common/gui/elements/gui_dock_node.hpp"

#include "common/gui/elements/gui_dock_space.hpp"


DockNode* DockNode::splitLeft() {
    return getDockSpace()->splitLeft(this, GUI_DOCK_SPLIT::VERTICAL);
}
DockNode* DockNode::splitRight() {
    return getDockSpace()->splitRight(this, GUI_DOCK_SPLIT::VERTICAL);
}
DockNode* DockNode::splitTop() {
    return getDockSpace()->splitLeft(this, GUI_DOCK_SPLIT::HORIZONTAL);
}
DockNode* DockNode::splitBottom() {
    return getDockSpace()->splitRight(this, GUI_DOCK_SPLIT::HORIZONTAL);
}

void DockNode::onMessage(GUI_MSG msg, uint64_t a_param, uint64_t b_param) {
    switch (msg) {
    case GUI_MSG::RESIZING: {
        gfxm::rect* prc = (gfxm::rect*)b_param;
        switch ((GUI_HIT)a_param) {
        case GUI_HIT::RIGHT:
            split_pos += (prc->max.x - prc->min.x) / (client_area.max.x - client_area.min.x);
            break;
        case GUI_HIT::BOTTOM:
            split_pos += (prc->max.y - prc->min.y) / (client_area.max.y - client_area.min.y);
            break;
        }
    } break;
    case GUI_MSG::NOTIFY: {
        GUI_NOTIFICATION n = (GUI_NOTIFICATION)a_param;
        switch (n) {
        case GUI_NOTIFICATION::DRAG_TAB_START:
            guiPostMessage(GUI_MSG::DOCK_TAB_DRAG_START, (uint64_t)children[b_param], (uint64_t)this);
            break;
        case GUI_NOTIFICATION::TAB_CLICKED:
            front_window = (GuiWindow*)children[b_param];
            break;
        }
    } break;
    case GUI_MSG::DOCK_TAB_DRAG_DROP_PAYLOAD: {
        GuiWindow* w = (GuiWindow*)a_param;
        addWindow(w);
        } break;
    case GUI_MSG::DOCK_TAB_DRAG_DROP_PAYLOAD_SPLIT: {
        GuiWindow* w = (GuiWindow*)a_param;
        GUI_DOCK_SPLIT_DROP split_type = (GUI_DOCK_SPLIT_DROP)b_param;
        switch (split_type) {
        case GUI_DOCK_SPLIT_DROP::MID:
            addWindow(w);
            break;
        case GUI_DOCK_SPLIT_DROP::LEFT:
            splitLeft()->left->addWindow(w);
            break;
        case GUI_DOCK_SPLIT_DROP::RIGHT:
            splitRight()->right->addWindow(w);
            break;
        case GUI_DOCK_SPLIT_DROP::TOP:
            splitTop()->left->addWindow(w);
            break;
        case GUI_DOCK_SPLIT_DROP::BOTTOM:
            splitBottom()->right->addWindow(w);
            break;
        }
    } break;
    case GUI_MSG::DOCK_TAB_DRAG_SUCCESS: {
        GuiWindow* w = (GuiWindow*)a_param;
        removeWindow(w);
        if (parent_node && isEmpty()) {
            getDockSpace()->collapse(parent_node);
        }
        } break;
    case GUI_MSG::DOCK_TAB_DRAG_FAIL:
        break;
    }
}