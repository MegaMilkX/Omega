#pragma once

#include <vector>
#include <map>
#include "handle/hshared.hpp"
#include "math/gfxm.hpp"
#include "transform_node/transform_node.hpp"
#include "gpu/param_block/transform_block.hpp"


class Skeleton;


class SkeletonInstance {
    friend Skeleton;

    Skeleton*            prototype = 0;

    std::vector<Handle<TransformNode>> bone_nodes;
    std::map<int, gpuTransformBlock*> transform_blocks;
    bool is_valid = true;
public:
    SkeletonInstance();
    ~SkeletonInstance();

    Skeleton* getSkeletonMaster() { return prototype; }

    const int*  getParentArrayPtr();
    Handle<TransformNode> getBoneNode(int i) { return bone_nodes[i]; }
    Handle<TransformNode> getBoneNode(const char* name);
    gpuTransformBlock* getTransformBlock(const char* name);
    gpuTransformBlock* getTransformBlock(int bone_idx);

    void setExternalRootTransform(Handle<TransformNode> node);

    int findBoneIndex(const char* name) const;

    static void reflect();
};
