#include "gui/elements/dock_node.hpp"

#include "gui/elements/dock_space.hpp"

DockNode::DockNode(GuiDockSpace* dock_space, DockNode* parent_node)
    : dock_space(dock_space), parent_node(parent_node) {

    tab_control.reset(new GuiTabControl());
    tab_control->setOwner(this);
    tab_control->setParent(this);
    dock_drag_target.reset(new GuiDockDragDropSplitter(dock_space->getDockGroup()));
    dock_drag_target->setOwner(this);
    dock_drag_target->setFlags(dock_drag_target->getFlags() | GUI_FLAG_TOPMOST | GUI_FLAG_FLOATING);
    dock_drag_target->setEnabled(true);
}

void DockNode::splitX(const char* node_name_left, const char* node_name_right, float ratio) {
    auto node = getDockSpace()->splitLeft(this, GUI_DOCK_SPLIT::VERTICAL, 0);
    node->split_pos = ratio;
    node->left->setId(node_name_left);
    node->right->setId(node_name_right);
}
void DockNode::splitY(const char* node_name_left, const char* node_name_right, float ratio) {
    auto node = getDockSpace()->splitLeft(this, GUI_DOCK_SPLIT::HORIZONTAL, 0);
    node->split_pos = ratio;
    node->left->setId(node_name_left);
    node->right->setId(node_name_right);
}
DockNode* DockNode::splitLeft(int size) {
    return getDockSpace()->splitLeft(this, GUI_DOCK_SPLIT::VERTICAL, size);
}
DockNode* DockNode::splitRight(int size) {
    return getDockSpace()->splitRight(this, GUI_DOCK_SPLIT::VERTICAL, size);
}
DockNode* DockNode::splitTop(int size) {
    return getDockSpace()->splitLeft(this, GUI_DOCK_SPLIT::HORIZONTAL, size);
}
DockNode* DockNode::splitBottom(int size) {
    return getDockSpace()->splitRight(this, GUI_DOCK_SPLIT::HORIZONTAL, size);
}

bool DockNode::onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) {
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
    } return true;
    case GUI_MSG::TITLE_CHANGED: {
        GuiWindow* e = params.getA<GuiWindow*>();
        for (int i = 0; i < tab_control->getTabCount(); ++i) {
            auto btn = tab_control->getTabButton(i);
            if (btn->getUserPtr() == e) {
                btn->setCaption(e->getTitle().c_str());
            }
        }
        return true;
    }
    case GUI_MSG::NOTIFY: {
        GUI_NOTIFY n = params.getA<GUI_NOTIFY>();
        switch (n) {
        case GUI_NOTIFY::TAB_CLICKED: {
            GuiTabButton* btn = params.getB<GuiTabButton*>();
            front_window = (GuiWindow*)btn->getUserPtr();
            guiSetActiveWindow(front_window);
            return true;
        }
        case GUI_NOTIFY::TAB_SWAP:
            std::iter_swap(children.begin() + params.getB<int>(), children.begin() + params.getC<int>());
            return true;
        case GUI_NOTIFY::TAB_CLOSED: {
            GuiTabButton* btn = params.getB<GuiTabButton*>();
            GuiWindow* wnd = (GuiWindow*)btn->getUserPtr();
            removeChild(wnd);
            guiDestroyWindow(wnd);
            if (parent_node && isEmpty() && !locked) {
                getDockSpace()->collapseBranch(parent_node);
            }
            return true;
        }
        case GUI_NOTIFY::TAB_DRAGGED_OUT: {
            GuiTabButton* btn = params.getB<GuiTabButton*>();
            GuiWindow* wnd = (GuiWindow*)btn->getUserPtr();
            // NOTE: Switching to another front_window is handled in overloaded removeChild()
            removeChild(wnd);
            guiGetRoot()->addChild(wnd);
            guiForceElementMoveState(wnd, 55, 15);
            // NOTE: this is already done in the overloaded removeChild()
            // TODO: seems bad, should change
            //tab_control->removeTab(id);

            if (parent_node && isEmpty() && !locked) {
                getDockSpace()->collapseBranch(parent_node);
                // NOTE: DO NOT DO ANYTHING AFTER THIS CALL
            }
            return true;
        }
        }
    } break;
    case GUI_MSG::DOCK_TAB_DRAG_DROP_PAYLOAD: {
        GuiWindow* w = params.getA<GuiWindow*>();
        addWindow(w);
        } return true;
    case GUI_MSG::DOCK_TAB_DRAG_DROP_PAYLOAD_SPLIT: {
        GuiWindow* w = params.getA<GuiWindow*>();
        GUI_DOCK_SPLIT_DROP split_type = params.getB<GUI_DOCK_SPLIT_DROP>();
        int node_size = params.getC<int>();
        switch (split_type) {
        case GUI_DOCK_SPLIT_DROP::MID:
            addWindow(w);
            break;
        case GUI_DOCK_SPLIT_DROP::LEFT:
            splitLeft(node_size)->left->addWindow(w);
            break;
        case GUI_DOCK_SPLIT_DROP::RIGHT:
            splitRight(node_size)->right->addWindow(w);
            break;
        case GUI_DOCK_SPLIT_DROP::TOP:
            splitTop(node_size)->left->addWindow(w);
            break;
        case GUI_DOCK_SPLIT_DROP::BOTTOM:
            splitBottom(node_size)->right->addWindow(w);
            break;
        }
        } return true;
    case GUI_MSG::DOCK_TAB_DRAG_SUCCESS: {
        GuiWindow* w = params.getA<GuiWindow*>();
        //removeWindow(w);
        if (parent_node && isEmpty()) {
            getDockSpace()->collapseBranch(parent_node);
            // NOTE: DO NOT DO ANYTHING AFTER THIS CALL
        }
        } return true;
    case GUI_MSG::DOCK_TAB_DRAG_FAIL:
        return true;
    }
    return false;
}