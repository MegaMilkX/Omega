#pragma once

#include <vector>
#include "handle/hshared.hpp"
#include "math/gfxm.hpp"
#include "transform_node/transform_node.hpp"


class Skeleton;


class SkeletonInstance {
    friend Skeleton;

    Skeleton*            prototype = 0;

    std::vector<Handle<TransformNode>> bone_nodes;
    bool is_valid = true;
public:
    SkeletonInstance();
    ~SkeletonInstance() {}

    Skeleton* getSkeletonMaster() { return prototype; }

    const int*  getParentArrayPtr();
    Handle<TransformNode> getBoneNode(int i) { return bone_nodes[i]; }
    Handle<TransformNode> getBoneNode(const char* name);

    void setExternalRootTransform(Handle<TransformNode> node);

    int findBoneIndex(const char* name) const;

    static void reflect();
};
