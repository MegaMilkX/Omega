#pragma once

#include <string>
#include "platform/gl/glextutil.h"
#include "gpu/gpu_texture_2d.hpp"
#include "handle/hshared.hpp"


class gpuFrameBuffer {
    unsigned int fbo;

    struct ColorTarget {
        std::string name;
        int index;
    };
    std::vector<ColorTarget> color_targets;
    HSHARED<gpuTexture2d> depth_target;
public:
    gpuFrameBuffer() {
        glGenFramebuffers(1, &fbo);
    }
    ~gpuFrameBuffer() {
        glDeleteFramebuffers(1, &fbo);
    }
    void addColorTarget(const char* name, gpuTexture2d* texture) {
        int index = color_targets.size();
        
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, GL_TEXTURE_2D, texture->getId(), 0);
        GL_CHECK(;);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        color_targets.push_back(ColorTarget{ std::string(name), index });
    }
    void addDepthTarget(HSHARED<gpuTexture2d> texture) {
        depth_target = texture;

        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture->getId(), 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void prepare() {
        // TODO: !!!
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        GLenum draw_buffers[] = {
            GL_COLOR_ATTACHMENT0
        };
        glDrawBuffers(1, draw_buffers);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    bool validate() {
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        bool result = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return result;
    }

    HSHARED<gpuTexture2d> getDepthTarget() {
        return depth_target;
    }

    int colorTargetCount() const {
        return color_targets.size();
    }
    const char* getColorTargetName(int i) const {
        return color_targets[i].name.c_str();
    }

    unsigned int getId() const {
        return fbo;
    }
};

inline void gpuFrameBufferBind(gpuFrameBuffer* fb) {
    glBindFramebuffer(GL_FRAMEBUFFER, fb->getId());
}
inline void gpuFrameBufferUnbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}