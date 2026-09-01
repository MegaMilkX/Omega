#include "buffer_texture.hpp"


gpuBufferTexture::gpuBufferTexture() {
    glGenTextures(1, &id);
    glActiveTexture(GL_TEXTURE0);
    GL_CHECK(glBindTexture(GL_TEXTURE_BUFFER, id));

    glBindTexture(GL_TEXTURE_BUFFER, 0);
}

gpuBufferTexture::~gpuBufferTexture() {
    glDeleteTextures(1, &id);
}

void gpuBufferTexture::setData(void* data, size_t byteCount) {
    width = byteCount / (sizeof(float) * 4);
    buffer.setTextureData(data, byteCount);
    glActiveTexture(GL_TEXTURE0);
    GL_CHECK(glBindTexture(GL_TEXTURE_BUFFER, id));
    GL_CHECK(glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, buffer.getId()));
    glBindTexture(GL_TEXTURE_BUFFER, 0);
}

void gpuBufferTexture::setData(const gfxm::vec4* data, size_t count) {
    setData((void*)data, count * sizeof(*data));
}

void gpuBufferTexture::setData(const float* data, size_t count) {
    setData((void*)data, count * sizeof(*data));
}

