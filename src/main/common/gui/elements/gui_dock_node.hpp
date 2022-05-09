#pragma once

#include "common/gui/elements/gui_tab_control.hpp"
#include "common/gui/elements/gui_dock_drag_target.hpp"
#include "common/gui/elements/gui_window.hpp"

class GuiWindow;

const float dock_resize_border_thickness = 10.0f;
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

    void moveChildWindows(DockNode* from) {
        std::vector<GuiElement*> from_children_copy = from->children;
        for (auto c : from_children_copy) {
            from->removeWindow((GuiWindow*)c);
            addWindow((GuiWindow*)c);
        }
    }
public:
    Font* font = 0; // Remove

    std::unique_ptr<DockNode> left;
    std::unique_ptr<DockNode> right;
    GUI_DOCK_SPLIT split_type;
    float split_pos = 0.5f;

    std::unique_ptr<GuiTabControl> tab_control;
    std::unique_ptr<GuiDockDragDropSplitter> dock_drag_target;

    DockNode(Font* font, GuiDockSpace* dock_space, DockNode* parent_node = 0)
    : font(font), dock_space(dock_space), parent_node(parent_node) {
        tab_control.reset(new GuiTabControl(font));
        tab_control->setOwner(this);
        dock_drag_target.reset(new GuiDockDragDropSplitter());
        dock_drag_target->setOwner(this);
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

    DockNode* splitLeft();
    DockNode* splitRight();
    DockNode* splitTop();
    DockNode* splitBottom();

    gfxm::rect getResizeBarRect() const {
        gfxm::rect rc;
        auto l = left.get();
        auto r = right.get();
        if (split_type == GUI_DOCK_SPLIT::VERTICAL) {
            rc = gfxm::rect(
                gfxm::vec2(
                    client_area.min.x + (client_area.max.x - client_area.min.x) * split_pos - dock_resize_border_thickness * 0.5f,
                    client_area.min.y
                ),
                gfxm::vec2(
                    client_area.min.x + (client_area.max.x - client_area.min.x) * split_pos + dock_resize_border_thickness * 0.5f,
                    client_area.max.y
                )
            );
        } else if (split_type == GUI_DOCK_SPLIT::HORIZONTAL) {
            rc = gfxm::rect(
                gfxm::vec2(
                    client_area.min.x,
                    client_area.min.y + (client_area.max.y - client_area.min.y) * split_pos - dock_resize_border_thickness * 0.5f
                ),
                gfxm::vec2(
                    client_area.max.x,
                    client_area.min.y + (client_area.max.y - client_area.min.y) * split_pos + dock_resize_border_thickness * 0.5f
                )
            );
        }
        return rc;
    }

    GuiHitResult hitTest(int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, this };
        }

        if (isLeaf()) {
            GuiHitResult h = tab_control->hitTest(x, y);
            if (h.hit != GUI_HIT::NOWHERE) {
                return h;
            }
            h = dock_drag_target->hitTest(x, y);
            if (h.hit != GUI_HIT::NOWHERE) {
                return h;
            }
            // TODO: Handle tabs
            if (front_window) {
                return front_window->hitTest(x, y);
            }
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        } else {
            gfxm::rect resize_rect = getResizeBarRect();
            if (gfxm::point_in_rect(resize_rect, gfxm::vec2(x, y))) {
                if (split_type == GUI_DOCK_SPLIT::VERTICAL) {
                    return GuiHitResult{ GUI_HIT::RIGHT, this };
                } else if (split_type == GUI_DOCK_SPLIT::HORIZONTAL) {
                    return GuiHitResult{ GUI_HIT::BOTTOM, this };
                }
            }

            GuiHitResult h = left->hitTest(x, y);
            if(h.hit != GUI_HIT::NOWHERE) {
                return h;
            }
            
            return right->hitTest(x, y);
        }
    }

    void onMessage(GUI_MSG msg, uint64_t a_param, uint64_t b_param) override;

    void onLayout(const gfxm::rect& rect, uint64_t flags) override {
        this->bounding_rect = rect;
        this->client_area = bounding_rect;

        auto l = left.get();
        auto r = right.get();

        if(isLeaf()) {
            tab_control->onLayout(client_area, 0);
            gfxm::rect new_rc = client_area;
            new_rc.min.y = tab_control->getClientArea().max.y;
            dock_drag_target->onLayout(new_rc, 0);
            // TODO: Show only one window currently tabbed into
            if (front_window) {
                front_window->onLayout(new_rc, GUI_LAYOUT_NO_TITLE | GUI_LAYOUT_NO_BORDER);
            }
        } else {
            gfxm::rect rc = client_area;
            gfxm::rect lrc = rc;
            gfxm::rect rrc = rc;
            if (split_type == GUI_DOCK_SPLIT::VERTICAL) {
                lrc.max.x = rc.min.x + (rc.max.x - rc.min.x) * split_pos - dock_resize_border_thickness * 0.5f;
                rrc.min.x = rc.min.x + (rc.max.x - rc.min.x) * split_pos + dock_resize_border_thickness * 0.5f;
            } else if (split_type == GUI_DOCK_SPLIT::HORIZONTAL) {
                lrc.max.y = rc.min.y + (rc.max.y - rc.min.y) * split_pos - dock_resize_border_thickness * 0.5f;
                rrc.min.y = rc.min.y + (rc.max.y - rc.min.y) * split_pos + dock_resize_border_thickness * 0.5f;
            }

            left->onLayout(lrc, 0);
            right->onLayout(rrc, 0);
        }
    }

    void onDraw() override {
        int sw = 0, sh = 0;
        platformGetWindowSize(sw, sh);
        

        auto l = left.get();
        auto r = right.get();
        if(isLeaf()) {
            glScissor(
                client_area.min.x,
                sh - client_area.max.y,
                client_area.max.x - client_area.min.x,
                client_area.max.y - client_area.min.y
            );
            tab_control->onDraw();

            glScissor(
                client_area.min.x,
                sh - client_area.max.y,
                client_area.max.x - client_area.min.x,
                client_area.max.y - client_area.min.y
            );
            // TODO: Show only one window currently tabbed into
            if (front_window) {
                front_window->onDraw();
            }
            if (guiIsDragDropInProgress()) {
                dock_drag_target->onDraw();
            }
        } else {
            glScissor(
                client_area.min.x,
                sh - client_area.max.y,
                client_area.max.x - client_area.min.x,
                client_area.max.y - client_area.min.y
            );
            // TODO: should draw resize split bar here (?)
            l->onDraw();
            r->onDraw();
        }

        // TODO: Dock splitter control
    }

    void addWindow(GuiWindow* wnd) {
        GuiElement::addChild(wnd);
        tab_control->setTabCount(tab_control->getTabCount() + 1);
        tab_control->getTabButton(tab_control->getTabCount() - 1)->setCaption(wnd->getTitle());
        front_window = wnd;
    }
    void removeWindow(GuiWindow* wnd) {
        if (front_window == wnd) {
            front_window = 0;
        }
        int id = GuiElement::getChildId(wnd);
        assert(id >= 0);
        if (id < 0) {
            return;
        }
        GuiElement::removeChild(wnd);
        tab_control->removeTab(id);
        if (children.size() > 0) {
            if (id >= children.size()) {
                front_window = (GuiWindow*)children[children.size() - 1];
            } else {
                front_window = (GuiWindow*)children[id];
            }
        }
    }
private:
    void addChild(GuiElement* elem) override {}
};