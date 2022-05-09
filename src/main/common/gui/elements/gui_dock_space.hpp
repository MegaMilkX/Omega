#pragma once


#include "common/gui/elements/gui_dock_node.hpp"
class GuiDockSpace : public GuiElement {
    DockNode root;
public:
    GuiDockSpace(Font* font);
    ~GuiDockSpace();

    DockNode* getRoot() {
        return &root;
    }

    GuiHitResult hitTest(int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, this };
        }

        return root.hitTest(x, y);/*
        std::stack<DockNode*> stack;
        stack.push(&root);
        while (!stack.empty()) {
            DockNode* node = stack.top();
            stack.pop();

            if (node->isLeaf()) {

            } else {
                gfxm::rect resize_rect = node->getResizeBarRect();
                if (gfxm::point_in_rect(resize_rect, gfxm::vec2(x, y))) {
                    if (node->split_type == GUI_DOCK_SPLIT::VERTICAL) {
                        return GuiHitResult{ GUI_HIT::RIGHT, node };
                    } else if(node->split_type == GUI_DOCK_SPLIT::VERTICAL) {
                        return GuiHitResult{ GUI_HIT::BOTTOM, node };
                    }
                }
                auto l = node->left.get();
                auto r = node->right.get();
                stack.push(l);
                stack.push(r);
            }
        }

        return GuiHitResult{ GUI_HIT::NOWHERE, this };*/
    }

    void onLayout(const gfxm::rect& rect, uint64_t flags) override {
        this->bounding_rect = rect;
        this->client_area = bounding_rect;
        
        root.onLayout(client_area, 0);
    }

    void onDraw() override {
        int sw = 0, sh = 0;
        platformGetWindowSize(sw, sh);
        glScissor(
            client_area.min.x,
            sh - client_area.max.y,
            client_area.max.x - client_area.min.x,
            client_area.max.y - client_area.min.y
        );

        guiDrawRect(client_area, GUI_COL_HEADER);

        root.onDraw();
    }
};
