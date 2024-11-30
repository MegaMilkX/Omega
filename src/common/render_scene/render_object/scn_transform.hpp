#pragma once

#include <vector>
#include "math/gfxm.hpp"
#include "transform_node/transform_node.hpp"

/*
enum class SCN_TRANSFORM_SOURCE {
    NONE,
    NODE,
    SKELETON
};*/
/*
struct scnSkeleton {
    //const int*          parents;
    //const gfxm::mat4*   local_transforms;
    //gfxm::mat4*         world_transforms;
    Handle<TransformNode>* bone_nodes;
    size_t              bone_count;
};*/
/*
struct scnNode {
    SCN_TRANSFORM_SOURCE transform_source = SCN_TRANSFORM_SOURCE::NONE;
    union {
        scnNode* node = 0;
        struct {
            scnSkeleton* skeleton;
            uint32_t bone_id;
        };
    };

    gfxm::mat4 local_transform = gfxm::mat4(1.0f);
    gfxm::mat4 world_transform = gfxm::mat4(1.0f);

    void attachToSkeleton(scnSkeleton* skel, int bone) {
        if (!skel) {
            transform_source = SCN_TRANSFORM_SOURCE::NONE;
            return;
        }
        transform_source = SCN_TRANSFORM_SOURCE::SKELETON;
        skeleton = skel;
        bone_id = bone;
    }
    void attachToNode(scnNode* node) {
        if (!node) {
            transform_source = SCN_TRANSFORM_SOURCE::NONE;
            return;
        }
        transform_source = SCN_TRANSFORM_SOURCE::NODE;
        this->node = node;
    }
};*/
