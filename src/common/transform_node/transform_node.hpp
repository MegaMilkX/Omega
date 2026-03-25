#pragma once

#include "math/gfxm.hpp"
#include "handle/handle.hpp"
#include "transform_system.hpp"
#include "transform_dirty_list.hpp"


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


typedef uint32_t transform_inherit_flags_t;
constexpr transform_inherit_flags_t TRANSFORM_INHERIT_POSITION  = 0x01;
constexpr transform_inherit_flags_t TRANSFORM_INHERIT_ROTATION  = 0x02;
constexpr transform_inherit_flags_t TRANSFORM_INHERIT_SCALE     = 0x04;
constexpr transform_inherit_flags_t TRANSFORM_INHERIT_ALL       = TRANSFORM_INHERIT_POSITION
                                                                | TRANSFORM_INHERIT_ROTATION
                                                                | TRANSFORM_INHERIT_SCALE;

class TransformNode;
using HTransform = Handle<TransformNode>;

class TransformNode {
    friend void transformNodeAttach(Handle<TransformNode> parent, Handle<TransformNode> child);

    Handle<TransformNode> parent;
    Handle<TransformNode> next_sibling;
    Handle<TransformNode> first_child;
    mutable bool dirty_ = true; // For world matrix caching
    mutable uint32_t generation = -1; // For dirty tickets

    gfxm::vec3 translation = gfxm::vec3(0, 0, 0);
    gfxm::quat rotation = gfxm::quat(0, 0, 0, 1);
    gfxm::vec3 scale = gfxm::vec3(1, 1, 1);
    mutable gfxm::mat4 world_transform = gfxm::mat4(1.f);

    std::unique_ptr<TransformCallback> dirty_callback;
    TransformTicket* first_ticket = nullptr;

    transform_inherit_flags_t inherit_flags = TRANSFORM_INHERIT_ALL;

    inline void dirty() {
        if (dirty_callback) {
            dirty_callback->call();
        }

        _fireTickets();

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

    void _fireTickets() {
        if (generation == TransformSystem::getGeneration()) {
            return;
        }
        generation = TransformSystem::getGeneration();

        auto cur = first_ticket;
        while (cur) {
            cur->markDirty();
            cur = cur->next;
        }

        auto ch = first_child;
        while (ch.isValid()) {
            ch->_fireTickets();
            ch = ch->next_sibling;
        }
    }

    void _validateChildren() {
        auto ch = first_child;
        while (ch.isValid()) {
            ch->setDirty();
            ch = ch->next_sibling;
            if (ch == first_child && ch.isValid()) {
                LOG_ERR("CRITICAL: TransformNode children are looping");
                assert(false);
                return;
            }
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
                    child->next_sibling = Handle<TransformNode>();
                    break;
                }
                ch = ch->next_sibling;
            }
        }
        _validateChildren();
    }
    void _eraseChild(Handle<TransformNode> child) {
        if (first_child == child) {
            first_child = first_child->next_sibling;
        } else {
            auto ch = first_child;
            while (ch.isValid()) {
                if (ch->next_sibling == child) {
                    ch->next_sibling = ch->next_sibling->next_sibling;
                    child->next_sibling = Handle<TransformNode>();
                    break;
                }
                ch = ch->next_sibling;
            }
        }
        _validateChildren();
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
        _validateChildren();
    }
public:
    ~TransformNode() {
        if (parent.isValid()) {
            parent->_eraseChild(this);
        }
    }

    void setDirty() {
        dirty();
        _fireTickets();
    }

    int addDirtyCallback(pfn_transform_callback_t cb, void* context);
    void removeDirtyCallback(int id);

    void attachTicket(TransformTicket* t);
    void detachTicket(TransformTicket* t);

    Handle<TransformNode> getParent() const {
        return parent;
    }

    void setInheritFlags(transform_inherit_flags_t flags) { inherit_flags = flags; }

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

    gfxm::vec3 getWorldTranslation() const { return getWorldTransform()[3]; }
    gfxm::quat getWorldRotation() const { return gfxm::to_quat(gfxm::to_orient_mat3(getWorldTransform())); }
    gfxm::vec3 getWorldScale() const { 
        return gfxm::vec3(
            gfxm::vec3(getWorldTransform()[0]).length(),
            gfxm::vec3(getWorldTransform()[1]).length(),
            gfxm::vec3(getWorldTransform()[2]).length()
        );
    }

    gfxm::vec3 getWorldForward() const { return gfxm::normalize(getWorldTransform()[2]); }
    gfxm::vec3 getWorldBack() const { return gfxm::normalize(-getWorldTransform()[2]); }
    gfxm::vec3 getWorldLeft() const { return gfxm::normalize(-getWorldTransform()[0]); }
    gfxm::vec3 getWorldRight() const { return gfxm::normalize(getWorldTransform()[0]); }
    gfxm::vec3 getWorldUp() const { return gfxm::normalize(getWorldTransform()[1]); }
    gfxm::vec3 getWorldDown() const { return gfxm::normalize(-getWorldTransform()[1]); }

    gfxm::mat4 getLocalTransform() const;
    const gfxm::mat4& getWorldTransform() const;
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