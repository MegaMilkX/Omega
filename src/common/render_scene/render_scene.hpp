#pragma once

#include <vector>
#include "math/gfxm.hpp"
#include "gpu/gpu_renderable.hpp"
#include "gpu/gpu.hpp"
#include "gpu/gpu_mesh_desc.hpp"

#include "gpu/render/uniform.hpp"

#include "render_scene/render_object/scn_mesh_object.hpp"
#include "render_scene/render_object/light_omni.hpp"
#include "render_scene/render_object/scn_skin.hpp"
#include "render_scene/render_object/scn_decal.hpp"
#include "render_scene/render_object/scn_text_billboard.hpp"
#include "render_scene/render_scene_view.hpp"

#include "gpu/skinning/skinning_compute.hpp"

#include "debug_draw/debug_draw.hpp"

class scnLightDirectional {
    gfxm::vec3  normal;
    uint32_t    color;
    float       intensity;
};

class scnRenderScene {
    std::vector<scnRenderObject*> renderObjects;
    std::vector<scnSkin*> skinObjects;
    std::vector<scnDecal*> decalObjects;
    std::vector<scnLightOmni*> lightObjects;

public:
    scnRenderScene();
    ~scnRenderScene();

    void addRenderObject(scnRenderObject* o) {
        renderObjects.push_back(o);
        o->onAdded();
        
        scnSkin* skn = dynamic_cast<scnSkin*>(o);
        if (skn) { skinObjects.push_back(skn); }
        scnDecal* dcl = dynamic_cast<scnDecal*>(o);
        if (dcl) { decalObjects.push_back(dcl); }
        scnLightOmni* omni = dynamic_cast<scnLightOmni*>(o);
        if(omni) { lightObjects.push_back(omni); }
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
        scnLightOmni* omni = dynamic_cast<scnLightOmni*>(o);
        if (omni) {
            for (int i = 0; i < lightObjects.size(); ++i) {
                if (lightObjects[i] == omni) {
                    lightObjects.erase(lightObjects.begin() + i);
                    break;
                }
            }
        }
    }

    void update(float dt) {
        // Update skin pose arrays (skin objects keep only the transforms they need, not the whole skeleton)
        for (int i = 0; i < skinObjects.size(); ++i) {
            auto skn = skinObjects[i];
            skn->updatePoseTransforms();
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
            d.bufTangentsSource = skn->bufTangentSource;
            d.bufBitangentsSource = skn->bufBitangentSource;
            d.bufBoneIndices = skn->bufBoneIndex4;
            d.bufBoneWeights = skn->bufBoneWeight4;
            d.bufVerticesOut = &skn->vertexBuffer;
            d.bufNormalsOut = &skn->normalBuffer;
            d.bufTangentsOut = &skn->tangentBuffer;
            d.bufBitangentsOut = &skn->bitangentBuffer;
            d.is_valid = skn->isValid();
        }
        if (!skinObjects.empty()) {
            updateSkinVertexDataCompute(&skin_data[0], skin_data.size());
        }

        int ubuf_model_model_loc = gpuGetPipeline()->getUniformBufferDesc(UNIFORM_BUFFER_MODEL)->getUniform(UNIFORM_MODEL_TRANSFORM);
        int ubuf_model_model_prev_loc = gpuGetPipeline()->getUniformBufferDesc(UNIFORM_BUFFER_MODEL)->getUniform(UNIFORM_MODEL_TRANSFORM_PREV);
        for (int i = 0; i < renderObjects.size(); ++i) {
            auto ro = renderObjects[i];
            if (ro->scene_node) {
                const gfxm::mat4& transform_current = ro->scene_node->getWorldTransform();
                ro->ubuf_model->setMat4(
                    ubuf_model_model_prev_loc,
                    ro->mat_model_prev
                );
                ro->ubuf_model->setMat4(
                    ubuf_model_model_loc,
                    transform_current
                );

                ro->mat_model_prev = transform_current;
            } 
        }

        // Debug draw
        // Skeletons
        /*
        for (int i = 0; i < skeletons.size(); ++i) {
            auto skel = skeletons[i];
            
            for (int j = skel->bone_count - 1; j > 0; --j) {
                int parent = skel->parents[j];
                dbgDrawLine(
                    skel->world_transforms[j] * gfxm::vec4(0,0,0,1),
                    skel->world_transforms[parent] * gfxm::vec4(0,0,0,1),
                    0xFFFFFFFF
                );
            }
        }*/
    }

    void draw(gpuRenderBucket* bucket) {
        for (int i = 0; i < renderObjects.size(); ++i) {
            for (int j = 0; j < renderObjects[i]->renderableCount(); ++j) {
                if (renderObjects[i]->getRenderable(j)->isInstanced()) {
                    if (renderObjects[i]->getRenderable(j)->getInstancingDesc()->getInstanceCount() == 0) {
                        continue;
                    }
                }
                bucket->add(renderObjects[i]->getRenderable(j));
            }
        }
        for (int i = 0; i < lightObjects.size(); ++i) {
            auto l = lightObjects[i];
            bucket->addLightOmni(
                l->getTransformNode()->getWorldTranslation(),
                l->color,
                l->intensity,
                l->enable_shadows
            );
        }
    }
};
