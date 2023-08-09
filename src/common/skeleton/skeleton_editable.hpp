#pragma once

#include <assert.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include "reflection/reflection.hpp"

#include "skeleton_bone.hpp"
#include "skeleton_dependant.hpp"
#include "skeleton_instance.hpp"


class SkeletonPose;
class Skeleton : public HANDLE_ENABLE_FROM_THIS<Skeleton> {
    TYPE_ENABLE_BASE()

    friend sklBone;

    struct BoneDeserialized {
        std::string name;
        std::string parent_name;
        int         index;
        int         parent_index;
        gfxm::vec3  translation;
        gfxm::quat  rotation;
        gfxm::vec3  scale;
    };

    std::unique_ptr<sklBone>                root;
    std::vector<sklBone*>                   bone_array;
    std::vector<int>                        parent_array;
    std::unordered_map<std::string, int>    name_to_index;

    std::set<sklSkeletonDependant*>         dependants;

    std::set<HSHARED<SkeletonPose>>  instances;

    void rebuildBoneArray();

public:
    Skeleton()
    : root(new sklBone(this, 0, "Root")) {
        rebuildBoneArray();
    }

    void clear();

    size_t boneCount() const { return bone_array.size(); }
    sklBone* getBone(int id) {
        if (id < 0 || id >= bone_array.size()) {
            assert(false);
            return 0;
        }
        return bone_array[id]; 
    }
    sklBone* findBone(const char* name) const {
        auto it = name_to_index.find(name);
        if (it == name_to_index.end()) {
            assert(false);
            return 0;
        }
        return bone_array[it->second];
    }

    sklBone* getRoot() { return root.get(); }


    const std::vector<sklBone*>&    getBoneArray() const;
    const int*                      getParentArrayPtr() const;
    std::vector<gfxm::mat4>         makeLocalTransformArray() const;
    std::vector<gfxm::mat4>         makeWorldTransformArray() const;

    HSHARED<SkeletonPose>    createInstance();
    void                            destroyInstance(HSHARED<SkeletonPose> inst);

    bool merge(Skeleton& other);


    void addDependant(sklSkeletonDependant* dep);
    void removeDependant(sklSkeletonDependant* dep);


    void dbgLog();

    static void reflect();
};
inline RHSHARED<Skeleton> getSkeleton(const char* path) {
    return resGet<Skeleton>(path);
}