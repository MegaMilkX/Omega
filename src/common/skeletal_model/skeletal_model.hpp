#pragma once

#include "skeletal_model.auto.hpp"
#include <set>
#include <unordered_map>

#include "skeleton/skeleton_editable.hpp"
#include "gpu/gpu_mesh.hpp"
#include "gpu/gpu_material.hpp"

#include "render_scene/render_scene.hpp"
#include "render_scene/render_object/scn_mesh_object.hpp"
#include "render_scene/render_object/scn_skin.hpp"
#include "world/common_systems/scene_system.hpp"
#include "skeletal_model_instance.hpp"

#include "animation/model_sequence/model_sequence.hpp"

#include "resource_manager/resource_manager.hpp"

class SkeletalModel;
class sklmComponent {
    friend SkeletalModel;
protected:
    const size_t instance_data_size;
    const size_t anim_sample_size;
public:
    TYPE_ENABLE();

    sklmComponent(size_t instance_data_size, size_t anim_sample_size)
        : instance_data_size(instance_data_size),
        anim_sample_size(anim_sample_size) {}
private:
    std::string name;
    size_t prop_seq_sample_buf_offset = 0;
    size_t instance_data_offset = 0;

    virtual void _constructInstanceData(void* instance_data_ptr, SkeletonInstance* skl_inst) = 0;
    virtual void _destroyInstanceData(void* instance_data_ptr) = 0;
    virtual void _onSpawnInstance(void* instance_data_ptr, scnRenderScene* scn) = 0;
    virtual void _onDespawnInstance(void* instance_data_ptr, scnRenderScene* scn) = 0;

    size_t _getInstanceDataSize() { return instance_data_size; }
    size_t _getAnimSampleSize() { return anim_sample_size; }
    virtual void _applyAnimSample(void* instance_data_ptr, void* sample_ptr) {}
    virtual void _enableTechnique(void* instance_data_ptr, const char* path, bool value) {}
    virtual void _setParam(void* instance_data_ptr, const char* param_name, GPU_TYPE type, const void* pvalue) {}
    virtual void _submit(void* instance_data_ptr, gpuRenderBucket* bucket) = 0;

public:
    virtual ~sklmComponent() {}

    void setName(const char* name) { this->name = name; }
    const std::string& getName() const { return name; }

    size_t getAnimSampleBufOffset() const { return prop_seq_sample_buf_offset; }
};
template<typename INSTANCE_DATA_T>
class sklmComponentT : public sklmComponent {

    void _constructInstanceData(void* instance_data_ptr, SkeletonInstance* skl_inst) override {
        INSTANCE_DATA_T* ptr = (INSTANCE_DATA_T*)instance_data_ptr;
        new (ptr)(INSTANCE_DATA_T)();
        onConstructInstance(ptr, skl_inst);
    }
    void _destroyInstanceData(void* instance_data_ptr) override {
        INSTANCE_DATA_T* ptr = (INSTANCE_DATA_T*)instance_data_ptr;
        onDestroyInstance(ptr);
        ptr->~INSTANCE_DATA_T();
    }
    void _onSpawnInstance(void* instance_data_ptr, scnRenderScene* scn) override {
        INSTANCE_DATA_T* ptr = (INSTANCE_DATA_T*)instance_data_ptr;
        onSpawnInstance(ptr, scn);
    }
    void _onDespawnInstance(void* instance_data_ptr, scnRenderScene* scn) override {
        INSTANCE_DATA_T* ptr = (INSTANCE_DATA_T*)instance_data_ptr;
        onDespawnInstance(ptr, scn);
    }

    void _enableTechnique(void* instance_data_ptr, const char* path, bool value) override {
        INSTANCE_DATA_T* i = (INSTANCE_DATA_T*)instance_data_ptr;
        onEnableTechnique(i, path, value);
    }
    void _setParam(void* instance_data_ptr, const char* param_name, GPU_TYPE type, const void* pvalue) override {
        INSTANCE_DATA_T* i = (INSTANCE_DATA_T*)instance_data_ptr;
        onSetParam(i, param_name, type, pvalue);
    }
    void _submit(void* instance_data_ptr, gpuRenderBucket* bucket) override {
        INSTANCE_DATA_T* i = (INSTANCE_DATA_T*)instance_data_ptr;
        onSubmit(i, bucket);
    }

public:
    TYPE_ENABLE();
    sklmComponentT(size_t anim_sample_size = 0) : sklmComponent(sizeof(INSTANCE_DATA_T), anim_sample_size) {}
    virtual void onConstructInstance(INSTANCE_DATA_T* instance_data, SkeletonInstance* skl_inst) {}
    virtual void onDestroyInstance(INSTANCE_DATA_T* instance_data) {}
    virtual void onSpawnInstance(INSTANCE_DATA_T* instance_data, scnRenderScene* scn) = 0;
    virtual void onDespawnInstance(INSTANCE_DATA_T* instance_data, scnRenderScene* scn) = 0;

    virtual void onEnableTechnique(INSTANCE_DATA_T* instance_data, const char* path, bool value) {}
    virtual void onSetParam(INSTANCE_DATA_T* instance_data, const char* param_name, GPU_TYPE type, const void* pvalue) {}
    virtual void onSubmit(INSTANCE_DATA_T* instance_data, gpuRenderBucket* bucket) = 0;
};
template<typename INSTANCE_DATA_T, typename ANIM_SAMPLE_T>
class sklmComponentAnimT : public sklmComponentT<INSTANCE_DATA_T> {
    void _applyAnimSample(void* inst, void* sample_ptr) {
        INSTANCE_DATA_T* i = (INSTANCE_DATA_T*)inst;
        ANIM_SAMPLE_T* p = (ANIM_SAMPLE_T*)sample_ptr;
        onApplyAnimSample(i, p);
    }
public:
    TYPE_ENABLE();
    sklmComponentAnimT() : sklmComponentT<INSTANCE_DATA_T>(sizeof(ANIM_SAMPLE_T)) {}
    virtual void onApplyAnimSample(INSTANCE_DATA_T* inst, ANIM_SAMPLE_T* sample) = 0;
};

struct sklmMeshInstance {
    scnMeshObject scn_msh; // TODO: to be removed
    gpuRenderable renderable;
    HTransform bone_node;
    gpuTransformBlock* transform_block = nullptr;
};

class sklmMeshComponent final : public sklmComponentT<sklmMeshInstance> {
    void onConstructInstance(sklmMeshInstance* inst, SkeletonInstance* skl_inst) override {
        auto bone_node = skl_inst->getBoneNode(bone_name.c_str());
        // Old
        inst->scn_msh.setMeshDesc(mesh->getMeshDesc());
        inst->scn_msh.setMaterial(material.get());
        inst->scn_msh.setTransformNode(bone_node);
        inst->scn_msh.getRenderable(0)->setRole(GPU_Role_Geometry);
        // New
        auto& renderable = inst->renderable;
        renderable.setRole(GPU_Role_Geometry);
        renderable.setMeshDesc(mesh->getMeshDesc());
        renderable.setMaterial(material.get());
        inst->bone_node = bone_node;
        inst->transform_block = skl_inst->getTransformBlock(bone_name.c_str());
        inst->transform_block->markTeleported();
        renderable.attachParamBlock(inst->transform_block);
        renderable.compile();
    }
    void onDestroyInstance(sklmMeshInstance* inst) override {
        // TODO: REMOVE RENDERABLE FROM RENDER GROUP
    }
    void onSpawnInstance(sklmMeshInstance* inst, scnRenderScene* scn) override {
        //scn->addRenderObject(&inst->scn_msh);
    }
    void onDespawnInstance(sklmMeshInstance* inst, scnRenderScene* scn) override {
        //scn->removeRenderObject(&inst->scn_msh);
    }

    void onEnableTechnique(sklmMeshInstance* inst, const char* path, bool value) override {
        // Old
        for (int i = 0; i < inst->scn_msh.renderableCount(); ++i) {
            auto r = inst->scn_msh.getRenderable(i);
            r->enableMaterialTechnique(path, value);
        }
        // New
        inst->renderable.enableMaterialTechnique(path, value);
    }
    void onSetParam(sklmMeshInstance* inst, const char* param_name, GPU_TYPE type, const void* pvalue) override {
        // Old
        for (int i = 0; i < inst->scn_msh.renderableCount(); ++i) {
            auto r = inst->scn_msh.getRenderable(i);
            r->setParam(param_name, type, pvalue);
        }
        // New
        inst->renderable.setParam(param_name, type, pvalue);
    }
    void onSubmit(sklmMeshInstance* inst, gpuRenderBucket* bucket) override {
        bucket->add(&inst->renderable);
    }

public:
    TYPE_ENABLE();
    std::string             bone_name;
    HSHARED<gpuMesh>        mesh;
    HSHARED<gpuMaterial>    material;

    static void reflect();
};
class sklmSkinComponent final : public sklmComponentT<scnSkin> {

    void onConstructInstance(scnSkin* scn_skn, SkeletonInstance* skl_inst) {
        scn_skn->setSourceMeshDesc(mesh->getMeshDesc());
        scn_skn->setMaterial(material.get());
        scn_skn->setSkeletonInstance(skl_inst);
        std::vector<int> bone_indices(bone_names.size(), 0);
        for (int i = 0; i < bone_names.size(); ++i) {
            auto& bone_name = bone_names[i];
            bone_indices[i] = skl_inst->findBoneIndex(bone_name.c_str());
        }
        scn_skn->setBoneIndices(bone_indices.data(), bone_indices.size());
        scn_skn->setInverseBindTransforms(inv_bind_transforms.data(), inv_bind_transforms.size());
        scn_skn->getRenderable(0)->setRole(GPU_Role_Geometry);
    }
    void onSpawnInstance(scnSkin* scn_skn, scnRenderScene* scn) override {
        scn->addRenderObject(scn_skn);
    }
    void onDespawnInstance(scnSkin* scn_skn, scnRenderScene* scn) override {
        scn->removeRenderObject(scn_skn);
    }

    void onEnableTechnique(scnSkin* scn_skn, const char* path, bool value) override {
        for (int i = 0; i < scn_skn->renderableCount(); ++i) {
            auto r = scn_skn->getRenderable(i);
            r->enableMaterialTechnique(path, value);
        }
    }
    void onSetParam(scnSkin* scn_skn, const char* param_name, GPU_TYPE type, const void* pvalue) override {
        for (int i = 0; i < scn_skn->renderableCount(); ++i) {
            auto r = scn_skn->getRenderable(i);
            r->setParam(param_name, type, pvalue);
        }
    }
    void onSubmit(scnSkin* inst, gpuRenderBucket* bucket) override {
        // TODO:
        //bucket->add(&inst->renderable);
    }

public:
    TYPE_ENABLE();
    std::vector<std::string> bone_names;
    std::vector<gfxm::mat4> inv_bind_transforms; // TODO: Move it to a gpuSkin somehow, no gpuMesh
    HSHARED<gpuMesh>      mesh;
    HSHARED<gpuMaterial>  material;

    static void reflect();
};
class sklmDecalComponent final : public sklmComponentAnimT<scnDecal, animDecalSample> {

    void onConstructInstance(scnDecal* decal, SkeletonInstance* skl_inst) {
        /*decal->setSkeletonNode(
            skl_inst->getScnSkeleton(),
            skl_inst->findBoneIndex(bone_name.c_str())
        );*/
        decal->setTransformNode(skl_inst->getBoneNode(bone_name.c_str()));
        decal->setBoxSize(gfxm::vec3(1, 0.1, 1));
        decal->setMaterial(material);
        decal->getRenderable(0)->setRole(GPU_Role_Decal);
    }
    void onSpawnInstance(scnDecal* decal, scnRenderScene* scn) override {
        scn->addRenderObject(decal);
    }
    void onDespawnInstance(scnDecal* decal, scnRenderScene* scn) override {
        scn->removeRenderObject(decal);
    }

    void onApplyAnimSample(scnDecal* decal, animDecalSample* s) {
        decal->setColor(s->rgba);
    }

    void onEnableTechnique(scnDecal* decal, const char* path, bool value) override {
        for (int i = 0; i < decal->renderableCount(); ++i) {
            auto r = decal->getRenderable(i);
            r->enableMaterialTechnique(path, value);
        }
    }
    void onSetParam(scnDecal* decal, const char* param_name, GPU_TYPE type, const void* pvalue) override {
        for (int i = 0; i < decal->renderableCount(); ++i) {
            auto r = decal->getRenderable(i);
            r->setParam(param_name, type, pvalue);
        }
    }
    void onSubmit(scnDecal* inst, gpuRenderBucket* bucket) override {
        // TODO:
        //bucket->add(&inst->renderable);
    }

public:
    TYPE_ENABLE();
    std::string             bone_name;
    RHSHARED<gpuMaterial>   material;
};


#include "animation/model_sequence/model_sequence_sample_buffer.hpp"


[[cppi_decl, no_reflect]];
class SkeletalModel;

class SkeletalModel : public ILoadable {
    ResourceRef<Skeleton>                           skeleton;
    std::vector<std::unique_ptr<sklmComponent>>     components;

    std::set<HSHARED<SkeletalModelInstance>>     instances;

public:
    TYPE_ENABLE();

    SkeletalModel();
    SkeletalModel(const SkeletalModel&) = delete;
    SkeletalModel& operator=(const SkeletalModel&) = delete;

    void setSkeleton(ResourceRef<Skeleton> skeleton) {
        this->skeleton = skeleton;
    }
    ResourceRef<Skeleton> getSkeleton() {
        return skeleton;
    }

    template<typename T>
    T* addComponent(const char* name) {
        T* ptr = new T();
        ptr->setName(name);
        size_t prop_seq_sample_buf_offset = 0;
        if (!components.empty()) {
            prop_seq_sample_buf_offset
                = components.back()->prop_seq_sample_buf_offset
                + components.back()->_getAnimSampleSize();
        }
        ptr->prop_seq_sample_buf_offset = prop_seq_sample_buf_offset;
        components.push_back(std::unique_ptr<T>(ptr));
        return ptr;
    }
    sklmComponent* findComponent(const char* name) {
        for (auto& c : components) {
            if (c->getName() == name) {
                return c.get();
            }
        }
        return 0;
    }

    HSHARED<SkeletalModelInstance> createInstance();
    HSHARED<SkeletalModelInstance> createInstance(HSHARED<SkeletonInstance>& skl_inst);
    // Do not call destroyInstance(). Instances call it in their destructor
    void destroyInstance(SkeletalModelInstance* mdl_inst);
    void spawnInstance(SkeletalModelInstance* mdl_inst, scnRenderScene* scn);
    void despawnInstance(SkeletalModelInstance* mdl_inst, scnRenderScene* scn);
    void initSampleBuffer(animModelSampleBuffer& buf);
    void applySampleBuffer(SkeletalModelInstance* mdl_inst, animModelSampleBuffer& buf);

    void enableTechnique(SkeletalModelInstance* mdl_inst, const char* path, bool value);
    void setParam(SkeletalModelInstance* mdl_inst, const char* param_name, GPU_TYPE type, const void* pvalue);
    void submit(SkeletalModelInstance* mdl_inst, gpuRenderBucket* bucket);

    void dbgLog();

    DEFINE_EXTENSIONS(e_skeletal_model);
    bool load(byte_reader& reader) override;

    static void reflect();
};
inline RHSHARED<SkeletalModel> getSkeletalModel(const char* path) {
    return resGet<SkeletalModel>(path);
}

inline void animMakeModelAnimMapping(animModelAnimMapping* mapping, SkeletalModel* model, animModelSequence* seq) {
    mapping->resize(seq->nodeCount());
    for (int i = 0; i < seq->nodeCount(); ++i) {
        auto n = seq->getNode(i);
        auto component = model->findComponent(n->name.c_str());
        if (!component) {
            mapping->operator[](i) = -1;
            continue;
        }
        mapping->operator[](i) = component->getAnimSampleBufOffset();
    }
}
