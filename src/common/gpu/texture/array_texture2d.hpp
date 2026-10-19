#pragma once

#include "texture.hpp"


class gpuArrayTexture2d : public gpuTexture {
public:
    gpuArrayTexture2d();
    ~gpuArrayTexture2d();

    gpuTextureType getType() const override { return gpuTextureType::Array2d; }

    void init(GLint internalFormat, int width, int height, int depth, int channels);
    void setWrapMode(GPU_TEXTURE_WRAP wrap);
    void setBorderColor(const gfxm::vec4& color);
};