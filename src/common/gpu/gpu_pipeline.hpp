#pragma once

#include <vector>
#include <memory>
#include <map>
#include "platform/gl/glextutil.h"
#include "gpu/gpu_types.hpp"
#include "gpu/types.hpp"
#include "gpu/gpu_texture_2d.hpp"
#include "gpu/gpu_uniform_buffer.hpp"
#include "gpu/gpu_material.hpp"
#include "gpu/gpu_render_target.hpp"
#include "gpu/gpu_framebuffer.hpp"
#include "gpu/pass/gpu_pass.hpp"
#include "gpu/gpu_pipeline_branch.hpp"
#include "gpu/param_block/param_block_context.hpp"
#include "util/strid.hpp"


struct GPU_INTERMEDIATE_PASS_DESC {
    std::vector<const gpuCompiledShader*> shaders;
    std::vector<gpuShaderSet*> base_shaders;
    std::vector<gpuShaderSet*> extension_shaders;
    shader_flags_t shader_flags = 0; // TODO:
    int extended_by_material = 0x0;
    draw_flags_t draw_flags = 0;
    GPU_BLEND_MODE blend_mode;

    void clearBaseShaderSets() {
        base_shaders.clear();
    }
    void addBaseShaderSet(gpuShaderSet* shader_set) {
        base_shaders.push_back(shader_set);
    }
    void addExtensionShaderSet(gpuShaderSet* shader_set) {
        extension_shaders.push_back(shader_set);
    }
};
struct GPU_INTERMEDIATE_RENDERABLE_CONTEXT {
    std::unordered_map<int, GPU_INTERMEDIATE_PASS_DESC> pass_map;
    
    GPU_INTERMEDIATE_PASS_DESC* getOrCreatePass(pipe_pass_id_t id) {
        auto it = pass_map.find(id);
        if (it == pass_map.end()) {
            it = pass_map.insert(
                std::make_pair(id, GPU_INTERMEDIATE_PASS_DESC{})
            ).first;
        }
        return &it->second;
    }
};


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
        int explicit_width = 0;
        int explicit_height = 0;
    };

private:
    std::vector<RenderChannel> render_channels;
    std::map<std::string, int> rt_map;
    int output_target = -1;

    std::vector<std::unique_ptr<gpuUniformBufferDesc>> uniform_buffer_descs;
    std::map<std::string, gpuUniformBufferDesc*> uniform_buffer_descs_by_name;

    std::vector<std::unique_ptr<gpuUniformBuffer>> uniform_buffers;

    std::vector<gpuUniformBuffer*> attached_uniform_buffers;
    std::map<type, gpuParamBlock*> param_blocks;

    // New stuff
    gpuPipelineBranch pipeline_root;
    std::vector<gpuPass*> linear_passes;

    std::set<gpuRenderTarget*> render_targets;

    bool is_pipeline_dirty = true;
    int dbg_param_block_upload_count = 0;

    void updatePassSequence();
    void createFramebuffers(gpuRenderTarget* target);
public:
    virtual ~gpuPipeline() {}

    virtual void init() = 0;
    virtual void resolveRenderableRole(GPU_Role t, GPU_INTERMEDIATE_RENDERABLE_CONTEXT& ctx, const gpuMaterial* mat) = 0;
    virtual void resolveRenderableEffect(GPU_Effect t, GPU_INTERMEDIATE_RENDERABLE_CONTEXT& ctx, const gpuMaterial* mat) = 0;

    void addColorChannel(
        const char* name,
        GLint format,
        bool is_double_buffered = false,
        GPU_TEXTURE_WRAP wrap_mode = GPU_TEXTURE_WRAP_CLAMP_BORDER,
        int explicit_width = 0, int explicit_height = 0,
        const gfxm::vec4& border_color = gfxm::vec4(FLOAT_INF, FLOAT_INF, FLOAT_INF, .0f)
    );
    void addDepthChannel(const char* name);
    void setOutputChannel(const char* render_target_name);

    gpuPass* addPass(const char* path, gpuPass* pass, int layer = 0);

    gpuUniformBufferDesc*   createUniformBufferDesc(const char* name);
    gpuUniformBufferDesc*   getUniformBufferDesc(const char* name);
    gpuUniformBuffer*       createUniformBuffer(const char* name);
    gpuUniformBuffer*       createUniformBuffer(gpuUniformBufferDesc* desc);
    void                    destroyUniformBuffer(gpuUniformBuffer* buf);

    void attachUniformBuffer(gpuUniformBuffer* buf);
    bool isUniformBufferAttached(const char* name);
    gpuPipeline& attachParamBlock(gpuParamBlock* block);

    bool compile();
    void updateDirty();
    void updateParamBlocks();

    void initRenderTarget(gpuRenderTarget* rt);

    void draw(gpuRenderTarget* target, gpuRenderBucket* bucket, const DRAW_PARAMS& params);

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
    gpuPass* getPass(pipe_pass_id_t i);

    const gpuPass* findPass(const char* path) const;
    gpuPipelineNode* findNode(const char* path);
    pipe_pass_id_t getPassId(const char* path) const;

    void enableTechnique(const char* path, bool value);

    //
    void notifyRenderTargetDestroyed(gpuRenderTarget* rt);

    int dbg_getParamBlockUploadCount() const { return dbg_param_block_upload_count; }
};
