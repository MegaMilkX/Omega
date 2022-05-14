#pragma once

#include <stack>

#include "common/gui/elements/gui_dock_node.hpp"
class GuiDockSpace : public GuiElement {
    Font* font = 0;
    std::unique_ptr<DockNode> root;

    std::unique_ptr<DockNode>* findNodePtr(DockNode* node) {
        std::unique_ptr<DockNode>* ptr = 0;

        std::stack<std::unique_ptr<DockNode>*> stack;
        stack.push(&root);
        while (!stack.empty()) {
            std::unique_ptr<DockNode>* nodeptr = stack.top();
            stack.pop();

            if ((*nodeptr).get() == node) {
                ptr = nodeptr;
                break;
            }

            if (!(*nodeptr)->isLeaf()) {
                auto l = &(*nodeptr)->left;
                auto r = &(*nodeptr)->right;
                stack.push(l);
                stack.push(r);
            }
        }
        return ptr;
    }
public:
    GuiDockSpace(Font* font);
    ~GuiDockSpace();

    DockNode* getRoot() {
        return root.get();
    }

    DockNode* splitLeft(DockNode* node, GUI_DOCK_SPLIT split) {
        std::unique_ptr<DockNode>* ptr = findNodePtr(node);
        DockNode* new_node = new DockNode(font, this, (*ptr)->parent_node);
        new_node->split_type = split;
        new_node->left.reset(new DockNode(font, this, new_node));
        new_node->right = std::move((*ptr));
        new_node->right->parent_node = new_node;
        new_node->dock_drag_target->setEnabled(false);
        (*ptr).reset(new_node);
        return new_node;
    }
    DockNode* splitRight(DockNode* node, GUI_DOCK_SPLIT split) {
        std::unique_ptr<DockNode>* ptr = findNodePtr(node);
        DockNode* new_node = new DockNode(font, this, (*ptr)->parent_node);
        new_node->split_type = split;
        new_node->left = std::move((*ptr));
        new_node->left->parent_node = new_node;
        new_node->right.reset(new DockNode(font, this, new_node));
        new_node->dock_drag_target->setEnabled(false);
        (*ptr).reset(new_node);
        return new_node;
    }
    bool collapse(DockNode* node) {
        std::unique_ptr<DockNode>* ptr_to_replace = 0;
        
        std::stack<std::unique_ptr<DockNode>*> stack;
        stack.push(&root);
        while (!stack.empty()) {
            std::unique_ptr<DockNode>* nodeptr = stack.top();
            stack.pop();

            if ((*nodeptr).get() == node) {
                ptr_to_replace = nodeptr;
                break;
            }

            if (!(*nodeptr)->isLeaf()) {
                auto l = &(*nodeptr)->left;
                auto r = &(*nodeptr)->right;
                stack.push(l);
                stack.push(r);
            }
        }

        assert(ptr_to_replace);
        assert(!(*ptr_to_replace)->isLeaf());
        if ((*ptr_to_replace)->isLeaf()) {
            return false;
        }

        auto parent_elem = (*ptr_to_replace)->getParent();
        DockNode* parent_node = (*ptr_to_replace)->parent_node;
        auto owner = (*ptr_to_replace)->getOwner();

        std::unique_ptr<DockNode> a;
        std::unique_ptr<DockNode> b;
        if ((*ptr_to_replace)->left->isEmpty()) {
            a = std::move((*ptr_to_replace)->left);
            b = std::move((*ptr_to_replace)->right);
        } else if ((*ptr_to_replace)->right->isEmpty()) {
            a = std::move((*ptr_to_replace)->right);
            b = std::move((*ptr_to_replace)->left);
        } else {
            assert(false);
            return false;
        }

        a.reset();
        (*ptr_to_replace) = std::move(b);
        (*ptr_to_replace)->parent = parent_elem;
        (*ptr_to_replace)->parent_node = parent_node;
        (*ptr_to_replace)->setOwner(owner);

        return true;
    }

    GuiHitResult hitTest(int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, this };
        }

        return root->hitTest(x, y);/*
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
        
        root->onLayout(client_area, 0);
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

        root->onDraw();
    }
};
