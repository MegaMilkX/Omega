#pragma once

#include "scn_transform.hpp"
#include "gpu/gpu.hpp"
#include "gpu/render/uniform.hpp"

#include "reflection/reflection.hpp"

class scnRenderScene;
class scnRenderObject {
    friend scnRenderScene;
    
public:
    TYPE_ENABLE();
protected:
    SCN_TRANSFORM_SOURCE transform_source = SCN_TRANSFORM_SOURCE::NONE;
    union {
        scnNode* node = 0;
        struct {
            scnSkeleton* skeleton;
            uint32_t bone_id;
        };
    };
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

    void                    setNode(scnNode* n) { 
        node = n;
        transform_source = SCN_TRANSFORM_SOURCE::NODE;
    }
    scnNode*                getNode() const { 
        return (transform_source == SCN_TRANSFORM_SOURCE::NODE ? node : 0); 
    }
    void                    setSkeletonNode(scnSkeleton* skel, int bone_id) {
        skeleton = skel;
        this->bone_id = bone_id;
        transform_source = SCN_TRANSFORM_SOURCE::SKELETON;
    }
    scnSkeleton*            getSkeletonNode() const {
        return (transform_source == SCN_TRANSFORM_SOURCE::SKELETON ? skeleton : 0);
    }
    uint32_t                getSkeletonNodeBone() const {
        return (transform_source == SCN_TRANSFORM_SOURCE::SKELETON ? bone_id : 0); // TODO: -1?
    }

    int                                 renderableCount() const { return renderables.size(); }
    /* TODO: const */ gpuRenderable*    getRenderable(int i) const { return renderables[i]; }

    virtual void onAdded() = 0;
    virtual void onRemoved() = 0;
};