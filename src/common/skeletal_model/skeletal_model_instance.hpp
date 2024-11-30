#pragma once

#include <memory>
#include <map>
#include "transform_node/transform_node.hpp"
#include "skeleton/skeleton_instance.hpp"
#include "render_scene/render_scene.hpp"

class mdlSkeletalModelMaster;

struct animModelSampleBuffer;

class mdlSkeletalModelInstance {
    friend mdlSkeletalModelMaster;
public:
    // This struct exists to allow model components to modify an instance during
    // instantiation without friending every possible component type
    // TODO: No longer relevant?
    struct InstanceData {
        HSHARED<SkeletonInstance>    skeleton_instance;
        std::vector<char>               instance_data_bytes;
    };
private:
    struct BoneProxy {
        int bone_idx;
        HSHARED<TransformNode> proxy;
    };

    mdlSkeletalModelMaster* prototype = 0;
    InstanceData instance_data;

public:
    ~mdlSkeletalModelInstance();
    SkeletonInstance* getSkeletonInstance() {
        if (!instance_data.skeleton_instance) {
            assert(false);
            return 0;
        }
        return instance_data.skeleton_instance.get();
    }
    Skeleton* getSkeletonMaster() {
        auto inst = getSkeletonInstance();
        if (!inst) {
            return 0;
        }
        return inst->getSkeletonMaster();
    }

    Handle<TransformNode> getBoneProxy(const std::string& name);
    Handle<TransformNode> getBoneProxy(int idx);

    void setExternalRootTransform(Handle<TransformNode> node);

    void applySampleBuffer(animModelSampleBuffer& buf);
    
    void updateWorldTransform(const gfxm::mat4& world);

    void spawn(scnRenderScene* scn);
    void despawn(scnRenderScene* scn);
};