#pragma once

#include <assert.h>
#include <array>
#include "math/gfxm.hpp"
#include "debug_draw/debug_draw.hpp"
#include "log/log.hpp"
#include "collision/intersection/ray.hpp"
#include "collision/intersection/capsule_capsule.hpp"


class Collider;

struct AabbTreeNode;
struct AabbTreeElement {
    Collider* collider = 0;
    AabbTreeNode* node = 0;
    gfxm::aabb aabb;
};
struct AabbTreeNode {
    gfxm::aabb aabb;
    AabbTreeElement* elem = 0;
    AabbTreeNode* parent = 0;
    AabbTreeNode* left = 0;
    AabbTreeNode* right = 0;
    bool aabb_dirty = false;

    bool isLeaf() const { return left == nullptr; }
    AabbTreeNode* getSibling() const {
        return this == parent->left
            ? parent->right
            : parent->left;
    }

    void setAabbDirtyFlag() {
        if (aabb_dirty) {
            return;
        }
        aabb_dirty = true;
        if (parent) {
            parent->setAabbDirtyFlag();
        }
    }

    void setLeaf(AabbTreeElement* elem) {
        this->elem = elem;
        elem->node = this;
        
        left = 0;
        right = 0;
    }
    void setBranch(AabbTreeNode* l, AabbTreeNode* r) {
        l->parent = this;
        r->parent = this;

        left = l;
        right = r;
        elem = 0;
    }
    void updateAabb() {
        if (isLeaf()) {
            aabb = elem->aabb;
        } else {
            aabb = gfxm::aabb_union(left->aabb, right->aabb);
        }
    }
    void updateDirtyAabb() {
        if (aabb_dirty) {
            if (isLeaf()) {
                aabb = elem->aabb;
            } else {
                left->updateDirtyAabb();
                right->updateDirtyAabb();
                aabb = gfxm::aabb_union(left->aabb, right->aabb);
            }
            aabb_dirty = false;
        }
    }

    bool rayTest(const gfxm::ray& ray, void* context, void(*callback_fn)(void*, const gfxm::ray&, Collider*)) {
        if (intersectRayAabb(ray, aabb)) {
            if (isLeaf()) {
                //dbgDrawAabb(gfxm::aabb_grow(aabb, .1f), DBG_COLOR_RED);
                callback_fn(context, ray, elem->collider);
                return true;
            } else {
                //dbgDrawAabb(gfxm::aabb_grow(aabb, .1f), 0xFFFF00FF);
                bool l = left->rayTest(ray, context, callback_fn);
                bool r = right->rayTest(ray, context, callback_fn);
                return l || r;
            }
        }
        return false;
    }
    bool sphereSweep(const gfxm::vec3& from, const gfxm::vec3& to, float radius, void* context, void(*callback_fn)(void*, const gfxm::vec3&, const gfxm::vec3&, float, Collider*)) {
        if (intersectCapsuleAabb(from, to, radius, aabb)) {
            if (isLeaf()) {
                callback_fn(context, from, to, radius, elem->collider);
                return true;
            } else {
                bool l = left->sphereSweep(from, to, radius, context, callback_fn);
                bool r = right->sphereSweep(from, to, radius, context, callback_fn);
                return l || r;
            }
        }

        return false;
    }

    void debugDraw() {
        if (!isLeaf()) {
            dbgDrawAabb(aabb, 0xFF00FFFF);
            left->debugDraw();
            right->debugDraw();
        }
    }
};
class AabbTree {
    AabbTreeNode* root = 0;

    void insertNode(AabbTreeNode* node, AabbTreeNode** parent) {
        AabbTreeNode *p = *parent;
        if (p->isLeaf()) {
            AabbTreeNode* newParent = new AabbTreeNode();
            newParent->parent = p->parent;
            newParent->setBranch(node, p);
            newParent->setAabbDirtyFlag();
            *parent = newParent;
        } else {
            gfxm::aabb aabb0 = p->left->aabb;
            gfxm::aabb aabb1 = p->right->aabb;
            float vol_diff0 =
                gfxm::volume(gfxm::aabb_union(aabb0, node->aabb)) - gfxm::volume(aabb0);
            float vol_diff1 =
                gfxm::volume(gfxm::aabb_union(aabb1, node->aabb)) - gfxm::volume(aabb1);

            if (vol_diff0 < vol_diff1) {
                insertNode(node, &p->left);
            } else {
                insertNode(node, &p->right);
            }
        }
    }
    void removeNode(AabbTreeNode* node) {
        AabbTreeNode* parent = node->parent;
        if (parent) {
            AabbTreeNode* sibling = node->getSibling();
            if (parent->parent) {
                sibling->parent = parent->parent;
                (parent == parent->parent->left
                    ? parent->parent->left
                    : parent->parent->right) = sibling;
            } else {
                root = sibling;
                sibling->parent = 0;
            }
            delete node;
            delete parent;
        } else {
            root = 0;
            delete node;
        }
    }
public:
    void add(AabbTreeElement* elem) {
        if (!root) {
            root = new AabbTreeNode;
            root->setLeaf(elem);
            root->updateAabb();
        } else {
            AabbTreeNode* node = new AabbTreeNode;
            node->setLeaf(elem);
            node->updateAabb();
            insertNode(node, &root);
        }
    }
    void remove(AabbTreeElement* elem) {
        AabbTreeNode* node = elem->node;
        if (!node) {
            assert(false);
            return;
        }
        node->elem = 0;
        elem->node = 0;
        removeNode(node);
    }

    void rayTest(const gfxm::ray& ray, void* context, void(*callback_fn)(void*, const gfxm::ray&, Collider*)) {
        if (root) {
            root->rayTest(ray, context, callback_fn);
        }
    }
    void sphereSweep(const gfxm::vec3& from, const gfxm::vec3& to, float radius, void* context, void(*callback_fn)(void*, const gfxm::vec3&, const gfxm::vec3&, float, Collider*)) {
        if (root) {
            root->sphereSweep(from, to, radius, context, callback_fn);
        }
    }

    void update() {
        if (root) {
            root->updateDirtyAabb();
        }
    }

    void debugDraw() {
        if (root) {
            root->debugDraw();
        }
    }
};