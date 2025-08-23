#pragma once

#include "scn_transform.hpp"
#include "gpu/gpu.hpp"
#include "gpu/render/uniform.hpp"

#include "reflection/reflection.hpp"

class scnRenderScene;
class scnRenderObject {
    friend scnRenderScene;

    gfxm::mat4 mat_model_prev;

public:
    TYPE_ENABLE();
protected:
    Handle<TransformNode> scene_node;
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
        ubuf_model->setMat4(
            ubuf_model->getDesc()->getUniform(UNIFORM_MODEL_TRANSFORM_PREV),
            gfxm::mat4(1.0f)
        );
    }
    virtual ~scnRenderObject() {
        for (int i = 0; i < renderables.size(); ++i) {
            delete renderables[i];
        }

        delete ubuf_model; // TODO ?
    }

    void setTransformNode(Handle<TransformNode> n) {
        scene_node = n;
    }
    Handle<TransformNode> getTransformNode() const {
        return scene_node;
    }

    int                                 renderableCount() const { return renderables.size(); }
    /* TODO: const */ gpuRenderable*    getRenderable(int i) const { return renderables[i]; }

    virtual void onAdded() = 0;
    virtual void onRemoved() = 0;
};
