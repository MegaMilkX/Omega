#pragma once

#include <vector>
#include <memory>
#include <map>
#include "platform/gl/glextutil.h"
#include "gpu/gpu_texture_2d.hpp"
#include "gpu/gpu_uniform_buffer.hpp"
#include "gpu/gpu_material.hpp"
#include "gpu/gpu_render_target.hpp"
#include "gpu/gpu_framebuffer.hpp"
#include "gpu/pass/gpu_pass.hpp"
#include "gpu/gpu_pipeline_branch.hpp"
#include "util/strid.hpp"

constexpr float FLOAT_INF = std::numeric_limits<float>::infinity();

class gpuPipeline {
public:
    struct RenderChannel {
        std::string name;
        GLint format;
        int lwt;
        bool is_depth;
        bool is_double_buffered;
        GPU_TEXTURE_WRAP wrap_mode;
        gfxm::vec4 border_color;
        gfxm::vec3 clear_color;
    };

private:
    std::vector<RenderChannel> render_channels;
    std::map<std::string, int> rt_map;
    int output_target = -1;

    std::vector<std::unique_ptr<gpuUniformBufferDesc>> uniform_buffer_descs;
    std::map<std::string, gpuUniformBufferDesc*> uniform_buffer_descs_by_name;

    std::vector<std::unique_ptr<gpuUniformBuffer>> uniform_buffers;

    std::vector<gpuUniformBuffer*> attached_uniform_buffers;

    // New stuff
    gpuPipelineBranch pipeline_root;
    std::vector<gpuPass*> linear_passes;

    std::set<gpuRenderTarget*> render_targets;

    bool is_pipeline_dirty = true;

    void updatePassSequence();
    void createFramebuffers(gpuRenderTarget* target);
public:
    virtual ~gpuPipeline() {}

    virtual void init() = 0;

    void addColorChannel(
        const char* name,
        GLint format,
        bool is_double_buffered = false,
        GPU_TEXTURE_WRAP wrap_mode = GPU_TEXTURE_WRAP_CLAMP_BORDER,
        const gfxm::vec4& border_color = gfxm::vec4(FLOAT_INF, FLOAT_INF, FLOAT_INF, .0f),
        /* TODO: Unused? */ const gfxm::vec4& clear_color = gfxm::vec4(0, 0, 0, 0)
    );
    void addDepthChannel(const char* name);
    void setOutputChannel(const char* render_target_name);

    gpuPass* addPass(const char* path, gpuPass* pass);

    gpuUniformBufferDesc*   createUniformBufferDesc(const char* name);
    gpuUniformBufferDesc*   getUniformBufferDesc(const char* name);
    gpuUniformBuffer*       createUniformBuffer(const char* name);
    void                    destroyUniformBuffer(gpuUniformBuffer* buf);

    void attachUniformBuffer(gpuUniformBuffer* buf);
    bool isUniformBufferAttached(const char* name);

    bool compile();
    void updateDirty();

    void initRenderTarget(gpuRenderTarget* rt);

    int                     channelCount() const;
    RenderChannel*          getChannel(int i);
    const RenderChannel*    getChannel(int i) const;
    int                     getChannelIndex(const char* name);
    int                     getFrameBufferIndex(const char* pass);

    gpuMaterial* createMaterial();

    void bindUniformBuffers();
    
    int uniformBufferCount() const;
    int passCount() const;
   
    gpuUniformBufferDesc* getUniformBuffer(int i);
    gpuPass* getPass(int pass);

    const gpuPass* findPass(const char* path) const;
    gpuPipelineNode* findNode(const char* path);

    void enableTechnique(const char* path, bool value);

    //
    void notifyRenderTargetDestroyed(gpuRenderTarget* rt);
};
