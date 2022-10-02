#pragma once

#include "handle/hshared.hpp"
#include "gpu/glx_texture_2d.hpp"
#include "gpu/gpu_framebuffer.hpp"

class gpuPipeline;
class gpuRenderTarget {
    friend gpuPipeline;

    gpuPipeline* pipeline = 0;
    int width = 800;
    int height = 600;
public:
    std::vector<HSHARED<gpuTexture2d>> textures;
    std::vector<std::unique_ptr<gpuFrameBuffer>> framebuffers;

    gpuTexture2d* getTexture(const char* name);
    void bindFrameBuffer(const char* technique, int pass);

    void setSize(int width, int height) {
        this->width = width;
        this->height = height;
        for (int i = 0; i < textures.size(); ++i) {
            textures[i]->resize(width, height);
        }
    }
    int getWidth() const { return width; }
    int getHeight() const { return height; }
};