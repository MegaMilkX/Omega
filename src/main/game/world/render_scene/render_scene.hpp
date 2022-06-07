#pragma once

#include <vector>
#include "math/gfxm.hpp"
#include "common/render/gpu_renderable.hpp"
#include "common/render/render.hpp"
#include "common/render/gpu_mesh_desc.hpp"


// TODO: ???

struct scnNode {
    gfxm::mat4 transform;
};

class scnRenderScene;
class scnRenderObject {
    friend scnRenderScene;
    
protected:
    scnNode* node = 0;
    std::vector<gpuRenderable*> renderables;

    gpuUniformBuffer* ubuf_model = 0;

    void addRenderable(gpuRenderable* r) {
        renderables.emplace_back(r);
    }

public:
    scnRenderObject() {
        ubuf_model = gpuGetPipeline()->createUniformBuffer(UNIFORM_BUFFER_MODEL);
        ubuf_model->setMat4(
            ubuf_model->getDesc()->getUniform(UNIFORM_MODEL_TRANSFORM),
            gfxm::mat4(1.0f)
        );
    }
    virtual ~scnRenderObject() {
        for (int i = 0; i < renderables.size(); ++i) {
            delete renderables[i];
        }

        delete ubuf_model; // TODO ?
    }

    void                    setNode(scnNode* n) { node = n; }
    scnNode*                getNode() const { return node; }

    int                                 renderableCount() const { return renderables.size(); }
    /* TODO: const */ gpuRenderable*    getRenderable(int i) const { return renderables[i]; }

    virtual void onAdded() = 0;
    virtual void onRemoved() = 0;
};

class scnMeshObject : public scnRenderObject {
    void onAdded() override {
        for (int i = 0; i < renderableCount(); ++i) {
            if (renderables[i]->getMaterial() && renderables[i]->getMeshDesc()) {
                renderables[i]->compile();
            }
        }
        
    }
    void onRemoved() override {

    }
public:
    scnMeshObject() {
        

        addRenderable(new gpuRenderable);
        getRenderable(0)->attachUniformBuffer(ubuf_model);
    }
    void setMeshDesc(const gpuMeshDesc* desc) {
        getRenderable(0)->setMeshDesc(desc);
    }
    void setMaterial(gpuMaterial* mat) {
        getRenderable(0)->setMaterial(mat);
    }
};

class scnModel : public scnRenderObject {
    std::vector<gfxm::mat4> local_transforms;
    std::vector<gfxm::mat4> world_transforms;
public:

};

struct scnSkeleton {
    std::vector<int>        parents;
    std::vector<gfxm::mat4> local_transforms;
    std::vector<gfxm::mat4> world_transforms;

    void init(Skeleton* sk) {
        parents = sk->parents;

        local_transforms = sk->default_pose;
        world_transforms = local_transforms;
        world_transforms[0] = gfxm::mat4(1.0f);
    }
};

class scnSkin : public scnRenderObject {
    friend scnRenderScene;

    gpuBuffer* bufVertexSource = 0;
    gpuBuffer* bufNormalSource = 0;
    gpuBuffer* bufBoneIndex4 = 0;
    gpuBuffer* bufBoneWeight4 = 0;

    gpuBuffer   vertexBuffer;
    gpuBuffer   normalBuffer;
    gpuMeshDesc skin_mesh_desc;

    std::vector<int>        bone_indices;
    std::vector<gfxm::mat4> inverse_bind_transforms;
    std::vector<gfxm::mat4> pose_transforms;

    scnSkeleton* skeleton = 0;

    void updatePoseTransforms() {
        for (int i = 0; i < pose_transforms.size(); ++i) {
            int bone_idx = bone_indices[i];
            auto& inverse_bind = inverse_bind_transforms[i];
            auto& world = skeleton->world_transforms[bone_idx];
            pose_transforms[i] = world * inverse_bind;
        }
    }

    void onAdded() override {
        getRenderable(0)->compile();
        pose_transforms.resize(bone_indices.size());
    }
    void onRemoved() override {

    }
public:
    scnSkin() {
        addRenderable(new gpuRenderable);
        getRenderable(0)->attachUniformBuffer(ubuf_model);
    }
    void setMeshDesc(const gpuMeshDesc* desc) {
        bufVertexSource = const_cast<gpuBuffer*>(desc->getAttribDesc(VFMT::Position_GUID)->buffer);
        bufNormalSource = const_cast<gpuBuffer*>(desc->getAttribDesc(VFMT::Normal_GUID)->buffer);
        bufBoneIndex4 = const_cast<gpuBuffer*>(desc->getAttribDesc(VFMT::BoneIndex4_GUID)->buffer);
        bufBoneWeight4 = const_cast<gpuBuffer*>(desc->getAttribDesc(VFMT::BoneWeight4_GUID)->buffer);

        int vertex_count = desc->getVertexCount();

        vertexBuffer.reserve(vertex_count * sizeof(gfxm::vec3), GL_DYNAMIC_DRAW);
        normalBuffer.reserve(vertex_count * sizeof(gfxm::vec3), GL_DYNAMIC_DRAW);

        skin_mesh_desc.clear();
        skin_mesh_desc.setAttribArray(VFMT::Position_GUID, &vertexBuffer, 0);
        skin_mesh_desc.setAttribArray(VFMT::Normal_GUID, &normalBuffer, 0);

        desc->merge(&skin_mesh_desc, false);
        skin_mesh_desc.setVertexCount(desc->getVertexCount());

        getRenderable(0)->setMeshDesc(&skin_mesh_desc);
    }
    void setSkeleton(scnSkeleton* skeleton) {
        this->skeleton = skeleton;
    }
    void setBoneIndices(int* indices, int count) {
        bone_indices.clear();
        bone_indices.insert(bone_indices.end(), indices, indices + count);        
    }
    void setInverseBindTransforms(const gfxm::mat4* transforms, int count) {
        inverse_bind_transforms.clear();
        inverse_bind_transforms.insert(inverse_bind_transforms.end(), transforms, transforms + count);
    }
    void setMaterial(gpuMaterial* mat) {
        getRenderable(0)->setMaterial(mat);
    }
};

#include "game/skinning/skinning_compute.hpp"

class scnRenderScene {
    std::vector<scnSkeleton*> skeletons;

    std::vector<scnRenderObject*> renderObjects;
    std::vector<scnSkin*> skinObjects;

public:
    void addRenderObject(scnRenderObject* o) {
        renderObjects.emplace_back(o);
        o->onAdded();
        
        scnSkin* skn = dynamic_cast<scnSkin*>(o);
        if (skn) {
            skinObjects.push_back(skn);
        }
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
            if (!ro->node) {
                continue;
            }
            ro->ubuf_model->setMat4(
                ubuf_model_model_loc,
                ro->node->transform
            );
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
