#include "transform_node.hpp"


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


gfxm::mat4 TransformNode::getLocalTransform() {
    return gfxm::translate(gfxm::mat4(1.0f), translation)
        * gfxm::to_mat4(rotation);
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