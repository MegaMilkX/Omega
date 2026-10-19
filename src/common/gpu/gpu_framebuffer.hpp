#pragma once

#include <string>
#include "platform/gl/glextutil.h"
#include "gpu/texture/texture2d.hpp"
#include "gpu/texture/array_texture2d.hpp"
#include "handle/hshared.hpp"


class gpuFrameBuffer {
    unsigned int fbo;

    struct ColorTarget {
        std::string name;
        gpuTexture* texture = nullptr;
        int index;
        int layer;
    };
    std::vector<ColorTarget> color_targets;
    gpuTexture* depth_target = nullptr;
    int width = 0;
    int height = 0;

    bool touchSize(gpuTexture* texture) {        
        const int tex_width = texture->getWidth();
        const int tex_height = texture->getHeight();
        if (width == 0) {
            width = tex_width;
        } else if (width != tex_width) {
            assert(false);
            LOG_ERR("Framebuffer color target width mismatch");
            return false;
        }
        if (height == 0) {
            height = tex_height;
        } else if (height != tex_height) {
            assert(false);
            LOG_ERR("Framebuffer color target height mismatch");
            return false;
        }
        return true;
    }
public:
    gpuFrameBuffer();
    ~gpuFrameBuffer();

    void addColorTarget(int attachment_index, const char* name, gpuTexture2d* texture);
    void addColorTarget(int attachment_index, const char* name, gpuArrayTexture2d* texture, int layer);
    void addDepthTarget(gpuTexture2d* texture);
    void addDepthTarget(gpuArrayTexture2d* texture, int layer);

    void updateSize() {
        gpuTexture* tex = nullptr;
        if(depth_target) {
            tex = depth_target;
        } else if(!color_targets.empty()) {
            tex = color_targets[0].texture;
        } else {
            return;
        }

        width = tex->getWidth();
        height = tex->getHeight();
    }

    bool validate() {
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        GLenum r = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        bool result = r == GL_FRAMEBUFFER_COMPLETE;
        if (!result) {
            LOG_ERR("gpuFrameBuffer::validate(): " << r);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return result;
    }

    int colorTargetCount() const {
        return color_targets.size();
    }
    const char* getColorTargetName(int i) const {
        return color_targets[i].name.c_str();
    }

    gfxm::ivec2 getSize() const { return gfxm::ivec2(width, height); }

    unsigned int getId() const {
        return fbo;
    }
};

inline void gpuFrameBufferBind(const gpuFrameBuffer* fb) {
    glBindFramebuffer(GL_FRAMEBUFFER, fb->getId());
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        assert(false);
        return;
    }
}
inline void gpuFrameBufferUnbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}