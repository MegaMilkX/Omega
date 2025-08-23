#include "aabb_tree.hpp"

#include "collision/collider.hpp"


static bool aabbTreeTestOverlap(const gfxm::aabb &a, const gfxm::aabb &b) {
    float ox = gfxm::_min(a.to.x, b.to.x) - gfxm::_max(a.from.x, b.from.x);
    float oy = gfxm::_min(a.to.y, b.to.y) - gfxm::_max(a.from.y, b.from.y);
    float oz = gfxm::_min(a.to.z, b.to.z) - gfxm::_max(a.from.z, b.from.z);
    return !(ox < 0.0f || oy < 0.0f || oz < 0.0f);
}

void AabbTreeNode::forEachOverlap(const gfxm::aabb& box, const AABB_TREE_OVERLAP_CALLBACK_T& callback) {
    if (!aabbTreeTestOverlap(box, aabb)) {
        return;
    }
    if (isLeaf()) {
        if (elem->collider) {
            // No need to test collider's aabb, it's exactly the same as this node's
            callback(elem->collider);
        }
    } else {
        left->forEachOverlap(box, callback);
        right->forEachOverlap(box, callback);
    }
}

