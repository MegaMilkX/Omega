#pragma once

#include "handle/hshared.hpp"
#include "gpu/gpu_texture_2d.hpp"
#include "gpu/gpu_framebuffer.hpp"

constexpr int RT_BUFFER_LAST_WRITTEN = -1;

enum RT_OUTPUT {
    RT_OUTPUT_AUTO,
    RT_OUTPUT_RGB,
    RT_OUTPUT_RRR,
    RT_OUTPUT_GGG,
    RT_OUTPUT_BBB,
    RT_OUTPUT_AAA,
    RT_OUTPUT_DEPTH
};

class gpuPipeline;
class gpuRenderTarget {
    friend gpuPipeline;

    gpuPipeline* pipeline = 0;
    int width = 800;
    int height = 600;

public:
    struct TextureLayer {
        HSHARED<gpuTexture2d> textures[2];
        int lwt;
    };

    int dbg_geomRangeBegin = 0;
    int dbg_geomRangeEnd = INT_MAX;
    bool dbg_drawWireframe = false;

    gpuRenderTarget() {}

    gpuRenderTarget(int width, int height)
        : width(width),
        height(height)
    {}

    ~gpuRenderTarget();

    void updateDirty();

    int default_output_texture = 0;
    RT_OUTPUT default_output_mode = RT_OUTPUT_RGB;
    gpuTexture2d* depth_texture = 0;
    std::vector<TextureLayer> layers;
    std::vector<std::unique_ptr<gpuFrameBuffer>> framebuffers;

    const gpuPipeline* getPipeline() const { return pipeline; }

    void setDefaultOutput(const char* name, RT_OUTPUT output_mode = RT_OUTPUT_RGB);

    gpuTexture2d* getTexture(const char* name, int buffer_idx = RT_BUFFER_LAST_WRITTEN);
    HSHARED<gpuTexture2d> getTextureSharedHandle(const char* name, int buffer_idx = RT_BUFFER_LAST_WRITTEN);

    void bindFrameBuffer(const char* pass_path);
    
    void setSize(int width, int height);

    int getWidth() const { return width; }
    int getHeight() const { return height; }

    void setDebugRenderGeometryRange(int begin, int end);
};
