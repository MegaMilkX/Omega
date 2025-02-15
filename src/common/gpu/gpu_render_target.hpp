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

    struct TextureLayer {
        HSHARED<gpuTexture2d> textures[2];
    };

    int default_output_texture = 0;
    gpuTexture2d* depth_texture = 0;
    std::vector<TextureLayer> layers;
    std::vector<std::unique_ptr<gpuFrameBuffer>> framebuffers;

    const gpuPipeline* getPipeline() const { return pipeline; }

    void setDefaultOutput(const char* name);

    gpuTexture2d* getTexture(const char* name, int buffer_idx = 0);
    HSHARED<gpuTexture2d> getTextureSharedHandle(const char* name, int buffer_idx = 0);
    void bindFrameBuffer(const char* technique, int pass);
    
    void setSize(int width, int height);

    int getWidth() const { return width; }
    int getHeight() const { return height; }

    void setDebugRenderGeometryRange(int begin, int end);
};