#pragma once

#include <set>
#include <unordered_map>

#include "skeleton/skeleton_editable.hpp"
#include "gpu/gpu_mesh.hpp"
#include "gpu/gpu_material.hpp"

#include "render_scene/render_scene.hpp"
#include "render_scene/render_object/scn_mesh_object.hpp"
#include "render_scene/render_object/scn_skin.hpp"
#include "skeletal_model_instance.hpp"

#include "animation/model_sequence/model_sequence.hpp"

class mdlSkeletalModelMaster;
class sklmComponent {
    friend mdlSkeletalModelMaster;
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

public:
    TYPE_ENABLE();
    sklmComponentT(size_t anim_sample_size = 0) : sklmComponent(sizeof(INSTANCE_DATA_T), anim_sample_size) {}
    virtual void onConstructInstance(INSTANCE_DATA_T* instance_data, SkeletonInstance* skl_inst) {}
    virtual void onDestroyInstance(INSTANCE_DATA_T* instance_data) {}
    virtual void onSpawnInstance(INSTANCE_DATA_T* instance_data, scnRenderScene* scn) = 0;
    virtual void onDespawnInstance(INSTANCE_DATA_T* instance_data, scnRenderScene* scn) = 0;

    virtual void onEnableTechnique(INSTANCE_DATA_T* instance_data, const char* path, bool value) {}
    virtual void onSetParam(INSTANCE_DATA_T* instance_data, const char* param_name, GPU_TYPE type, const void* pvalue) {}
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

class sklmMeshComponent : public sklmComponentT<scnMeshObject> {

    void onConstructInstance(scnMeshObject* scn_msh, SkeletonInstance* skl_inst) override {
        scn_msh->setMeshDesc(mesh->getMeshDesc());
        scn_msh->setMaterial(material.get());
        scn_msh->setTransformNode(skl_inst->getBoneNode(bone_name.c_str()));
        /*
        scn_msh->setSkeletonNode(
            skl_inst->getScnSkeleton(), skl_inst->findBoneIndex(bone_name.c_str())
        );*/
    }
    void onSpawnInstance(scnMeshObject* scn_msh, scnRenderScene* scn) override {
        scn->addRenderObject(scn_msh);
    }
    void onDespawnInstance(scnMeshObject* scn_msh, scnRenderScene* scn) override {
        scn->removeRenderObject(scn_msh);
    }

    void onEnableTechnique(scnMeshObject* scn_msh, const char* path, bool value) override {
        for (int i = 0; i < scn_msh->renderableCount(); ++i) {
            auto r = scn_msh->getRenderable(i);
            r->enableMaterialTechnique(path, value);
        }
    }
    void onSetParam(scnMeshObject* scn_msh, const char* param_name, GPU_TYPE type, const void* pvalue) override {
        for (int i = 0; i < scn_msh->renderableCount(); ++i) {
            auto r = scn_msh->getRenderable(i);
            r->setParam(param_name, type, pvalue);
        }
    }

public:
    TYPE_ENABLE();
    std::string             bone_name;
    HSHARED<gpuMesh>        mesh;
    HSHARED<gpuMaterial>    material;

    static void reflect();
};
class sklmSkinComponent : public sklmComponentT<scnSkin> {

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

public:
    TYPE_ENABLE();
    std::vector<std::string> bone_names;
    std::vector<gfxm::mat4> inv_bind_transforms; // TODO: Move it to a gpuSkin somehow, no gpuMesh
    HSHARED<gpuMesh>      mesh;
    HSHARED<gpuMaterial>  material;

    static void reflect();
};
class sklmDecalComponent : public sklmComponentAnimT<scnDecal, animDecalSample> {

    void onConstructInstance(scnDecal* decal, SkeletonInstance* skl_inst) {
        /*decal->setSkeletonNode(
            skl_inst->getScnSkeleton(),
            skl_inst->findBoneIndex(bone_name.c_str())
        );*/
        decal->setTransformNode(skl_inst->getBoneNode(bone_name.c_str()));
        decal->setBoxSize(gfxm::vec3(1, 0.1, 1));
        decal->setTexture(texture);
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

public:
    TYPE_ENABLE();
    std::string             bone_name;
    RHSHARED<gpuTexture2d>  texture;
};


#include "animation/model_sequence/model_sequence_sample_buffer.hpp"



class mdlSkeletalModelMaster {
    RHSHARED<Skeleton>                              skeleton;
    std::vector<std::unique_ptr<sklmComponent>>     components;

    std::set<HSHARED<mdlSkeletalModelInstance>>     instances;

public:
    TYPE_ENABLE();

    mdlSkeletalModelMaster();

    void setSkeleton(RHSHARED<Skeleton> skeleton) {
        this->skeleton = skeleton;
    }
    RHSHARED<Skeleton> getSkeleton() {
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

    HSHARED<mdlSkeletalModelInstance> createInstance();
    HSHARED<mdlSkeletalModelInstance> createInstance(HSHARED<SkeletonInstance>& skl_inst);
    // Do not call destroyInstance(). Instances call it in their destructor
    void destroyInstance(mdlSkeletalModelInstance* mdl_inst);
    void spawnInstance(mdlSkeletalModelInstance* mdl_inst, scnRenderScene* scn);
    void despawnInstance(mdlSkeletalModelInstance* mdl_inst, scnRenderScene* scn);
    void initSampleBuffer(animModelSampleBuffer& buf);
    void applySampleBuffer(mdlSkeletalModelInstance* mdl_inst, animModelSampleBuffer& buf);

    void enableTechnique(mdlSkeletalModelInstance* mdl_inst, const char* path, bool value);
    void setParam(mdlSkeletalModelInstance* mdl_inst, const char* param_name, GPU_TYPE type, const void* pvalue);

    void dbgLog();

    static void reflect();
};
inline RHSHARED<mdlSkeletalModelMaster> getSkeletalModel(const char* path) {
    return resGet<mdlSkeletalModelMaster>(path);
}

inline void animMakeModelAnimMapping(animModelAnimMapping* mapping, mdlSkeletalModelMaster* model, animModelSequence* seq) {
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
