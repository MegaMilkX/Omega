#pragma once

#include "math/gfxm.hpp"
#include "gpu/gpu_buffer.hpp"


class gpuBufferTexture {
    GLuint id = 0;
    int width = 0;
    gpuBuffer buffer;
public:
    gpuBufferTexture();
    ~gpuBufferTexture();

    GLuint getId() const { return id; }

    void setData(void* data, size_t byteCount);
    void setData(const gfxm::vec4* data, size_t count);
    void setData(const float* data, size_t count);
};

