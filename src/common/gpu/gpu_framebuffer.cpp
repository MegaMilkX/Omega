#include "gpu_framebuffer.hpp"



gpuFrameBuffer::gpuFrameBuffer() {
    glGenFramebuffers(1, &fbo);
}
gpuFrameBuffer::~gpuFrameBuffer() {
    glDeleteFramebuffers(1, &fbo);
}
void gpuFrameBuffer::addColorTarget(int attachment_index, const char* name, gpuTexture2d* texture) {
    const int mip_level = 0;

    if (!touchSize(texture)) {
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + attachment_index, GL_TEXTURE_2D, texture->getId(), mip_level);
    GL_CHECK(;);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    color_targets.push_back(ColorTarget{ std::string(name), texture, attachment_index, 0 });
}
void gpuFrameBuffer::addColorTarget(int attachment_index, const char* name, gpuArrayTexture2d* texture, int layer) {
    const int mip_level = 0;

    if (!touchSize(texture)) {
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + attachment_index, texture->getId(), mip_level, layer);
    GL_CHECK(;);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    color_targets.push_back(ColorTarget{ std::string(name), texture, attachment_index, layer });
}
void gpuFrameBuffer::addDepthTarget(gpuTexture2d* texture) {
    const int mip_level = 0;

    if (!touchSize(texture)) {
        return;
    }

    depth_target = texture;

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture->getId(), mip_level);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
void gpuFrameBuffer::addDepthTarget(gpuArrayTexture2d* texture, int layer) {
    const int mip_level = 0;

    if (!touchSize(texture)) {
        return;
    }

    depth_target = texture;

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, texture->getId(), mip_level, layer);
    GL_CHECK(;);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

