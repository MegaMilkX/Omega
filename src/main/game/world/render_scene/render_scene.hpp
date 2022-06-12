#pragma once

#include <vector>
#include "math/gfxm.hpp"
#include "common/render/gpu_renderable.hpp"
#include "common/render/render.hpp"
#include "common/render/gpu_mesh_desc.hpp"

#include "game/render/uniform.hpp"

#include "game/world/render_scene/render_object/scn_mesh_object.hpp"
#include "game/world/render_scene/render_object/scn_skin.hpp"
#include "game/world/render_scene/render_object/scn_decal.hpp"
#include "game/world/render_scene/render_object/scn_text_billboard.hpp"

#include "game/skinning/skinning_compute.hpp"

class scnRenderScene {
    std::vector<scnNode*>     nodes;
    std::vector<scnSkeleton*> skeletons;

    std::vector<scnRenderObject*> renderObjects;
    std::vector<scnSkin*> skinObjects;
    std::vector<scnDecal*> decalObjects;

public:
    void addRenderObject(scnRenderObject* o) {
        renderObjects.emplace_back(o);
        o->onAdded();
        
        scnSkin* skn = dynamic_cast<scnSkin*>(o);
        if (skn) { skinObjects.push_back(skn); }
        scnDecal* dcl = dynamic_cast<scnDecal*>(o);
        if (dcl) { decalObjects.push_back(dcl); }
    }
    void removeRenderObject(scnRenderObject* o) {
        for (int i = 0; i < renderObjects.size(); ++i) {
            if (renderObjects[i] == o) {
                renderObjects[i]->onRemoved();
                renderObjects.erase(renderObjects.begin() + i);
                break;
            }
        }

        scnSkin* skn = dynamic_cast<scnSkin*>(o);
        if (skn) {
            for (int i = 0; i < skinObjects.size(); ++i) {
                if (skinObjects[i] == skn) {
                    skinObjects.erase(skinObjects.begin() + i);
                    break;
                }
            }
        }
        scnDecal* dcl = dynamic_cast<scnDecal*>(o);
        if (dcl) {
            for (int i = 0; i < decalObjects.size(); ++i) {
                if (decalObjects[i] == dcl) {
                    decalObjects.erase(decalObjects.begin() + i);
                    break;
                }
            }
        }
    }

    void addNode(scnNode* node) {
        nodes.emplace_back(node);
    }
    void removeNode(scnNode* node) {
        for (int i = 0; i < nodes.size(); ++i) {
            if (nodes[i] == node) {
                nodes.erase(nodes.begin() + i);
                break;
            }
        }
    }

    void addSkeleton(scnSkeleton* sk) {
        skeletons.emplace_back(sk);
    }
    void removeSkeleton(scnSkeleton* sk) {
        for (int i = 0; i < skeletons.size(); ++i) {
            if (skeletons[i] == sk) {
                skeletons.erase(skeletons.begin() + i);
                break;
            }
        }
    }

    void update(/*TODO: float dt for interpolation?*/) {
        // Update skeleton transforms
        for (int i = 0; i < skeletons.size(); ++i) {
            auto skel = skeletons[i];
            
            for (int j = 1; j < skel->world_transforms.size(); ++j) {
                int parent = skel->parents[j];
                skel->world_transforms[j]
                    = skel->world_transforms[parent] 
                    * skel->local_transforms[j];
            }
        }

        // Update skin pose arrays (skin objects keep only the transforms they need)
        for (int i = 0; i < skinObjects.size(); ++i) {
            auto skn = skinObjects[i];
            skn->updatePoseTransforms();
        }

        // Update node world transforms
        for (int i = 0; i < nodes.size(); ++i) {
            auto n = nodes[i];
            switch (n->transform_source) {
            case SCN_TRANSFORM_SOURCE::NODE:
                n->world_transform = n->node->world_transform * n->local_transform;
                break;
            case SCN_TRANSFORM_SOURCE::SKELETON:
                n->world_transform = n->skeleton->world_transforms[n->bone_id] * n->local_transform;
                break;
            case SCN_TRANSFORM_SOURCE::NONE:
                n->world_transform = n->local_transform;
                break;
            };
        }

        // Do skinning
        std::vector<SkinUpdateData> skin_data(skinObjects.size());
        for (int i = 0; i < skinObjects.size(); ++i) {
            auto skn = skinObjects[i];
            SkinUpdateData& d = skin_data[i];
            d.vertex_count = skn->skin_mesh_desc.getVertexCount();
            d.pose_transforms = &skn->pose_transforms[0];
            d.pose_count = skn->pose_transforms.size();
            d.bufVerticesSource = skn->bufVertexSource;
            d.bufNormalsSource = skn->bufNormalSource;
            d.bufBoneIndices = skn->bufBoneIndex4;
            d.bufBoneWeights = skn->bufBoneWeight4;
            d.bufVerticesOut = &skn->vertexBuffer;
            d.bufNormalsOut = &skn->normalBuffer;
        }
        if (!skinObjects.empty()) {
            updateSkinVertexDataCompute(&skin_data[0], skin_data.size());
        }

        int ubuf_model_model_loc = gpuGetPipeline()->getUniformBufferDesc(UNIFORM_BUFFER_MODEL)->getUniform(UNIFORM_MODEL_TRANSFORM);
        for (int i = 0; i < renderObjects.size(); ++i) {
            auto ro = renderObjects[i];
            switch (ro->transform_source) {
            case SCN_TRANSFORM_SOURCE::NODE:
                if (!ro->node) { assert(false); continue; }
                ro->ubuf_model->setMat4(
                    ubuf_model_model_loc,
                    ro->node->world_transform
                );
                break;
            case SCN_TRANSFORM_SOURCE::SKELETON:
                if (!ro->skeleton) { assert(false); continue; }
                ro->ubuf_model->setMat4(
                    ubuf_model_model_loc,
                    ro->skeleton->world_transforms[ro->bone_id]
                );
                break;
            }            
        }
    }

    void draw() {
        for (int i = 0; i < renderObjects.size(); ++i) {
            for (int j = 0; j < renderObjects[i]->renderableCount(); ++j) {
                gpuDrawRenderable(renderObjects[i]->getRenderable(j));
            }
        }
    }
};
