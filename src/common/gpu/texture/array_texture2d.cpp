#include "array_texture2d.hpp"


gpuArrayTexture2d::gpuArrayTexture2d() {
    GL_CHECK(;);
    glGenTextures(1, &id);
    GL_CHECK(;);
}
gpuArrayTexture2d::~gpuArrayTexture2d() {
    glDeleteTextures(1, &id);
}
void gpuArrayTexture2d::init(GLint internalFormat, int width, int height, int depth, int channels) {
    GL_CHECK(;);
    this->internalFormat = internalFormat;
    this->width = width;
    this->height = height;
    this->depth = depth;

    GLsizei mipCount = log2f(gfxm::_max(width, height)) + 1;

    glActiveTexture(GL_TEXTURE0);
    GL_CHECK(;);
    glBindTexture(GL_TEXTURE_2D_ARRAY, id);
    GL_CHECK(;);

    glTexStorage3D(GL_TEXTURE_2D_ARRAY, mipCount, internalFormat, width, height, depth);
    GL_CHECK(;);

    glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

void gpuArrayTexture2d::setWrapMode(GPU_TEXTURE_WRAP wrap) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, id);

    GLint w = GL_REPEAT;
    switch (wrap) {
    case GPU_TEXTURE_WRAP_CLAMP: w = GL_CLAMP_TO_EDGE; break;
    case GPU_TEXTURE_WRAP_REPEAT: w = GL_REPEAT; break;
    case GPU_TEXTURE_WRAP_CLAMP_BORDER: w = GL_CLAMP_TO_BORDER; break;
    default: assert(false); return;
    }

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, w);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, w);

    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

void gpuArrayTexture2d::setBorderColor(const gfxm::vec4& color) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, id);
    glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, (float*)&color);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}