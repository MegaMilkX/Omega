#include "gpu_pipeline.hpp"

#include "gpu/gpu.hpp"
#include "gpu/gpu_util.hpp"
#include "platform/platform.hpp"
#include "gpu/render_bucket.hpp"
#include "gpu/program_lib.hpp"
#include "gpu/texture/array_texture2d.hpp"
#include "util/timer.hpp"


void gpuPipeline::makeDefaultPassProgram(gpuPass* pass) {
    if (pass->base_shader_sets.empty()) {
        LOG_ERR("Pass " << pass->getId() << " has no base shader sets");
        return;
    }

    PassShaderKey key;

    std::vector<const gpuCompiledShader*> compiled;
    for (int j = 0; j < pass->base_shader_sets.size(); ++j) {
        auto set = pass->base_shader_sets[j].get();

        auto compiled_set = gpuCompilePassShaderSet(set, key);
        if (!compiled_set) {
            LOG_ERR("Failed to compile shader set " << j << " for pass " << pass->getId());
            assert(false);
            continue;
        }
        for (int k = 0; k < compiled_set->shaders.size(); ++k) {
            compiled.push_back(compiled_set->shaders[k].get());
        }
    }

    {
        TransformShaderKey key;
        gpuShaderSet* to_world_shaders = gpuGetDevice()->getSharedResources()->getToWorldShader();
        auto compiled_set = gpuCompileTransformShaderSet(to_world_shaders, key);
        if (!compiled_set) {
            LOG_ERR("Failed to compile default to_world shader set");
            assert(false);
            return;
        }
        for (int k = 0; k < compiled_set->shaders.size(); ++k) {
            compiled.push_back(compiled_set->shaders[k].get());
        }
    }

    pass->default_program = gpuGetProgram(compiled.data(), compiled.size());
}

void gpuPipeline::updatePasses() {
    for (int i = 0; i < linear_passes.size(); ++i) {
        auto pass = linear_passes[i];
        gpuMakeDrawBuffersArray(
            pass,
            pass->default_program->getId(),
            pass->default_draw_buffers,
            sizeof(pass->default_draw_buffers) / sizeof(pass->default_draw_buffers[0])
        );
    }

    for (int i = 0; i < linear_passes.size(); ++i) {
        auto pass = linear_passes[i];

        const gpuShaderProgram* shader = pass->default_program.get();
        ShaderSamplerSet* sampler_set = &pass->sampler_set;

        sampler_set->clear();
        for (int k = 0; k < pass->textureCount(); ++k) {
            auto tex_desc = pass->getTextureDesc(k);
            int slot = shader->getDefaultSamplerSlot(tex_desc->sampler_name.c_str());
            if (slot < 0) {
                continue;
            }

            ShaderSamplerSet::Sampler sampler;
            if (tex_desc->type == SHADER_SAMPLER_TEXTURE2D_REF) {
                sampler.tex_ref = tex_desc->tex_ref;
            } else {
                sampler.texture_id = tex_desc->texture;
            }
            sampler.source = SHADER_SAMPLER_SOURCE_GPU;
            sampler.type = tex_desc->type;
            sampler.slot = slot;
            sampler_set->add(sampler);
        }

        for (int k = 0; k < pass->channelCount(); ++k) {
            const gpuPass::ChannelDesc* ch_desc = pass->getChannelDesc(k);
            if (!ch_desc->reads) {
                continue;
            }
            const std::string& ch_name = ch_desc->source_local_name;

            // TODO: Use a prefix for glsl sampler names
            int slot = shader->getDefaultSamplerSlot(ch_name.c_str());
            if (slot < 0) {
                continue;
            }

            int chidx = getChannelIndex(ch_desc->pipeline_channel_name.c_str());
            auto ch = getChannel(chidx);
            SHADER_SAMPLER_TYPE sampler_type = SHADER_SAMPLER_TEXTURE2D;
            if (ch->depth > 1) {
                sampler_type = SHADER_SAMPLER_TEXTURE2DARRAY;
            }

            ShaderSamplerSet::Sampler sampler;
            sampler.source = SHADER_SAMPLER_SOURCE_CHANNEL_IDX;
            sampler.type = sampler_type;
            sampler.slot = slot;
            sampler.pipe_channel_index = chidx;
            sampler_set->add(sampler);
        }
    }
}

bool gpuPipeline::validateProgram(gpuShaderProgram* prog) {
    if (!prog) {
        return false;
    }

    bool ret = true;
    for (int i = 0; i < uniform_buffer_descs.size(); ++i) {
        auto desc = uniform_buffer_descs[i].get();
        auto result = prog->validateUniformBlock(desc);
        if (result != UNIFORM_BLOCK_OK && result != UNIFORM_BLOCK_ABSENT) {
            LOG_ERR("Uniform block validation error: " << result);
            assert(false);
            ret = false;
            continue;
        }
    }
    return true;
}

void gpuPipeline::addColorChannel(
    const char* name,
    GLint format,
    bool is_double_buffered,
    GPU_TEXTURE_WRAP wrap_mode,
    int explicit_width,
    int explicit_height,
    const gfxm::vec4& border_color
) {
    auto it = rt_map.find(name);
    if (it != rt_map.end()) {
        assert(false);
        LOG_ERR("Color render target " << name << " already exists");
        return;
    }
    int index = render_channels.size();
    render_channels.push_back(RenderChannel{
        .name = name,
        .format = format,
        .is_depth = false,
        .is_double_buffered = is_double_buffered,
        .wrap_mode = wrap_mode,
        .border_color = border_color,
        .clear_color = gfxm::vec3(.0f, .0f, .0f),
        .explicit_width = explicit_width,
        .explicit_height = explicit_height
    });
    rt_map[name] = index;
}

void gpuPipeline::addDepthChannel(
    const char* name,
    int explicit_width, int explicit_height,
    GPU_TEXTURE_WRAP wrap_mode,
    const gfxm::vec4& border_color
) {
    auto it = rt_map.find(name);
    if (it != rt_map.end()) {
        assert(false);
        LOG_ERR("Depth render target " << name << " already exists");
        return;
    }

    int index = render_channels.size();
    render_channels.push_back(RenderChannel{
        .name = name,
        .format = GL_DEPTH_COMPONENT,
        .is_depth = true,
        .is_double_buffered = false,
        .wrap_mode = wrap_mode,
        .border_color = border_color,
        .clear_color = gfxm::vec3(.0f, .0f, .0f),
        .explicit_width = explicit_width,
        .explicit_height = explicit_height
    });
    rt_map[name] = index;
}

void gpuPipeline::addDepthArrayChannel(
    const char* name,
    int explicit_width, int explicit_height, int depth,
    GPU_TEXTURE_WRAP wrap_mode,
    const gfxm::vec4& border_color
) {
    auto it = rt_map.find(name);
    if (it != rt_map.end()) {
        assert(false);
        LOG_ERR("Depth render target " << name << " already exists");
        return;
    }
    
    int index = render_channels.size();
    render_channels.push_back(RenderChannel{
        .name = name,
        .format = GL_DEPTH_COMPONENT32F, // glTexStorage3D accepts only sized formats, no GL_DEPTH_COMPONENT
        .is_depth = true,
        .is_double_buffered = false,
        .wrap_mode = wrap_mode,
        .border_color = border_color,
        .clear_color = gfxm::vec3(.0f, .0f, .0f),
        .explicit_width = explicit_width,
        .explicit_height = explicit_height,
        .depth = depth
    });
    rt_map[name] = index;
}

void gpuPipeline::setOutputChannel(const char* render_target_name) {
    output_target_name = render_target_name;
    auto it = rt_map.find(render_target_name);
    if (it == rt_map.end()) {
        LOG_ERR("setOutputSource(): render target '" << render_target_name << "' does not exist");
        assert(false);
        return;
    }
    output_target = it->second;
}

gpuPass* gpuPipeline::addPass(const char* path, gpuPass* pass, int layer) {
    const std::vector<char> t = { '/', '\\' };
    std::string spath(path);
    auto it = spath.begin();
    if (spath[0] == '/' || spath[0] == '\\') {
        assert(false);
        ++it;
    }

    gpuPipelineBranch* tech_branch = &pipeline_root;

    std::string node_name;
    while(it != spath.end()) {
        auto prev_it = it;
        it = std::find_first_of(it, spath.end(), t.begin(), t.end());

        node_name = std::string(prev_it, it);

        if(it != spath.end()) {
            ++it;
        }

        if (it == spath.end()) {
            break;
        }

        LOG_DBG("Branch: " << node_name);
        tech_branch = tech_branch->getOrCreateBranch(node_name);
    }
    LOG_DBG("Pass: " << node_name);

    if(!tech_branch->addPass(node_name, pass)) {
        LOG_ERR("Failed to add pass " << node_name);
        return 0;
    }

    pass->id = linear_passes.size();
    pass->full_name = path;
    linear_passes.push_back(pass);
    return pass;
}

gpuUniformBufferDesc* gpuPipeline::createUniformBufferDesc(const char* name) {
    auto it = uniform_buffer_descs_by_name.find(name);
    if (it != uniform_buffer_descs_by_name.end()) {
        assert(false);
        return 0;
    }

    const int max_bindings = platformGeti(PLATFORM_MAX_UNIFORM_BUFFER_BINDINGS);
    if (uniform_buffer_descs.size() >= max_bindings) {
        LOG_ERR("Uniform buffer binding limit reached");
        assert(false);
        return 0;
    }

    int id = uniform_buffer_descs.size();
    auto ptr = new gpuUniformBufferDesc(name);
    uniform_buffer_descs.emplace_back(std::unique_ptr<gpuUniformBufferDesc>(ptr));
    uniform_buffer_descs_by_name[name] = uniform_buffer_descs.back().get();
    return ptr;
}

gpuUniformBufferDesc* gpuPipeline::getUniformBufferDesc(const char* name) {
    auto it = uniform_buffer_descs_by_name.find(name);
    if (it == uniform_buffer_descs_by_name.end()) {
        return 0;
    }
    return it->second;
}

gpuUniformBuffer* gpuPipeline::createUniformBuffer(const char* name) {
    auto desc = getUniformBufferDesc(name);
    if (!desc) {
        return 0;
    }
    return createUniformBuffer(desc);
}

gpuUniformBuffer* gpuPipeline::createUniformBuffer(gpuUniformBufferDesc* desc) {
    auto ub = new gpuUniformBuffer(desc);
    uniform_buffers.push_back(std::unique_ptr<gpuUniformBuffer>(ub));
    return ub;
}

void gpuPipeline::destroyUniformBuffer(gpuUniformBuffer* buf) {
    for (int i = 0; i < uniform_buffers.size(); ++i) {
        if (uniform_buffers[i].get() == buf) {
            uniform_buffers.erase(uniform_buffers.begin() + i);
            break;
        }
    }
}

void gpuPipeline::attachUniformBuffer(gpuUniformBuffer* buf) {
    attached_uniform_buffers.push_back(buf);
}
bool gpuPipeline::isUniformBufferAttached(const char* name) {
    for (int i = 0; i < attached_uniform_buffers.size(); ++i) {
        if (attached_uniform_buffers[i]->getDesc()->getName() == std::string(name)) {
            return true;
        }
    }
    return false;
}

gpuPipeline& gpuPipeline::attachParamBlock(gpuParamBlock* block) {
    auto t = block->getMgr()->getBlockType();
    param_blocks[t] = block;
    return *this;
}

bool gpuPipeline::compile() {
    for (int i = 0; i < linear_passes.size(); ++i) {
        auto pass = linear_passes[i];
        makeDefaultPassProgram(pass);
        if (!validateProgram(pass->getProgram())) {
            LOG_ERR("validateProgram for pass " << i << " failed");
        }
    }

    LOG("Getting color targets from default pass programs...");
    for (int i = 0; i < linear_passes.size(); ++i) {
        auto pass = linear_passes[i];
        
        LOG_WARN(pass->full_name << " fragment outputs:");
        // Pull fragment output names from program and define used channels from that
        auto default_prog = pass->getProgram();
        if (!default_prog) {
            LOG_ERR("No default program for pass " << i);
            continue;
        }
        if (!pass->disable_auto_targets && default_prog) {
            for (int i = 0; i < default_prog->outputCount(); ++i) {
                auto& out = default_prog->getOutput(i);
                std::string name_no_prefix;
                
                if (out.name.starts_with("gl_")) {
                    // TODO: Should skip those in enumFragmentOutputLocations so they don't appear here
                    continue;
                } else if (out.name.starts_with("out")) {
                    name_no_prefix = out.name.substr(3);
                } else {
                    name_no_prefix = out.name;
                }

                if (name_no_prefix.empty()) {
                    LOG_ERR("\tEmpty fragment output name after trimming prefix: '" << out.name << "'");
                    continue;
                }

                if (auto desc = pass->getChannelDescByShaderTargetName(name_no_prefix)) {
                    // Already declared to be used by this pass
                    LOG_WARN("\t" << name_no_prefix << ": already mapped to " << desc->pipeline_channel_name << " on cpp side");
                    continue;
                }

                auto it = rt_map.find(name_no_prefix);
                if (it == rt_map.end()) {
                    LOG_ERR("\t" << name_no_prefix << ": not a known pipeline layer name");
                    continue;
                }

                LOG("\t" << name_no_prefix);
                pass->setColorTarget(name_no_prefix.c_str(), name_no_prefix.c_str());
            }
        }

        // Set render target channel indices
        for (int j = 0; j < pass->channelCount(); ++j) {
            gpuPass::ChannelDesc* ch_desc = pass->getChannelDesc(j);
            const std::string& ch_name = ch_desc->pipeline_channel_name;
            auto it = rt_map.find(ch_name);
            if (it == rt_map.end()) {
                LOG_ERR("Pipeline channel '" << ch_name << "' does not exist");
                assert(false);
                ch_desc->pipe_channel_idx = -1;
                continue;
            }
            ch_desc->pipe_channel_idx = it->second;
        }

        // Depth target
        const auto& tgt_name = pass->getDepthTargetGlobalName();
        if (!tgt_name.empty()) {
            auto it = rt_map.find(tgt_name);
            if (it == rt_map.end()) {
                LOG_ERR("Depth target '" << tgt_name << "' does not exist");
                assert(false);
                pass->setDepthTargetTextureIndex(-1);
            } else {
                pass->setDepthTargetTextureIndex(it->second);
            }
        }
    }

    updatePasses();

    for (int i = 0; i < linear_passes.size(); ++i) {
        linear_passes[i]->onCompiled(this);
    }

    is_pipeline_dirty = false;
    return true;
}
void gpuPipeline::updateDirty() {
    if (!is_pipeline_dirty) {
        return;
    }
    LOG("Rendering pipeline was changed, updating");
    //updatePassSequence();
    for (auto rt : render_targets) {
        // TODO:
        //createFramebuffers(rt, rt->getSequence());
    }
    is_pipeline_dirty = false;
}

void gpuPipeline::updateParamBlocks() {
    dbg_param_block_upload_count = gpuGetDevice()->getParamBlockContext()->update();
}


void gpuPipeline::initRenderTarget(gpuRenderTarget* rt) {
    LOG("Initializing render target");
    rt->pipeline = this;

    rt->pipe_channel_to_local.resize(channelCount());
    std::fill(rt->pipe_channel_to_local.begin(), rt->pipe_channel_to_local.end(), -1);

    render_targets.insert(rt);
    LOG("Render target initialized");
}

class ChannelOverrides {
public:
    ChannelOverrides(std::initializer_list<std::pair<std::string, std::string>> list)
        : map(list.begin(), list.end()) {}
    ChannelOverrides(const std::map<std::string, std::string>& map)
        : map(map) {}

    const std::string& resolve(const std::string& pipeline_channel_name) const {
        auto it = map.find(pipeline_channel_name);
        return it == map.end() ? pipeline_channel_name : it->second;
    }
private:
    std::map<std::string, std::string> map;
};

static void ensureTextures(gpuRenderTarget* rt, std::span<gpuPassInstance> passes, const ChannelOverrides& ov) {
    for (gpuPassInstance& inst : passes) {
        gpuPass* pass = inst.pass;
        if (pass->hasAnyFlags(PASS_FLAG_DISABLED)) {
            continue;
        }
        for (int j = 0; j < pass->channelCount(); ++j) {
            const gpuPass::ChannelDesc* d = pass->getChannelDesc(j);
            // Only read+write channels are ping-ponged, so only they need a second buffer
            rt->ensureChannel(ov.resolve(d->pipeline_channel_name), d->reads && d->writes);
        }
        if (pass->hasDepthTarget()) {
            rt->ensureChannel(ov.resolve(pass->getDepthTarget()->global_name), false);
        }
    }
}

// gpuBindSamplers() uses rt_chan_to_pass to find a pass channel's lwt_buffer_idx
static void mapPassChannels(gpuRenderTarget* rt, std::span<gpuPassInstance> passes, const ChannelOverrides& ov) {
    for (gpuPassInstance& inst : passes) {
        if (inst.pass->hasAnyFlags(PASS_FLAG_DISABLED)) {
            continue;
        }

        inst.rt_chan_to_pass.assign(rt->channels.size(), -1);

        for (int j = 0; j < (int)inst.channels.size(); ++j) {
            gpuPassInstance::ChannelDesc& c = inst.channels[j];

            auto* layer = rt->getChannel(ov.resolve(c.name));
            if (!layer) {
                LOG_ERR("Pass " << inst.pass->getFullName() << ": channel '" << c.name << "' missing in render target");
                assert(false);
                continue;
            }
            c.render_target_channel_idx = layer->idx;
            inst.rt_chan_to_pass[layer->idx] = j;
        }

        if (inst.pass->hasDepthTarget()) {
            auto name = ov.resolve(inst.pass->getDepthTarget()->global_name);
            auto* layer = rt->getChannel(name);
            if (!layer) {
                LOG_ERR("Pass " << inst.pass->getFullName() << ": depth channel '" << name << "' missing in render target");
                assert(false);
                continue;
            }
            inst.depth_layer_name = name;
            inst.depth_target_idx = layer->idx;
        }
    }
}

static int countInvocations(gpuRenderTarget* rt, gpuPassInstance& pass_inst, const ChannelOverrides& ov) {
    gpuPass* pass = pass_inst.pass;

    int n = 1;
    auto accumulate = [&](gpuRenderTarget::TextureLayer* layer) -> bool {
        if (!layer) {
            assert(false);
            return false;
        }
        gpuTexture* tex = layer->textures[0].get();
        const gpuTextureType type = tex->getType();
        if (type == gpuTextureType::Tex2d) {
            return true;
        }
        if (type != gpuTextureType::Array2d) {
            LOG("Unsupported texture target type: " << int(type));
            assert(false);
            return false;
        }
        const int nlayers = tex->layerCount();
        if (n != 1 && n != nlayers) {
            LOG("Array target layer count must match other array targets in the framebuffer");
            assert(false);
            return false;
        }
        n = nlayers;
        return true;
    };

    for (int j = 0; j < pass->channelCount(); ++j) {
        const gpuPass::ChannelDesc* d = pass->getChannelDesc(j);
        if (!d->writes) {
            continue;
        }
        if (!accumulate(rt->getChannel(ov.resolve(d->pipeline_channel_name)))) {
            return 0;
        }
    }
    if (pass->hasDepthTarget()) {
        if (!accumulate(rt->getChannel(ov.resolve(pass->getDepthTarget()->global_name)))) {
            return 0;
        }
    }
    pass_inst.n_invocations = n;
    return n;
}
/*
static bool buildPassFramebuffers(
    gpuRenderTarget* rt, gpuPassInstance& inst, const ChannelOverrides& ov, std::vector<int>& lwt_array
) {
    inst.framebuffers.clear();
    for (int k = 0; k < inst.n_invocations; ++k) {
        inst.framebuffers.push_back(buildFramebuffer(rt, inst, ov, k, lwt_array));
    }
    return inst.n_invocations != 0;
}*/

void gpuPipeline::initRenderTargetMap(gpuRenderTarget* rt, gpuRenderSequence* seq) {
    LOG("initRenderTargetMap");

    assert(seq);
    std::span<gpuPassInstance> passes = seq->passes;
    const std::map<std::string, std::string>& overrides = seq->overrides;
    
    ensureTextures(rt, passes, overrides);
    mapPassChannels(rt, passes, overrides);

    for (gpuPassInstance& inst : passes) {
        if (inst.pass->hasAnyFlags(PASS_FLAG_DISABLED)) {
            continue;
        }
        countInvocations(rt, inst, overrides);
    }
}

EngineRenderView* gpuPipeline::createView(RendererType rtype, const gfxm::rect& rc) {
    // TODO: Store views, add removal
    auto view = new EngineRenderView(rc, rtype, false);
    views.push_back(std::unique_ptr<EngineRenderView>(view));
    return view;
}

EngineRenderView* gpuPipeline::createOffscreenView(RendererType rtype, int w, int h) {
    // TODO: Store views, add removal
    auto view = new EngineRenderView(gfxm::rect(0, 0, 1, 1), rtype, true);
    // TODO: SIZE? (w, h)
    views.push_back(std::unique_ptr<EngineRenderView>(view));
    return view;
}

void gpuPipeline::destroyView(EngineRenderView* v) {
    for (int i = 0; i < views.size(); ++i) {
        if (views[i].get() != v) {
            continue;
        }
        views.erase(views.begin() + i);
        break;
    }
}

void gpuPipeline::drawSingleView(EngineRenderView* rv, float time) {
    gpuRenderer* renderer = rv->getRenderer();
    gpuRenderTarget* target = rv->getRenderTarget();
    gpuRenderBucket* bucket = rv->getRenderBucket();

    timer timer_query;
    timer_query.start();
    for (int j = 0; j < rv->queryInterfaceCount(); ++j) {
        auto qi = rv->getQueryInterface(j);
        if (!qi) {
            assert(false);
            continue;
        }
        VisibilityQuery vq(rv->getProjection(), rv->getViewTransform(), 0);
        GeometryQuery query_geo(vq, bucket);
        qi->queryGeometry(query_geo);
    }
    rv->stats.geom_query_time = timer_query.stop();

    DRAW_PARAMS params = {
        .view = rv->getViewTransform(),
        .view_prev = rv->getViewTransform(), // TODO: motion blur
        .projection = rv->getProjection(),
        .vp_rect_ratio = rv->getRect(),
        .viewport_x = (int)(target->getWidth() * rv->getRect().min.x),
        .viewport_y = (int)(target->getHeight() * rv->getRect().min.y),
        .viewport_width = (int)(target->getWidth() * (rv->getRect().max.x - rv->getRect().min.x)),
        .viewport_height = (int)(target->getHeight() * (rv->getRect().max.y - rv->getRect().min.y)),
        .time = time
    };

    renderer->draw(bucket, rv, params);
}

void gpuPipeline::draw(gpuRenderTarget* target, gpuRenderBucket* bucket, const DRAW_PARAMS& params) {
    bucket->sort(params);
    bindUniformBuffers();

    for (auto kv : param_blocks) {
        auto ub = kv.second->ubuf;
        GLint gl_id = ub->gpu_buf.getId();
        glBindBufferBase(GL_UNIFORM_BUFFER, ub->getDesc()->binding_location, gl_id);
    }
    /*
    for (int i = 0; i < linear_passes.size(); ++i) {
        auto pass = linear_passes[i];
        
        if (pass->hasAnyFlags(PASS_FLAG_DISABLED | PASS_FLAG_NO_DRAW)) {
            continue;
        }

        pass->onDraw(target, bucket, i, params);
    }*/
}

int gpuPipeline::channelCount() const {
    return render_channels.size();
}
gpuPipeline::RenderChannel* gpuPipeline::getChannel(int i) {
    return &render_channels[i];
}
const gpuPipeline::RenderChannel* gpuPipeline::getChannel(int i) const {
    return &render_channels[i];
}
int gpuPipeline::getChannelIndex(const char* name) {
    auto it = rt_map.find(name);
    if (it == rt_map.end()) {
        assert(false);
        LOG_ERR("getChannelIndex(): " << name << " does not exist");
        return -1;
    }
    return it->second;
}

gpuMaterial* gpuPipeline::createMaterial() {
    auto ptr = new gpuMaterial();
    return ptr;
}

void gpuPipeline::bindUniformBuffers() {
    for (int i = 0; i < attached_uniform_buffers.size(); ++i) {
        auto& ub = attached_uniform_buffers[i];
        GLint gl_id = ub->gpu_buf.getId();
        glBindBufferBase(GL_UNIFORM_BUFFER, ub->getDesc()->binding_location, gl_id);
    }
}
void gpuPipeline::bindParamBlocks() {
    for (auto kv : param_blocks) {
        auto ub = kv.second->ubuf;
        GLint gl_id = ub->gpu_buf.getId();
        glBindBufferBase(GL_UNIFORM_BUFFER, ub->getDesc()->binding_location, gl_id);
    }
}

int gpuPipeline::uniformBufferCount() const {
    return uniform_buffer_descs.size();
}
int gpuPipeline::passCount() const {
    return linear_passes.size();
}

gpuUniformBufferDesc* gpuPipeline::getUniformBuffer(int i) {
    return uniform_buffer_descs[i].get();
}
gpuPass* gpuPipeline::getPass(pipe_pass_id_t i) {
    return linear_passes[i];
}

const gpuPass* gpuPipeline::findPass(const char* path) const {
    const std::vector<char> t = { '/', '\\' };
    std::string spath(path);
    auto it = spath.begin();
    if (spath[0] == '/' || spath[0] == '\\') {
        assert(false);
        ++it;
    }

    const gpuPipelineBranch* tech_branch = &pipeline_root;

    std::string node_name;
    while(it != spath.end()) {
        auto prev_it = it;
        it = std::find_first_of(it, spath.end(), t.begin(), t.end());

        node_name = std::string(prev_it, it);

        if(it != spath.end()) {
            ++it;
        }

        if (it == spath.end()) {
            break;
        }

        tech_branch = tech_branch->getBranch(node_name);
        if (!tech_branch) {
            return 0;
        }
    }

    return tech_branch->getPass(node_name);
}

gpuPipelineNode* gpuPipeline::findNode(const char* path) {
    const std::vector<char> t = { '/', '\\' };
    std::string spath(path);
    auto it = spath.begin();
    if (spath[0] == '/' || spath[0] == '\\') {
        assert(false);
        ++it;
    }

    gpuPipelineNode* node = &pipeline_root;

    std::string node_name;
    while(it != spath.end()) {
        auto prev_it = it;
        it = std::find_first_of(it, spath.end(), t.begin(), t.end());

        node_name = std::string(prev_it, it);

        if(it != spath.end()) {
            ++it;
        }

        if (it == spath.end()) {
            break;
        }

        node = node->getChild(node_name);
        if (!node) {
            return 0;
        }
    }

    return node->getChild(node_name);
}

pipe_pass_id_t gpuPipeline::getPassId(const char* path) const {
    auto pass = findPass(path);
    if (!pass) {
        return -1;
    }
    return pass->getId();
}

void gpuPipeline::enableTechnique(const char* path, bool value) {
    auto node = findNode(path);
    if (!node) {
        LOG_ERR("enableTechnique: path '" << path << "' does not exist");
        assert(false);
        return;
    }
    node->enable(value);
    is_pipeline_dirty = true;
}


void gpuPipeline::notifyRenderTargetDestroyed(gpuRenderTarget* rt) {
    render_targets.erase(rt);
}
