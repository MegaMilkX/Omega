#pragma once

#include "gpu_renderable.hpp"
#include "gpu_uniform_buffer.hpp"


class gpuRenderableInstance {
    gpuRenderable*                  renderable = 0;
    std::vector<gpuUniformBuffer*>  uniform_buffers;
public:
    void setRenderable(gpuRenderable* r) {
        renderable = r;
    }

    gpuRenderableInstance& addUniformBuffer(gpuUniformBuffer* buf) {
        uniform_buffers.push_back(buf);
        return *this;
    }

    void bindUniformBuffers() {
        for (int i = 0; i < uniform_buffers.size(); ++i) {
            auto& ub = uniform_buffers[i];
            GLint gl_id = ub->gpu_buf.getId();
            glBindBufferBase(GL_UNIFORM_BUFFER, ub->getDesc()->id, gl_id);
        }
    }

    gpuRenderable* getRenderable() {
        return renderable;
    }
};