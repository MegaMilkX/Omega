#include "transform_node.hpp"


int TransformNode::addDirtyCallback(pfn_transform_callback_t cb, void* context) {
    static int next_dirty_callback_id = 0;

    std::unique_ptr<TransformCallback>* upptr = &dirty_callback;
    while (upptr->get() != 0) {
        upptr = &upptr->get()->next;
    }

    upptr->reset(new TransformCallback);
    std::unique_ptr<TransformCallback>& uptr = *upptr;
    uptr->callback = cb;
    uptr->id = next_dirty_callback_id++;
    uptr->next = 0;
    uptr->context = context;

    return uptr->id;
}

void TransformNode::removeDirtyCallback(int id) {
    std::unique_ptr<TransformCallback>* upptr = &dirty_callback;
    while (upptr->get() && (*upptr)->id != id) {
        upptr = &upptr->get()->next;
    }
    if (*upptr) {
        (*upptr) = std::move((*upptr)->next);
    }
}


void TransformNode::translate(float x, float y, float z) {
    translate(gfxm::vec3(x, y, z));
}
void TransformNode::translate(const gfxm::vec3& t) {
    translation += t;
    dirty();
}
void TransformNode::rotate(float angle, const gfxm::vec3& axis) {
    rotation = gfxm::angle_axis(angle, axis) * rotation;
    dirty();
}
void TransformNode::rotate(const gfxm::quat& q) {
    rotation = q * rotation;
    dirty();
}

void TransformNode::setTranslation(float x, float y, float z) {
    setTranslation(gfxm::vec3(x, y, z));
}
void TransformNode::setTranslation(const gfxm::vec3& t) {
    translation = t;
    dirty();
}
void TransformNode::setRotation(const gfxm::quat& q) {
    rotation = q;
    dirty();
}
void TransformNode::setScale(const gfxm::vec3& s) {
    scale = s;
    dirty();
}


gfxm::mat4 TransformNode::getLocalTransform() {
    return gfxm::translate(gfxm::mat4(1.0f), translation)
        * gfxm::to_mat4(rotation)
        * gfxm::scale(gfxm::mat4(1.0f), scale);
}
const gfxm::mat4& TransformNode::getWorldTransform() {
    if (!dirty_) {
        return world_transform;
    } else {
        if (parent.isValid()) {
            world_transform = parent->getWorldTransform() * getLocalTransform();
        } else {
            world_transform = getLocalTransform();
        }
        dirty_ = false;
        return world_transform;
    }
}