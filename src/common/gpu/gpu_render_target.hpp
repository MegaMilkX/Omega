#pragma once

#include "handle/hshared.hpp"
#include "gpu/gpu_texture_2d.hpp"
#include "gpu/gpu_framebuffer.hpp"

class gpuPipeline;
class gpuRenderTarget {
    friend gpuPipeline;

    gpuPipeline* pipeline = 0;
    int width = 800;
    int height = 600;

public:
    int dbg_geomRangeBegin = 0;
    int dbg_geomRangeEnd = INT_MAX;
    bool dbg_drawWireframe = false;

    gpuRenderTarget() {}

    gpuRenderTarget(int width, int height)
        : width(width),
        height(height)
    {}

    int default_output_texture = 0;
    gpuTexture2d* depth_texture = 0;
    std::vector<HSHARED<gpuTexture2d>> textures;
    std::vector<std::unique_ptr<gpuFrameBuffer>> framebuffers;

    void setDefaultOutput(const char* name);

    gpuTexture2d* getTexture(const char* name);
    HSHARED<gpuTexture2d> getTextureSharedHandle(const char* name);
    void bindFrameBuffer(const char* technique, int pass);
    
    void setSize(int width, int height) {
        if (this->width == width && this->height == height) {
            return;
        }
        this->width = width;
        this->height = height;
        for (int i = 0; i < textures.size(); ++i) {
            textures[i]->resize(width, height);
        }
    }
    int getWidth() const { return width; }
    int getHeight() const { return height; }

    void setDebugRenderGeometryRange(int begin, int end);
};