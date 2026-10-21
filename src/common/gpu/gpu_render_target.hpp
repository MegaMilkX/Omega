#pragma once

#include "handle/hshared.hpp"
#include "gpu/types.hpp"
#include "gpu/texture/texture2d.hpp"
#include "gpu/gpu_framebuffer.hpp"

constexpr int RT_BUFFER_LAST_WRITTEN = -1;


class gpuPipeline;
class gpuRenderSequence;
// TODO: Rename to gpuRenderResources or smth?
class gpuRenderTarget {
    friend gpuPipeline;

    gpuPipeline* pipeline = 0;
    int width = 800;
    int height = 600;
public:
    struct TextureLayer {
        std::unique_ptr<gpuTexture> textures[2];
        int idx = 0;
        int lwt = 0; // Used to get the last written to texture for debug and stuff, dynamically updated during rendering
        int explicit_width = 0;
        int explicit_height = 0;
        bool double_buffered = false;

        TextureLayer() {}
        TextureLayer(TextureLayer&& other) noexcept {
            textures[0] = std::move(other.textures[0]);
            textures[1] = std::move(other.textures[1]);
            idx = other.idx;
            lwt = other.lwt;
            explicit_width = other.explicit_width;
            explicit_height = other.explicit_height;
            double_buffered = other.double_buffered;
        }
    };
    int default_output_texture = 0;
    RT_OUTPUT default_output_mode = RT_OUTPUT_RGB;
    gpuTexture* depth_texture = 0;
    //std::vector<TextureLayer> layers;
    //std::vector<std::unique_ptr<gpuFrameBuffer>> framebuffers;

    std::vector<std::unique_ptr<TextureLayer>> channels;
    std::map<std::string, int> channel_map;
    std::vector<int> pipe_channel_to_local;

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

    TextureLayer* ensureChannel(const std::string& name, bool double_buffered);
    TextureLayer* getChannel(const std::string& name);
    TextureLayer* getChannelByPipeId(int i);
    const TextureLayer* getChannelByPipeId(int i) const;

    void updateDirty();

    const gpuPipeline* getPipeline() const { return pipeline; }

    void setDefaultOutput(const char* name, RT_OUTPUT output_mode = RT_OUTPUT_RGB);

    gpuTexture2d* getTexture(const char* name, int buffer_idx = RT_BUFFER_LAST_WRITTEN);

    void setSize(int width, int height);

    int getWidth() const { return width; }
    int getHeight() const { return height; }

    void setDebugRenderGeometryRange(int begin, int end);
};
