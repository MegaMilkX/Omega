#pragma once

#include "m3d/m3d_model.hpp"
#include "gpu/skinning/skin_instance.hpp"
#include "gpu/gpu.hpp"


class m3dSkeletalInstance {
    ResourceRef<m3dModel> model;
    HSHARED<SkeletonInstance> skl_inst;
    std::vector<std::unique_ptr<gpuRenderable>> renderables;
    std::vector<std::unique_ptr<gpuSkinInstance>> skin_instances;
    HTransform external_root = HTransform();

    void clear() {
        renderables.clear();
        skin_instances.clear();
    }
public:
    const m3dModel* getModel() const { return model.get(); }
    SkeletonInstance* getSkeletonInstance() { return skl_inst.get(); }
    int renderableCount() const { return static_cast<int>(renderables.size()); }
    gpuRenderable* getRenderable(int i) { return renderables[i].get(); }

    void attachTo(HTransform node) {
        external_root = node;
        if(skl_inst) {
            skl_inst->setExternalRootTransform(node);
        }
    }

    void submit(gpuRenderBucket* bucket) {
        for (int i = 0; i < skin_instances.size(); ++i) {
            auto skn = skin_instances[i].get();
            // TODO: Should not actually update pose for each submit,
            // Instead, only when root moved, applied anim samples, etc.
            skn->updatePose(skl_inst.get());
            gpuScheduleSkinTask(skn->skin_task);
        }
        for (int i = 0; i < renderables.size(); ++i) {
            auto rdr = renderables[i].get();
            bucket->add(rdr);
        }
    }

    void init(const ResourceRef<m3dModel>& mdl, HSHARED<SkeletonInstance> skeleton_inst = HSHARED<SkeletonInstance>()) {
        clear();
        model = mdl;
        if(skeleton_inst) {
            skl_inst = skeleton_inst;
        } else {
            skl_inst = model->skeleton->createInstance();
            if(external_root && skl_inst) {
                skl_inst->setExternalRootTransform(external_root);
            }
        }

        std::unordered_map<int, int> mesh_to_skin_inst;
        
        // meshes
        for (int i = 0; i < model->meshes.size(); ++i) {
            auto& m3d_mesh = model->meshes[i];

            if (m3d_mesh.skin) {
                auto shared = gpuGetDevice()->getSharedResources();
                auto pskin = new gpuSkinInstance;
                if (!pskin->build(&m3d_mesh, skl_inst.get())) {
                    delete pskin;
                    assert(false);
                    LOG_ERR("Failed to build skinned mesh");
                    continue;
                }
                mesh_to_skin_inst[i] = skin_instances.size();
                skin_instances.push_back(std::unique_ptr<gpuSkinInstance>(pskin));
            }
        }

        // TODO: while making instances of the same mesh actually instanced is not desirable here,
        // should still investigate if can make renderables for the same mesh_desc and material more lightweight
        
        // mesh instances
        for (int i = 0; i < model->mesh_instances.size(); ++i) {
            auto& m3d_mesh_inst = model->mesh_instances[i];
            auto& m3d_mesh = model->meshes[m3d_mesh_inst.mesh_idx];
            
            gpuRenderable* rdr = new gpuRenderable;
            auto mat = model->materials[m3d_mesh.material_idx];
            rdr->setRole(GPU_Role_Geometry);
            rdr->setMaterial(mat.get());
            
            if (m3d_mesh.skin) {
                auto it = mesh_to_skin_inst.find(m3d_mesh_inst.mesh_idx);
                if (it == mesh_to_skin_inst.end()) {
                    LOG_ERR("m3dSkeletalInstance: missing a skinning instance for a skinned mesh " << m3d_mesh_inst.mesh_idx);
                    delete rdr;
                    continue;
                }
                gpuSkinInstance* skin_inst = skin_instances[it->second].get();
                rdr->setMeshDesc(&skin_inst->mesh_desc);
                
                auto shared = gpuGetDevice()->getSharedResources();
                rdr->attachParamBlock(shared->getIdentityTransformBlock());
            } else {
                const gpuMesh* gpu_mesh = m3d_mesh.mesh.get();
                const gpuMeshDesc* mesh_desc = gpu_mesh->getMeshDesc();
                rdr->setMeshDesc(mesh_desc);

                int bone_idx = skl_inst->findBoneIndex(m3d_mesh_inst.bone_name.c_str());
                auto trs_block = skl_inst->getTransformBlock(bone_idx);
                rdr->attachParamBlock(trs_block);
            }

            rdr->compile();
            renderables.push_back(std::unique_ptr<gpuRenderable>(rdr));
        }
    }
};

