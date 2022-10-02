#pragma once

#include "skeleton/skeleton_editable.hpp"
#include "skeletal_model/skeletal_model.hpp"
#include "animation/animation.hpp"
#include "collision/collision_triangle_mesh.hpp"

#include <assimp/scene.h>

struct assimpLoadedResources {
    std::vector<std::string>           material_names;
    std::vector<RHSHARED<gpuMaterial>> materials;
    std::vector<RHSHARED<Animation>>   animations;
};


class assimpImporter {
    const aiScene* ai_scene = 0;
    double fbxScaleFactor = 1.0f;

    HSHARED<sklSkeletonEditable> skeleton;

public:
    assimpImporter();
    ~assimpImporter();
    bool loadFile(const char* fname);

    bool loadSkeletalModel(mdlSkeletalModelMaster* sklm, assimpLoadedResources* resources = 0);
    bool loadAnimation(Animation* anim, const char* track_name, int frame_start = 0, int frame_end = -1, const char* root_motion_bone = 0);
    
    bool loadCollisionTriangleMesh(CollisionTriangleMesh* trimesh);
};
