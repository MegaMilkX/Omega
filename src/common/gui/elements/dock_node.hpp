#pragma once

#include "gui/elements/tab_control.hpp"
#include "gui/elements/dock_drag_target.hpp"
#include "gui/elements/window.hpp"

class GuiWindow;

enum GUI_DOCK_NODE_MODE {
    GUI_DOCK_NODE_MULTIPLE,
    GUI_DOCK_NODE_SINGLE,
};

const float dock_resizer_thickness = 5.0f;
const float dock_border_thickness = .0f;
enum class GUI_DOCK_SPLIT {
    VERTICAL,
    HORIZONTAL
};
class GuiDockSpace;
class DockNode : public GuiElement {
    friend GuiDockSpace;

    GuiDockSpace* dock_space = 0;
    DockNode* parent_node = 0;
    GuiWindow* front_window = 0;
    GUI_DOCK_NODE_MODE mode = GUI_DOCK_NODE_MULTIPLE;
    std::string identifier;
    bool locked = false;

public:
    std::unique_ptr<DockNode> left;
    std::unique_ptr<DockNode> right;
    GUI_DOCK_SPLIT split_type;
    float split_pos = 0.5f;

    std::unique_ptr<GuiTabControl> tab_control;
    std::unique_ptr<GuiElement> container;
    std::unique_ptr<GuiDockDragDropSplitter> dock_drag_target;

    DockNode(GuiDockSpace* dock_space, DockNode* parent_node = 0);

    DockNode* findNode(const std::string& identifier) {
        if (isLeaf()) {
            if (getId() == identifier) {
                return this;
            } else {
                return 0;
            }
        }
        DockNode* n = left->findNode(identifier);
        if (n) {
            return n;
        }
        n = right->findNode(identifier);
        return n;
    }

    void setMode(GUI_DOCK_NODE_MODE mode) {
        this->mode = mode;
    }
    void setId(const std::string& identifier) {
        this->identifier = identifier;
    }
    const std::string& getId() const {
        return identifier;
    }
    void setLocked(bool locked) {
        this->locked = locked;
    }
    bool getLocked() const {
        return locked;
    }

    void setDockGroup(void* group) {
        dock_drag_target->setDockGroup(group);
        if (!isLeaf()) {
            left->setDockGroup(group);
            right->setDockGroup(group);
        }
    }

    bool isLeaf() const {
        return left == nullptr || right == nullptr;
    }
    bool isEmpty() const {
        return isLeaf() && children.empty();
    }

    GuiDockSpace* getDockSpace() {
        return dock_space;
    }

    void splitX(const char* node_name_left, const char* node_name_right, float ratio = .5f);
    void splitY(const char* node_name_left, const char* node_name_right, float ratio = .5f);
    DockNode* splitLeft(int size = 0);
    DockNode* splitRight(int size = 0);
    DockNode* splitTop(int size = 0);
    DockNode* splitBottom(int size = 0);

    gfxm::rect getResizeBarRect() const {
        gfxm::rect rc;
        auto l = left.get();
        auto r = right.get();
        if (split_type == GUI_DOCK_SPLIT::VERTICAL) {
            rc = gfxm::rect(
                gfxm::vec2(
                    client_area.min.x + (client_area.max.x - client_area.min.x) * split_pos - dock_resizer_thickness * 0.5f,
                    client_area.min.y
                ),
                gfxm::vec2(
                    client_area.min.x + (client_area.max.x - client_area.min.x) * split_pos + dock_resizer_thickness * 0.5f,
                    client_area.max.y
                )
            );
        } else if (split_type == GUI_DOCK_SPLIT::HORIZONTAL) {
            rc = gfxm::rect(
                gfxm::vec2(
                    client_area.min.x,
                    client_area.min.y + (client_area.max.y - client_area.min.y) * split_pos - dock_resizer_thickness * 0.5f
                ),
                gfxm::vec2(
                    client_area.max.x,
                    client_area.min.y + (client_area.max.y - client_area.min.y) * split_pos + dock_resizer_thickness * 0.5f
                )
            );
        }
        return rc;
    }

    void onHitTest(GuiHitResult& hit, int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return;
        }

        if (isLeaf()) {
            GuiElement::onHitTest(hit, x, y);
            return;
        } else {
            gfxm::rect resize_rect = getResizeBarRect();
            if (gfxm::point_in_rect(resize_rect, gfxm::vec2(x, y))) {
                if (split_type == GUI_DOCK_SPLIT::VERTICAL) {
                    hit.add(GUI_HIT::RIGHT, this);
                    return;
                } else if (split_type == GUI_DOCK_SPLIT::HORIZONTAL) {
                    hit.add(GUI_HIT::BOTTOM, this);
                    return;
                }
            }

            left->hitTest(hit, x, y);
            if(hit.hasHit()) {
                return;
            }
            
            right->hitTest(hit, x, y);
            if (hit.hasHit()) {
                return;
            }
        }
    }

    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override;

    void onLayout(const gfxm::vec2& extents, uint64_t flags) override {
        if (flags & GUI_LAYOUT_WIDTH_PASS) {
            rc_bounds.min.x = 0;
            rc_bounds.max.x = extents.x;
            client_area.min.x = rc_bounds.min.x;
            client_area.max.x = rc_bounds.max.x;

        }

        if (flags & GUI_LAYOUT_HEIGHT_PASS) {
            rc_bounds.min.y = 0;
            rc_bounds.max.y = extents.y;
            client_area.min.y = rc_bounds.min.y;
            client_area.max.y = rc_bounds.max.y;
        }

        if (flags & GUI_LAYOUT_POSITION_PASS) {

        }

        if(isLeaf()) {
            GuiElement::onLayout(extents, flags);
        } else {
            gfxm::rect rc = client_area;
            gfxm::rect lrc = rc;
            gfxm::rect rrc = rc;
            if (split_type == GUI_DOCK_SPLIT::VERTICAL) {
                lrc.max.x = rc.min.x + (rc.max.x - rc.min.x) * split_pos - dock_border_thickness * 0.5f;
                rrc.min.x = rc.min.x + (rc.max.x - rc.min.x) * split_pos + dock_border_thickness * 0.5f;
            } else if (split_type == GUI_DOCK_SPLIT::HORIZONTAL) {
                lrc.max.y = rc.min.y + (rc.max.y - rc.min.y) * split_pos - dock_border_thickness * 0.5f;
                rrc.min.y = rc.min.y + (rc.max.y - rc.min.y) * split_pos + dock_border_thickness * 0.5f;
            }

            if (flags & GUI_LAYOUT_WIDTH_PASS) {
                left->layout(gfxm::rect_size(lrc), GUI_LAYOUT_WIDTH_PASS);
                right->layout(gfxm::rect_size(rrc), GUI_LAYOUT_WIDTH_PASS);
            }
            if (flags & GUI_LAYOUT_HEIGHT_PASS) {
                left->layout(gfxm::rect_size(lrc), GUI_LAYOUT_HEIGHT_PASS);
                right->layout(gfxm::rect_size(rrc), GUI_LAYOUT_HEIGHT_PASS);
            }
            if (flags & GUI_LAYOUT_POSITION_PASS) {
                left->layout_position = lrc.min;
                left->layout(gfxm::rect_size(lrc), GUI_LAYOUT_POSITION_PASS);
                right->layout_position = rrc.min;
                right->layout(gfxm::rect_size(rrc), GUI_LAYOUT_POSITION_PASS);
            }
        }
    }

    void onDraw() override {
        if(isLeaf()) {
            GuiElement::onDraw();
        } else {
            guiDrawPushScissorRect(client_area);
            // TODO: should draw resize split bar here (?)
            left->draw();
            right->draw();
            guiDrawPopScissorRect();
        }

        // TODO: Dock splitter control
    }

    void addWindow(GuiWindow* wnd) {
        wnd->setSize(gui_vec2(gui::fill(), gui::fill()));
        
        auto tab_btn = tab_control->addTab(wnd->getTitle().c_str(), wnd);
        tab_btn->subscribe<GuiEvt_LClick>([this, wnd, tab_btn](const GuiEvt_LClick&) {
            tab_control->clearSelected();
            tab_btn->setSelected(true);
            tab_btn->setStyleDirty();
            container->clearChildren();
            container->pushBack(wnd);
            front_window = wnd;
            guiSetActiveWindow(front_window);
        });

        tab_control->clearSelected();
        tab_btn->setSelected(true);
        tab_btn->setStyleDirty();
        container->clearChildren();
        container->pushBack(wnd);
        front_window = wnd;
        guiSetActiveWindow(front_window);
    }
    void removeWindow(GuiWindow* wnd) {
        for (int i = 0; i < tab_control->getTabCount(); ++i) {
            auto btn = tab_control->getTabButton(i);
            if (btn->getUserPtr() == wnd) {
                tab_control->removeTab(i);
                if (wnd == front_window && tab_control->getTabCount() > 0) {
                    tab_control->setCurrentTab(std::min(tab_control->getTabCount() - 1, i));
                } else if(tab_control->getTabCount() == 0) {
                    container->clearChildren();
                    front_window = 0;
                }
                break;
            }
        }
    }
};