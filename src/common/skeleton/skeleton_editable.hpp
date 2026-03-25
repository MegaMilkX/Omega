#pragma once

#include "skeleton_editable.auto.hpp"
#include <assert.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include "reflection/reflection.hpp"
#include "resource_manager/resource_manager.hpp"
#include "resource_manager/byte_writer/byte_writer.hpp"

#include "skeleton_bone.hpp"
#include "skeleton_instance.hpp"


class SkeletonInstance;
[[cppi_class]];
class Skeleton : public ILoadable, public IWritable/*, public HANDLE_ENABLE_FROM_THIS<Skeleton>*/ {
    TYPE_ENABLE()

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
    uint64_t                                fingerprint = 0;

    std::set<HSHARED<SkeletonInstance>>  instances;

    void rebuildBoneArray();
    uint64_t computeFingerprint() const;
public:
    Skeleton()
    : root(new sklBone(this, 0, "Root")) {
        rebuildBoneArray();
    }
    Skeleton(const Skeleton&) = delete;
    Skeleton& operator=(const Skeleton&) = delete;

    void clear();

    uint64_t getFingerprint() const { return fingerprint; }

    size_t boneCount() const { return bone_array.size(); }
    sklBone* getBone(int id) {
        if (id < 0 || id >= bone_array.size()) {
            assert(false);
            return 0;
        }
        return bone_array[id]; 
    }
    const sklBone* getBone(int id) const {
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

    void setBoneName(int idx, const char* name);
    void setRootName(const char* name);

    const std::vector<sklBone*>&    getBoneArray() const;
    const int*                      getParentArrayPtr() const;
    std::vector<gfxm::mat4>         makeLocalTransformArray() const;
    std::vector<gfxm::mat4>         makeWorldTransformArray() const;

    HSHARED<SkeletonInstance>       createInstance();
    void                            destroyInstance(HSHARED<SkeletonInstance> inst);

    bool merge(Skeleton& other);


    void dbgLog();

    DEFINE_EXTENSIONS(e_skl, e_skeleton);
    bool load_json(byte_reader& in); // TODO: to be removed, convert old skeleton files to skl first
    bool load(byte_reader& in) override;
    void write(const std::string& path) const;
    void write(byte_writer& out) const override;

    [[cppi_decl, serialize_json]]
    void toJson(nlohmann::json& j);
};