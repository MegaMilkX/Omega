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

class sklmSkeletalModelEditable;
class sklmComponent {
    friend sklmSkeletalModelEditable;
public:
    TYPE_ENABLE_BASE();
private:
    std::string name;

    virtual void _appendInstance(sklmSkeletalModelInstance::InstanceData& inst, const sklSkeletonEditable* skel) = 0;

public:
    virtual ~sklmComponent() {}

    void setName(const char* name) { this->name = name; }
    const std::string& getName() const { return name; }
};
class sklmMeshComponent : public sklmComponent {
    TYPE_ENABLE(sklmComponent);

    void _appendInstance(sklmSkeletalModelInstance::InstanceData& inst, const sklSkeletonEditable* skel) override {
        auto scn_msh = new scnMeshObject;
        scn_msh->setMeshDesc(mesh->getMeshDesc());
        scn_msh->setMaterial(material.get());
        scn_msh->setSkeletonNode(inst.skeleton_instance->getScnSkeleton(), skel->findBone(bone_name.c_str())->getIndex());
        inst.render_objects.push_back(std::unique_ptr<scnMeshObject>(scn_msh));
    }

public:
    std::string             bone_name;
    HSHARED<gpuMesh>        mesh;
    HSHARED<gpuMaterial>    material;

    static void reflect();
};
class sklmSkinComponent : public sklmComponent {
    TYPE_ENABLE(sklmComponent);

    void _appendInstance(sklmSkeletalModelInstance::InstanceData& inst, const sklSkeletonEditable* skel) override {
        auto scn_skn = new scnSkin;
        scn_skn->setMeshDesc(mesh->getMeshDesc());
        scn_skn->setMaterial(material.get());
        scn_skn->setSkeleton(inst.skeleton_instance->getScnSkeleton());
        std::vector<int> bone_indices(bone_names.size(), 0);
        for (int i = 0; i < bone_names.size(); ++i) {
            auto& bone_name = bone_names[i];
            bone_indices[i] = skel->findBone(bone_name.c_str())->getIndex();
        }
        scn_skn->setBoneIndices(bone_indices.data(), bone_indices.size());
        scn_skn->setInverseBindTransforms(inv_bind_transforms.data(), inv_bind_transforms.size());
        inst.render_objects.push_back(std::unique_ptr<scnSkin>(scn_skn));
    }

public:
    std::vector<std::string> bone_names;
    std::vector<gfxm::mat4> inv_bind_transforms; // TODO: Move it to a gpuSkin somehow, no gpuMesh
    HSHARED<gpuMesh>      mesh;
    HSHARED<gpuMaterial>  material;

    static void reflect();
};

class sklmSkeletalModelEditable : public sklSkeletonDependant {
    std::vector<std::unique_ptr<sklmComponent>>     components;
    //std::set<HSHARED<gpuMesh>>                      meshes;
    //std::vector<HSHARED<gpuMaterial>>               materials;

    std::set<HSHARED<sklmSkeletalModelInstance>>    instances;

public:
    sklmSkeletalModelEditable() {
        setSkeleton(HSHARED<sklSkeletonEditable>(HANDLE_MGR<sklSkeletonEditable>::acquire()));
    }

    void onSkeletonSet(sklSkeletonEditable* skel) override {
        // TODO
    }
    void onSkeletonRemoved(sklSkeletonEditable* skel) override {
        // TODO
    }
    void onBoneAdded(sklBone* bone) override {
        // TODO
    }
    void onBoneRemoved(sklBone* bone) override {
        // TODO
    }

    template<typename T>
    T* addComponent(const char* name) {
        T* ptr = new T();
        ptr->setName(name);
        components.push_back(std::unique_ptr<T>(ptr));
        return ptr;
    }

    HSHARED<sklmSkeletalModelInstance> createInstance();
    HSHARED<sklmSkeletalModelInstance> createInstance(HSHARED<sklSkeletonInstance>& skl_inst);

    void dbgLog();

    static void reflect();
};
