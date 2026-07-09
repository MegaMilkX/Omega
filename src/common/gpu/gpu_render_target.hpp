#pragma once

#include "handle/hshared.hpp"
#include "gpu/types.hpp"
#include "gpu/gpu_texture_2d.hpp"
#include "gpu/gpu_framebuffer.hpp"

constexpr int RT_BUFFER_LAST_WRITTEN = -1;


class gpuPipeline;
class gpuRenderSequence;
class gpuRenderTarget {
    friend gpuPipeline;

    gpuPipeline* pipeline = 0;
    //gpuRenderSequence* sequence = 0;
    int width = 800;
    int height = 600;

public:
    struct TextureLayer {
        std::unique_ptr<gpuTexture2d> textures[2];
        int lwt = 0; // Used to get the last written to texture for debug and stuff, dynamically updated during rendering

        TextureLayer() {}
        TextureLayer(TextureLayer&& other) noexcept {
            textures[0] = std::move(other.textures[0]);
            textures[1] = std::move(other.textures[1]);
            lwt = other.lwt;
        }
    };

    int dbg_geomRangeBegin = 0;
    int dbg_geomRangeEnd = INT_MAX;
    bool dbg_drawWireframe = false;

    gpuRenderTarget() {}

    gpuRenderTarget(int width, int height)
        : //sequence(seq),
        width(width),
        height(height)
    {}

    ~gpuRenderTarget();

    void updateDirty();

    int default_output_texture = 0;
    RT_OUTPUT default_output_mode = RT_OUTPUT_RGB;
    gpuTexture2d* depth_texture = 0;
    std::vector<TextureLayer> layers;
    //std::vector<std::unique_ptr<gpuFrameBuffer>> framebuffers;
    std::vector<int> pipe_channel_to_layer;

    const gpuPipeline* getPipeline() const { return pipeline; }

    void setDefaultOutput(const char* name, RT_OUTPUT output_mode = RT_OUTPUT_RGB);

    gpuTexture2d* getTexture(const char* name, int buffer_idx = RT_BUFFER_LAST_WRITTEN);

    void setSize(int width, int height);

    int getWidth() const { return width; }
    int getHeight() const { return height; }

    void setDebugRenderGeometryRange(int begin, int end);
};
