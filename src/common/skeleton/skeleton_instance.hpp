#pragma once

#include <vector>
#include "handle/hshared.hpp"
#include "math/gfxm.hpp"
#include "render_scene/render_scene.hpp"


class sklSkeletonEditable;
class sklSkeletonPrototype;


class sklSkeletonInstance {
    friend sklSkeletonEditable;
    friend sklSkeletonPrototype;

    sklSkeletonEditable*            prototype = 0;

    std::unique_ptr<scnSkeleton>    scn_skel;
    gfxm::mat4*                     local_transforms = 0;
    gfxm::mat4*                     world_transforms = 0;
    
    // Keep track of spawn requests to avoid spawning more than once
    int                             spawn_count = 0;

public:
    sklSkeletonInstance();
    ~sklSkeletonInstance() {
        delete[] local_transforms;
        delete[] world_transforms;
    }

    const int*  getParentArrayPtr();
    gfxm::mat4* getLocalTransformsPtr() { return local_transforms; }
    gfxm::mat4* getWorldTransformsPtr() { return world_transforms; }

    scnSkeleton* getScnSkeleton() { return scn_skel.get(); }

    void onSpawn(scnRenderScene* scn) {
        if (spawn_count++ > 0) {
            return;
        }
        scn->addSkeleton(scn_skel.get());
    }
    void onDespawn(scnRenderScene* scn) {
        if (--spawn_count > 0) {
            return;
        }
        scn->removeSkeleton(scn_skel.get());
    }

    static void reflect();
};
