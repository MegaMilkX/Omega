#pragma once

#include <memory>
#include <map>
#include "transform_node/transform_node.hpp"
#include "skeleton/skeleton_instance.hpp"
#include "render_scene/render_scene.hpp"
#include "world/common_systems/scene_system.hpp"

class SkeletalModelInstance;
class SkeletalModelSceneProxy : public SceneProxy {
    SkeletalModelInstance* model = nullptr;
public:
    void setModel(SkeletalModelInstance* mdl) {
        model = mdl;
        markDirty();
    }
    void clearModel() {
        model = nullptr;
        markDirty();
    }
    void updateBounds() override {
        auto node = getTransformNode();
        setBoundingSphere(.25f, node->getWorldTranslation());
        setBoundingBox(gfxm::aabb(
            node->getWorldTranslation() - gfxm::vec3(.25, .25, .25),
            node->getWorldTranslation() + gfxm::vec3(.25, .25, .25)
        ));
    }
    void submit(gpuRenderBucket* bucket) override;
};

class SkeletalModel;

struct animModelSampleBuffer;

class SkeletalModelInstance {
    friend SkeletalModel;
public:
    // This struct exists to allow model components to modify an instance during
    // instantiation without friending every possible component type
    // TODO: No longer relevant?
    struct InstanceData {
        HSHARED<SkeletonInstance>    skeleton_instance;
        std::vector<char>               instance_data_bytes;
    };
private:
    SkeletalModel* prototype = 0;
    InstanceData instance_data;
    SkeletalModelSceneProxy vis_proxy;

public:
    SkeletalModelInstance();
    ~SkeletalModelInstance();

    void submit(gpuRenderBucket* bucket);

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

    void enableTechnique(const char* path, bool value);
    void setParam(const char* param_name, GPU_TYPE type, const void* pvalue);

    void setExternalRootTransform(Handle<TransformNode> node);

    void applySampleBuffer(animModelSampleBuffer& buf);
    
    void updateWorldTransform(const gfxm::mat4& world);

    void spawnModel(SceneSystem* scene_sys, scnRenderScene* scn);
    void despawnModel(SceneSystem* scene_sys, scnRenderScene* scn);
};

inline void SkeletalModelSceneProxy::submit(gpuRenderBucket* bucket) {
    if (!model) {
        return;
    }
    model->submit(bucket);
}