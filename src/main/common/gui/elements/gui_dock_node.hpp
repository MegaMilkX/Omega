#pragma once

#include "common/gui/elements/gui_tab_control.hpp"
#include "common/gui/elements/gui_window.hpp"

class GuiWindow;

const float dock_resize_border_thickness = 10.0f;
enum class GUI_DOCK_SPLIT {
    VERTICAL,
    HORIZONTAL
};
class DockNode : public GuiElement {
public:
    Font* font = 0; // Remove

    std::unique_ptr<DockNode> left;
    std::unique_ptr<DockNode> right;
    GUI_DOCK_SPLIT split_type;
    float split_pos = 0.5f;

    std::unique_ptr<GuiTabControl> tab_control;

    DockNode(Font* font)
    : font(font) {
        tab_control.reset(new GuiTabControl(font));
    }

    bool isLeaf() const {
        return left == nullptr || right == nullptr;
    }

    void splitV() {
        split_type = GUI_DOCK_SPLIT::VERTICAL;
        left.reset(new DockNode(font));
        right.reset(new DockNode(font));
    }
    void splitH() {
        split_type = GUI_DOCK_SPLIT::HORIZONTAL;
        left.reset(new DockNode(font));
        right.reset(new DockNode(font));
    }

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
            // TODO: Handle tabs
            GuiHitResult h = tab_control->hitTest(x, y);
            if (h.hit != GUI_HIT::NOWHERE) {
                return h;
            }
            for (auto & ch : children) {
                return ch->hitTest(x, y);
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

    void onMessage(GUI_MSG msg, uint64_t a_param, uint64_t b_param) override {
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
        }
    }

    void onLayout(const gfxm::rect& rect, uint64_t flags) override {
        this->bounding_rect = rect;
        this->client_area = bounding_rect;

        auto l = left.get();
        auto r = right.get();

        if(isLeaf()) {
            tab_control->onLayout(client_area, 0);
            gfxm::rect new_rc = client_area;
            new_rc.min.y = tab_control->getClientArea().max.y;
            // TODO: Show only one window currently tabbed into
            for (auto& ch : children) {
                ch->onLayout(new_rc, GUI_LAYOUT_NO_TITLE | GUI_LAYOUT_NO_BORDER);
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
            for (auto& ch : children) {
                ch->onDraw();
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
        
        {
            if (isLeaf()) {
                glScissor(
                    client_area.min.x,
                    sh - client_area.max.y,
                    client_area.max.x - client_area.min.x,
                    client_area.max.y - client_area.min.y
                );
                gfxm::vec2 center = gfxm::lerp(client_area.min, client_area.max, 0.5f);
                gfxm::rect rc(
                    center - gfxm::vec2(20.0f, 20.0f),
                    center + gfxm::vec2(20.0f, 20.0f)
                );

                guiDrawRect(rc, GUI_COL_BUTTON);
            }
        }
    }

    void addWindow(GuiWindow* wnd) {
        GuiElement::addChild(wnd);
        tab_control->setTabCount(tab_control->getTabCount() + 1);
        tab_control->getTabButton(tab_control->getTabCount() - 1)->setCaption(wnd->getTitle());
    }
private:
    void addChild(GuiElement* elem) override {}
};