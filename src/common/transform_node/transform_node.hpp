#pragma once

#include "math/gfxm.hpp"
#include "handle/handle.hpp"

typedef void (*pfn_transform_callback_t)(void*);

struct TransformCallback {
    pfn_transform_callback_t callback;
    std::unique_ptr<TransformCallback> next;
    void* context;
    int id;

    void call() {
        callback(context);
        if (next) {
            next->callback(next->context);
        }
    }
};

class TransformNode {
    friend void transformNodeAttach(Handle<TransformNode> parent, Handle<TransformNode> child);

    Handle<TransformNode> parent;
    Handle<TransformNode> next_sibling;
    Handle<TransformNode> first_child;
    bool dirty_ = true;

    gfxm::vec3 translation = gfxm::vec3(0, 0, 0);
    gfxm::quat rotation = gfxm::quat(0, 0, 0, 1);
    gfxm::vec3 scale = gfxm::vec3(1, 1, 1);
    gfxm::mat4 world_transform = gfxm::mat4(1.f);

    std::unique_ptr<TransformCallback> dirty_callback;

    inline void dirty() {
        if (dirty_callback) {
            dirty_callback->call();
        }

        if (dirty_) {
            return;
        }

        dirty_ = true;
        auto ch = first_child;
        while (ch.isValid()) {
            ch->dirty();
            ch = ch->next_sibling;
        }
    }
    void _insertChild(Handle<TransformNode> child) {
        if (!first_child.isValid()) {
            first_child = child;
        } else {
            auto ch = first_child;
            while (ch.isValid()) {
                if (!ch->next_sibling.isValid()) {
                    ch->next_sibling = child;
                    break;
                }
                ch = ch->next_sibling;
            }
        }
    }
    void _eraseChild(Handle<TransformNode> child) {
        if (first_child == child) {
            first_child = first_child->next_sibling;
        } else {
            auto ch = first_child;
            while (ch.isValid()) {
                if (ch->next_sibling == child) {
                    ch->next_sibling = ch->next_sibling->next_sibling;
                    break;
                }
                ch = ch->next_sibling;
            }
        }
    }
    void _eraseChild(TransformNode* pchild) {
        if (!first_child.isValid()) {
            return;
        }
        if (first_child.deref() == pchild) {
            first_child = first_child->next_sibling;
        } else {
            auto ch = first_child;
            while (ch.isValid()) {
                if (ch->next_sibling.isValid() && ch->next_sibling.deref() == pchild) {
                    ch->next_sibling = ch->next_sibling->next_sibling;
                    break;
                }
                ch = ch->next_sibling;
            }
        }
    }
public:
    ~TransformNode() {
        if (parent.isValid()) {
            parent->_eraseChild(this);
        }
    }

    int addDirtyCallback(pfn_transform_callback_t cb, void* context);
    void removeDirtyCallback(int id);

    Handle<TransformNode> getParent() const {
        return parent;
    }

    void translate(float x, float y, float z);
    void translate(const gfxm::vec3& t);
    void rotate(float angle, const gfxm::vec3& axis);
    void rotate(const gfxm::quat& q);

    void setTranslation(float x, float y, float z);
    void setTranslation(const gfxm::vec3& t);
    void setRotation(const gfxm::quat& q);
    void setScale(const gfxm::vec3& s);

    const gfxm::vec3& getTranslation() const { return translation; }
    const gfxm::quat& getRotation() const { return rotation; }
    const gfxm::vec3& getScale() const { return scale; }

    gfxm::vec3 getWorldTranslation() { return getWorldTransform()[3]; }
    gfxm::quat getWorldRotation() { return gfxm::to_quat(gfxm::to_orient_mat3(getWorldTransform())); }

    gfxm::vec3 getWorldForward() { return gfxm::normalize(getWorldTransform()[2]); }
    gfxm::vec3 getWorldBack() { return gfxm::normalize(-getWorldTransform()[2]); }
    gfxm::vec3 getWorldLeft() { return gfxm::normalize(-getWorldTransform()[0]); }
    gfxm::vec3 getWorldRight() { return gfxm::normalize(getWorldTransform()[0]); }
    gfxm::vec3 getWorldUp() { return gfxm::normalize(getWorldTransform()[1]); }
    gfxm::vec3 getWorldDown() { return gfxm::normalize(-getWorldTransform()[1]); }

    gfxm::mat4 getLocalTransform();
    const gfxm::mat4& getWorldTransform();
};


inline void transformNodeAttach(Handle<TransformNode> parent, Handle<TransformNode> child) {
    assert(parent.isValid() && child.isValid());
    auto old_parent = child->parent;
    if (old_parent.isValid()) {
        old_parent->_eraseChild(child);
    }
    child->parent = parent;
    if (parent.isValid()) {
        parent->_insertChild(child);
    }
}