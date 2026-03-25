#pragma once

#include "scn_transform.hpp"
#include "gpu/gpu.hpp"
#include "gpu/render/uniform.hpp"
#include "gpu/param_block/transform_block.hpp"

#include "reflection/reflection.hpp"

class scnRenderScene;
class scnRenderObject {
    friend scnRenderScene;

    TransformTicket* transform_ticket = nullptr;

public:
    TYPE_ENABLE();
protected:
    Handle<TransformNode> scene_node;
    std::vector<gpuRenderable*> renderables;

    //gpuUniformBuffer* ubuf_model = 0;
    gpuTransformBlock* transform_block = nullptr;
    bool own_ubuf_model = true;

    void addRenderable(gpuRenderable* r) {
        renderables.emplace_back(r);
    }

public:
    scnRenderObject(bool own_model_ubuf = true)
    : own_ubuf_model(own_ubuf_model) {
        if(own_model_ubuf) {
            transform_block = gpuGetDevice()->createParamBlock<gpuTransformBlock>();
            /*
            ubuf_model = gpuGetPipeline()->createUniformBuffer(UNIFORM_BUFFER_MODEL);
            ubuf_model->setMat4(
                ubuf_model->getDesc()->getUniform(UNIFORM_MODEL_TRANSFORM),
                gfxm::mat4(1.0f)
            );
            ubuf_model->setMat4(
                ubuf_model->getDesc()->getUniform(UNIFORM_MODEL_TRANSFORM_PREV),
                gfxm::mat4(1.0f)
            );*/
        }
    }
    virtual ~scnRenderObject() {
        for (int i = 0; i < renderables.size(); ++i) {
            delete renderables[i];
        }

        if(own_ubuf_model) {
            gpuGetDevice()->destroyParamBlock(transform_block);
            //delete ubuf_model; // TODO ?
        }
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
