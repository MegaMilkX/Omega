#pragma once

#include <stack>

#include "gui/elements/gui_dock_node.hpp"
class GuiDockSpace : public GuiElement {
    void* dock_group = 0;
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
    GuiDockSpace(void* dock_group = 0);
    ~GuiDockSpace();

    void setDockGroup(void* group) { 
        dock_group = group;
        root->setDockGroup(group);
    }
    void* getDockGroup() const { return dock_group; }

    DockNode* getRoot() {
        return root.get();
    }

    DockNode* findNode(const std::string& identifier) {
        if (root->getId() == identifier) {
            return root.get();
        }
        return root->findNode(identifier);
    }

    DockNode* splitLeft(DockNode* node, GUI_DOCK_SPLIT split, int size = 0) {
        assert(size >= 0);
        std::unique_ptr<DockNode>* ptr = findNodePtr(node);
        gfxm::rect containing_rc = (*ptr)->getBoundingRect();
        gfxm::vec2 containing_size = containing_rc.max - containing_rc.min;
        DockNode* new_node = new DockNode(this, (*ptr)->parent_node);
        new_node->split_type = split;
        new_node->left.reset(new DockNode(this, new_node));
        new_node->right = std::move((*ptr));
        new_node->right->parent_node = new_node;
        new_node->dock_drag_target->setEnabled(false);
        new_node->split_pos = .5f;
        if(size > 0) {
            if (split == GUI_DOCK_SPLIT::HORIZONTAL) {
                new_node->split_pos = size / containing_size.y;
            } else if(split == GUI_DOCK_SPLIT::VERTICAL) {
                new_node->split_pos = size / containing_size.x;
            }
        }
        (*ptr).reset(new_node);
        return new_node;
    }
    DockNode* splitRight(DockNode* node, GUI_DOCK_SPLIT split, int size = 0) {
        assert(size >= 0);
        std::unique_ptr<DockNode>* ptr = findNodePtr(node);
        gfxm::rect containing_rc = (*ptr)->getBoundingRect();
        gfxm::vec2 containing_size = containing_rc.max - containing_rc.min;
        DockNode* new_node = new DockNode(this, (*ptr)->parent_node);
        new_node->split_type = split;
        new_node->left = std::move((*ptr));
        new_node->left->parent_node = new_node;
        new_node->right.reset(new DockNode(this, new_node));
        new_node->dock_drag_target->setEnabled(false);
        new_node->split_pos = .5f;
        if(size > 0) {
            if (split == GUI_DOCK_SPLIT::HORIZONTAL) {
                new_node->split_pos = 1.f - size / containing_size.y;
            } else if(split == GUI_DOCK_SPLIT::VERTICAL) {
                new_node->split_pos = 1.f - size / containing_size.x;
            }
        }
        (*ptr).reset(new_node);
        return new_node;
    }
    bool collapseBranch(DockNode* node) {
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
        if ((*ptr_to_replace)->left->isEmpty() && !(*ptr_to_replace)->left->getLocked()) {
            a = std::move((*ptr_to_replace)->left);
            b = std::move((*ptr_to_replace)->right);
        } else if ((*ptr_to_replace)->right->isEmpty() && !(*ptr_to_replace)->right->getLocked()) {
            a = std::move((*ptr_to_replace)->right);
            b = std::move((*ptr_to_replace)->left);
        } else {
            return false;
        }

        a.reset();
        (*ptr_to_replace) = std::move(b);
        (*ptr_to_replace)->parent = parent_elem;
        (*ptr_to_replace)->parent_node = parent_node;
        (*ptr_to_replace)->setOwner(owner);

        return true;
    }

    GuiHitResult onHitTest(int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, this };
        }

        return root->onHitTest(x, y);
    }

    void onLayout(const gfxm::rect& rect, uint64_t flags) override {
        this->rc_bounds = rect;
        this->client_area = rc_bounds;
        
        root->layout(client_area, 0);
    }

    void onDraw() override {
        guiDrawPushScissorRect(client_area);

        guiDrawRect(client_area, GUI_COL_HEADER);
        root->draw();

        guiDrawPopScissorRect();
    }

    GUI_DOCK getDockPosition() const override {
        return GUI_DOCK::FILL;
    }
};
