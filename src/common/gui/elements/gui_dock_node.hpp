#pragma once

#include "gui/elements/gui_tab_control.hpp"
#include "gui/elements/gui_dock_drag_target.hpp"
#include "gui/elements/gui_window.hpp"

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
    std::string identifier;
    bool locked = false;

    void moveChildWindows(DockNode* from) {
        std::vector<GuiElement*> from_children_copy = from->children;
        for (auto c : from_children_copy) {
            from->removeWindow((GuiWindow*)c);
            addWindow((GuiWindow*)c);
        }
    }
public:
    std::unique_ptr<DockNode> left;
    std::unique_ptr<DockNode> right;
    GUI_DOCK_SPLIT split_type;
    float split_pos = 0.5f;

    std::unique_ptr<GuiTabControl> tab_control;
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

    GuiHitResult onHitTest(int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, this };
        }

        if (isLeaf()) {
            GuiHitResult h = tab_control->onHitTest(x, y);
            if (h.hit != GUI_HIT::NOWHERE) {
                return h;
            }
            if (front_window) {
                return front_window->onHitTest(x, y);
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

            GuiHitResult h = left->onHitTest(x, y);
            if(h.hit != GUI_HIT::NOWHERE) {
                return h;
            }
            
            return right->onHitTest(x, y);
        }
    }

    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override;

    void onLayout(const gfxm::rect& rect, uint64_t flags) override {
        this->rc_bounds = rect;
        this->client_area = rc_bounds;

        auto l = left.get();
        auto r = right.get();

        if(isLeaf()) {
            for (int i = 0; i < tab_control->getTabCount(); ++i) {
                if (guiGetActiveWindow() == tab_control->getTabButton(i)->getUserPtr()) {
                    tab_control->getTabButton(i)->setHighlighted(true);
                } else {
                    tab_control->getTabButton(i)->setHighlighted(false);
                }
            }
            tab_control->layout(client_area, 0);
            gfxm::rect new_rc = client_area;
            new_rc.min.y = tab_control->getClientArea().max.y;

            if (front_window) {
                front_window->layout(new_rc, GUI_LAYOUT_NO_TITLE | GUI_LAYOUT_NO_BORDER);
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

            left->layout(lrc, 0);
            right->layout(rrc, 0);
        }
    }

    void onDraw() override {
        auto l = left.get();
        auto r = right.get();
        if(isLeaf()) {
            guiDrawPushScissorRect(client_area);

            tab_control->draw();

            if (front_window == guiGetActiveWindow()) {
                guiDrawRect(
                    gfxm::rect(
                        gfxm::vec2(tab_control->getClientArea().min.x, tab_control->getClientArea().max.y - 3.0f),
                        gfxm::vec2(tab_control->getClientArea().max.x, tab_control->getClientArea().max.y)
                    ),
                    GUI_COL_ACCENT
                );
            }

            // TODO: Show only one window currently tabbed into
            if (front_window) {
                front_window->draw();
            }

            //guiDrawText(client_area.min, identifier.c_str(), guiGetCurrentFont(), 0, GUI_COL_TEXT);
            guiDrawPopScissorRect();
        } else {
            guiDrawPushScissorRect(client_area);
            // TODO: should draw resize split bar here (?)
            l->draw();
            r->draw();
            guiDrawPopScissorRect();
        }

        // TODO: Dock splitter control
    }

    void addWindow(GuiWindow* wnd) {
        addChild(wnd);
    }
    void removeWindow(GuiWindow* wnd) {
        removeChild(wnd);
    }
private:
    void addChild(GuiElement* elem) override {
        GuiElement::addChild(elem);
        GuiWindow* wnd = (GuiWindow*)elem;
        tab_control->addTab(wnd->getTitle().c_str(), wnd);
        front_window = wnd;
    }
    void removeChild(GuiElement* elem) override {
        GuiWindow* wnd = (GuiWindow*)elem;
        for (int i = 0; i < tab_control->getTabCount(); ++i) {
            auto btn = tab_control->getTabButton(i);
            if (btn->getUserPtr() == wnd) {
                tab_control->removeTab(i);
                if (wnd == front_window && tab_control->getTabCount() > 0) {
                    tab_control->setCurrentTab(std::min(tab_control->getTabCount() - 1, i));
                } else if(tab_control->getTabCount() == 0) {
                    front_window = 0;
                }
                break;
            }
        }
        GuiElement::removeChild(wnd);
    }
};