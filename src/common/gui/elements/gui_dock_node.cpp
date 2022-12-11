#include "gui/elements/gui_dock_node.hpp"

#include "gui/elements/gui_dock_space.hpp"


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

void DockNode::onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) {
    switch (msg) {
    case GUI_MSG::RESIZING: {
        gfxm::rect* prc = params.getB<gfxm::rect*>();
        switch (params.getA<GUI_HIT>()) {
        case GUI_HIT::RIGHT:
            split_pos += (prc->max.x - prc->min.x) / (client_area.max.x - client_area.min.x);
            break;
        case GUI_HIT::BOTTOM:
            split_pos += (prc->max.y - prc->min.y) / (client_area.max.y - client_area.min.y);
            break;
        }
    } break;
    case GUI_MSG::NOTIFY: {
        GUI_NOTIFY n = params.getA<GUI_NOTIFY>();
        switch (n) {
        case GUI_NOTIFY::DRAG_TAB_START:
            guiPostMessage<GuiElement*, DockNode*>(GUI_MSG::DOCK_TAB_DRAG_START, children[params.getB<int>()], this);
            break;
        case GUI_NOTIFY::DRAG_TAB_END:
            // NOTE: Nothing to do here?
            break;
        case GUI_NOTIFY::TAB_CLICKED:
            front_window = (GuiWindow*)children[params.getB<int>()];
            guiSetActiveWindow(front_window);
            break;
        }
    } break;
    case GUI_MSG::DOCK_TAB_DRAG_DROP_PAYLOAD: {
        GuiWindow* w = params.getA<GuiWindow*>();
        addWindow(w);
        } break;
    case GUI_MSG::DOCK_TAB_DRAG_DROP_PAYLOAD_SPLIT: {
        GuiWindow* w = params.getA<GuiWindow*>();
        GUI_DOCK_SPLIT_DROP split_type = params.getB<GUI_DOCK_SPLIT_DROP>();
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
        GuiWindow* w = params.getA<GuiWindow*>();
        //removeWindow(w);
        if (parent_node && isEmpty()) {
            getDockSpace()->collapse(parent_node);
        }
        } break;
    case GUI_MSG::DOCK_TAB_DRAG_FAIL:
        break;
    }
}