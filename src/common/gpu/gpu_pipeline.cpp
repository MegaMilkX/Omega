#include "gpu_pipeline.hpp"

#include "gpu/gpu.hpp"
#include "gpu/gpu_util.hpp"
#include "platform/platform.hpp"
#include "gpu/render_bucket.hpp"
#include "gpu/program_lib.hpp"


void gpuPipeline::makeDefaultPassProgram(gpuPass* pass) {
    if (pass->base_shader_sets.empty()) {
        LOG_ERR("Pass " << pass->getId() << " has no base shader sets");
        return;
    }
    std::vector<const gpuCompiledShader*> compiled;
    for (int j = 0; j < pass->base_shader_sets.size(); ++j) {
        auto set = pass->base_shader_sets[j].get();

        auto compiled_set = set->getCompiled(0/* flags */);
        if (!compiled_set) {
            LOG_ERR("Failed to compile shader set " << j << " for pass " << pass->getId());
            assert(false);
            continue;
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
            sampler.source = SHADER_SAMPLER_SOURCE_GPU;
            sampler.type = tex_desc->type;
            sampler.slot = slot;
            sampler.texture_id = tex_desc->texture;
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

            ShaderSamplerSet::Sampler sampler;
            sampler.source = SHADER_SAMPLER_SOURCE_CHANNEL_IDX;
            sampler.type = SHADER_SAMPLER_TEXTURE2D;
            sampler.slot = slot;
            sampler.pipe_channel_index = getChannelIndex(ch_desc->pipeline_channel_name.c_str());
            sampler_set->add(sampler);
        }
    }
}
void gpuPipeline::updateRenderSequence(gpuRenderSequence* seq) {
    std::vector<int> lwt_array(channelCount());
    std::fill(lwt_array.begin(), lwt_array.end(), 0);

    // Set 'last written to' indices for double buffered channels
    for (int i = 0; i < seq->passes.size(); ++i) {
        gpuPassInstance* pass_inst = &seq->passes[i];
        gpuPass* pass = pass_inst->pass;
        if (pass->hasAnyFlags(PASS_FLAG_DISABLED)) {
            continue;
        }

        for (int j = 0; j < pass->channelCount(); ++j) {
            gpuPass::ChannelDesc* ch_desc = pass->getChannelDesc(j);
            gpuPassInstance::ChannelDesc* instance_ch_desc = &pass_inst->channels[j];
            const RenderChannel* pipeline_channel = getChannel(ch_desc->pipe_channel_idx);

            int& lwt = lwt_array[ch_desc->pipe_channel_idx];
            instance_ch_desc->lwt_buffer_idx = lwt;

            if (!pipeline_channel->is_double_buffered) {
                continue;
            }

            if (ch_desc->reads && ch_desc->writes) {
                lwt = (lwt + 1) % 2;
            }
        }
    }
}
/*
void gpuPipeline::createFramebuffers(gpuRenderTarget* rt, gpuRenderSequence* seq) {
    LOG("Deleting old framebuffers");

    assert(seq);
    std::span<gpuPassInstance> passes = seq->passes;

    rt->framebuffers.clear();
    rt->framebuffers.resize(passes.size());


    LOG("Creating framebuffers");
    // TODO: DOUBLE BUFFERED RT LAYERS
    // READ + WRITE = read from the last written to, WRITE becomes lwt (last written to)
    // WRITE = write to the last written to, lwt does not change
    // READ = read from the last written to, lwt does not change

    for (int i = 0; i < rt->layers.size(); ++i) {
        rt->layers[i].lwt = 0;
    }
    for (int j = 0; j < passes.size(); ++j) {
        gpuPassInstance& pass_inst = passes[j];
        auto pass = pass_inst.pass;

        if (pass->hasAnyFlags(PASS_FLAG_DISABLED)) {
            continue;
        }

        auto fb = new gpuFrameBuffer;
        rt->framebuffers[j].reset(fb);

        assert(pass->channelCount() <= platformGeti(PLATFORM_MAX_FRAMEBUFFER_COLOR_LAYERS));

        for (int k = 0; k < pass->channelCount(); ++k) {
            const gpuPass::ChannelDesc* ch_desc = pass->getChannelDesc(k);
            const gpuPassInstance::ChannelDesc* instance_ch_desc = &pass_inst.channels[k];
            const std::string& ch_name = ch_desc->pipeline_channel_name;
            const gpuPipeline::RenderChannel* pipeline_channel = rt->getPipeline()->getChannel(ch_desc->pipe_channel_idx);
            gpuRenderTarget::TextureLayer& rt_layer = rt->layers[instance_ch_desc->render_target_channel_idx];

            if (!ch_desc->writes) {
                continue;
            }

            rt_layer.lwt = 0;
            if (pass->hasFlags(PASS_FLAG_CLEAR_PASS)) {
                assert(!ch_desc->target_local_name.empty());

                if (pipeline_channel->is_double_buffered) {
                    // NOTE: two addColorTarget() with same name
                    // is ok (for now) since we do not use those names
                    // to retrieve buffers, only retrieve names using indices
                    fb->addColorTarget(
                        std::format("{}{}", ch_desc->target_local_name, 0).c_str(),
                        rt_layer.textures[0].get()
                    );
                    fb->addColorTarget(
                        std::format("{}{}", ch_desc->target_local_name, 1).c_str(),
                        rt_layer.textures[1].get()
                    );
                } else {
                    fb->addColorTarget(
                        ch_desc->target_local_name.c_str(),
                        rt_layer.textures[0].get()
                    );
                }
            } else if (ch_desc->reads && ch_desc->writes) {
                assert(!ch_desc->target_local_name.empty());
                if (!pipeline_channel->is_double_buffered) {
                    assert(false);
                    LOG_ERR("Misconfig: Render target layer '" << ch_name << "' is not double buffered, but a pass tries to use it as such");
                    continue;
                }

                fb->addColorTarget(
                    ch_desc->target_local_name.c_str(),
                    rt_layer.textures[(instance_ch_desc->lwt_buffer_idx + 1) % 2].get()
                );
                rt_layer.lwt = (instance_ch_desc->lwt_buffer_idx + 1) % 2;
            } else if(ch_desc->writes) {
                assert(!ch_desc->target_local_name.empty());
                fb->addColorTarget(
                    ch_desc->target_local_name.c_str(),
                    rt_layer.textures[instance_ch_desc->lwt_buffer_idx].get()
                );
                rt_layer.lwt = instance_ch_desc->lwt_buffer_idx;
            }
        }

        if (pass->hasDepthTarget()) {
            int depth_idx = pass_inst.depth_target_idx;
            fb->addDepthTarget(
                rt->layers[depth_idx].textures[0].get()
            );
        }

        if (!fb->validate()) {
            assert(false);
            LOG_ERR("FrameBuffer validation failed: pass " << j);
            continue;
        }
        //fb->prepare();
    }
}*/

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

    constexpr float FLT_INF = std::numeric_limits<float>().max();

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
    auto ptr = new gpuUniformBufferDesc();
    uniform_buffer_descs.emplace_back(std::unique_ptr<gpuUniformBufferDesc>(ptr));
    uniform_buffer_descs_by_name[name] = uniform_buffer_descs.back().get();
    ptr->name(name);
    ptr->id = id;
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
    }

    LOG("Getting color targets from default pass programs...");
    for (int i = 0; i < linear_passes.size(); ++i) {
        auto pass = linear_passes[i];
        
        LOG_WARN(pass->full_name << " fragment outputs:");
        // Pull fragment output names from program and define used channels from that
        auto default_prog = pass->getProgram();
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

    rt->pipe_channel_to_layer.resize(channelCount());
    std::fill(rt->pipe_channel_to_layer.begin(), rt->pipe_channel_to_layer.end(), -1);

    LOG("Creating render target textures");
    for (int i = 0; i < render_channels.size(); ++i) {
        const uint32_t pipe_ch_idx = i;// seq->channels[i].pipe_channel_index;
        auto rtdesc = render_channels[pipe_ch_idx];
        rt->pipe_channel_to_layer[pipe_ch_idx] = i;

        int width = rt->width;
        int height = rt->height;
        if (rtdesc.explicit_width) {
            width = rtdesc.explicit_width;
        }
        if (rtdesc.explicit_height) {
            height = rtdesc.explicit_height;
        }

        gpuRenderTarget::TextureLayer layer;
        layer.textures[0].reset(new gpuTexture2d);
        if (rtdesc.is_double_buffered) {
            layer.textures[1].reset(new gpuTexture2d);
        }

        // TODO: DERIVE CHANNEL COUNT FROM FORMAT
        if (rtdesc.format == GL_RGB) {
            layer.textures[0]->changeFormat(rtdesc.format, width, height, 3);
            if (rtdesc.is_double_buffered) {
                layer.textures[1]->changeFormat(rtdesc.format, width, height, 3);
            }
        } else if (rtdesc.format == GL_RGBA) {
            layer.textures[0]->changeFormat(rtdesc.format, width, height, 4);
            if (rtdesc.is_double_buffered) {
                layer.textures[1]->changeFormat(rtdesc.format, width, height, 4);
            }
        } else if (rtdesc.format == GL_SRGB) {
            layer.textures[0]->changeFormat(rtdesc.format, width, height, 3);
            if (rtdesc.is_double_buffered) {
                layer.textures[1]->changeFormat(rtdesc.format, width, height, 3);
            }
        } else if(rtdesc.format == GL_RED) {
            layer.textures[0]->changeFormat(rtdesc.format, width, height, 1);
            if (rtdesc.is_double_buffered) {
                layer.textures[1]->changeFormat(rtdesc.format, width, height, 1);
            }
        } else if (rtdesc.format == GL_RG) {
            layer.textures[0]->changeFormat(rtdesc.format, width, height, 2);
            if (rtdesc.is_double_buffered) {
                layer.textures[1]->changeFormat(rtdesc.format, width, height, 2);
            }
        } else if(rtdesc.format == GL_RGB16F) {
            layer.textures[0]->changeFormat(rtdesc.format, width, height, 3, GL_FLOAT);
            if (rtdesc.is_double_buffered) {
                layer.textures[1]->changeFormat(rtdesc.format, width, height, 3, GL_FLOAT);
            }
        } else if(rtdesc.format == GL_RGBA16F) {
            layer.textures[0]->changeFormat(rtdesc.format, width, height, 4, GL_FLOAT);
            if (rtdesc.is_double_buffered) {
                layer.textures[1]->changeFormat(rtdesc.format, width, height, 4, GL_FLOAT);
            }
        } else if(rtdesc.format == GL_RGB32F) {
            layer.textures[0]->changeFormat(rtdesc.format, width, height, 3, GL_FLOAT);
            if (rtdesc.is_double_buffered) {
                layer.textures[1]->changeFormat(rtdesc.format, width, height, 3, GL_FLOAT);
            }
        } else if(rtdesc.format == GL_RGBA32F) {
            layer.textures[0]->changeFormat(rtdesc.format, width, height, 4, GL_FLOAT);
            if (rtdesc.is_double_buffered) {
                layer.textures[1]->changeFormat(rtdesc.format, width, height, 4, GL_FLOAT);
            }
        } else if(rtdesc.format == GL_DEPTH_COMPONENT) {
            layer.textures[0]->changeFormat(rtdesc.format, width, height, 1);
            if (rtdesc.is_double_buffered) {
                layer.textures[1]->changeFormat(rtdesc.format, width, height, 1);
            }
        } else {
            assert(false);
            LOG_ERR("Render target format not supported!");
        }

        layer.textures[0]->setWrapMode(rtdesc.wrap_mode);
        layer.textures[0]->setBorderColor(rtdesc.border_color);
        if (rtdesc.is_double_buffered) {
            layer.textures[1]->setWrapMode(rtdesc.wrap_mode);
            layer.textures[1]->setBorderColor(rtdesc.border_color);
        }

        if (rtdesc.is_depth) {
            if (rtdesc.is_double_buffered) {
                assert(false);
                LOG_ERR("!!! Depth texture cannot be double buffered !!!");
            }
            rt->depth_texture = layer.textures[0].get();
        }
        rt->layers.push_back(std::move(layer));
    }

    //createFramebuffers(rt, seq);
    
    rt->default_output_texture = output_target;// seq->getChannelIdx(output_target_name);

    render_targets.insert(rt);
    LOG("Render target initialized");
}
void gpuPipeline::initRenderTargetMap(
    gpuRenderTargetMap* map,
    gpuRenderTarget* rt,
    gpuRenderSequence* seq,
    std::initializer_list<std::pair<std::string, std::string>> overrides
) {
    LOG("Deleting old framebuffers");

    std::map<std::string, std::string> override_map;
    for (const auto& pair : overrides) {
        override_map.insert(pair);
    }

    assert(seq);
    std::span<gpuPassInstance> passes = seq->passes;

    map->target = rt;
    map->framebuffers.clear();
    map->framebuffers.resize(passes.size());

    LOG("Creating framebuffers");
    // TODO: DOUBLE BUFFERED RT LAYERS
    // READ + WRITE = read from the last written to, WRITE becomes lwt (last written to)
    // WRITE = write to the last written to, lwt does not change
    // READ = read from the last written to, lwt does not change
    /*
    for (int i = 0; i < rt->layers.size(); ++i) {
        rt->layers[i].lwt = 0;
    }*/
    std::vector<int>& lwt_array = map->lwt_array;
    lwt_array.resize(rt->layers.size());
    std::fill(map->lwt_array.begin(), map->lwt_array.end(), 0);

    for (int i = 0; i < passes.size(); ++i) {
        gpuPassInstance& pass_inst = passes[i];
        auto pass = pass_inst.pass;

        if (pass->hasAnyFlags(PASS_FLAG_DISABLED)) {
            continue;
        }

        auto fb = new gpuFrameBuffer;
        map->framebuffers[i].reset(fb);

        assert(pass->channelCount() <= platformGeti(PLATFORM_MAX_FRAMEBUFFER_COLOR_LAYERS));

        for (int j = 0; j < pass->channelCount(); ++j) {
            const gpuPass::ChannelDesc* ch_desc = pass->getChannelDesc(j);
            const char* ch_name = ch_desc->pipeline_channel_name.c_str();
            auto it = override_map.find(ch_name);
            if (it != override_map.end()) {
                ch_name = it->second.c_str();
            }
            const gpuPassInstance::ChannelDesc* instance_ch_desc = &pass_inst.channels[j];
            const int target_ch_idx = gpuGetPipeline()->getChannelIndex(ch_name);
            const int pipe_ch_idx = target_ch_idx;
            //const int target_ch_idx = instance_ch_desc->render_target_channel_idx;
            const gpuPipeline::RenderChannel* pipeline_channel = rt->getPipeline()->getChannel(pipe_ch_idx);
            /* const */ gpuRenderTarget::TextureLayer* rt_layer = &rt->layers[target_ch_idx];


            if (!ch_desc->writes) {
                continue;
            }

            lwt_array[target_ch_idx] = 0;
            if (pass->hasFlags(PASS_FLAG_CLEAR_PASS)) {
                assert(!ch_desc->target_local_name.empty());

                if (pipeline_channel->is_double_buffered) {
                    // NOTE: two addColorTarget() with same name
                    // is ok (for now) since we do not use those names
                    // to retrieve buffers, only retrieve names using indices
                    fb->addColorTarget(
                        std::format("{}{}", ch_desc->target_local_name, 0).c_str(),
                        rt_layer->textures[0].get()
                    );
                    fb->addColorTarget(
                        std::format("{}{}", ch_desc->target_local_name, 1).c_str(),
                        rt_layer->textures[1].get()
                    );
                } else {
                    fb->addColorTarget(
                        ch_desc->target_local_name.c_str(),
                        rt_layer->textures[0].get()
                    );
                }
            } else if (ch_desc->reads && ch_desc->writes) {
                assert(!ch_desc->target_local_name.empty());
                if (!pipeline_channel->is_double_buffered) {
                    assert(false);
                    LOG_ERR("Misconfig: Render target layer '" << ch_name << "' is not double buffered, but a pass tries to use it as such");
                    continue;
                }

                fb->addColorTarget(
                    ch_desc->target_local_name.c_str(),
                    rt_layer->textures[(instance_ch_desc->lwt_buffer_idx + 1) % 2].get()
                );
                lwt_array[target_ch_idx] = (instance_ch_desc->lwt_buffer_idx + 1) % 2;
            } else if(ch_desc->writes) {
                assert(!ch_desc->target_local_name.empty());
                fb->addColorTarget(
                    ch_desc->target_local_name.c_str(),
                    rt_layer->textures[instance_ch_desc->lwt_buffer_idx].get()
                );
                lwt_array[target_ch_idx] = instance_ch_desc->lwt_buffer_idx;
            }
        }

        if (pass->hasDepthTarget()) {
            const char* depth_name = pass_inst.depth_layer_name.c_str();
            auto it = override_map.find(depth_name);
            if (it != override_map.end()) {
                depth_name = it->second.c_str();
            }
            int depth_idx = gpuGetPipeline()->getChannelIndex(depth_name);
            fb->addDepthTarget(
                rt->layers[depth_idx].textures[0].get()
            );
        }

        if (!fb->validate()) {
            assert(false);
            LOG_ERR("FrameBuffer validation failed: pass " << i);
            continue;
        }
        //fb->prepare();
    }
}

EngineRenderView* gpuPipeline::createView(RendererType rtype, const gfxm::rect& rc) {
    // TODO: Store views, add removal
    auto renderer = getRenderer(rtype);
    if(!renderer) return nullptr;
    auto view = new EngineRenderView(rc, renderer, false);
    renderer->initView(view);
    views.push_back(std::unique_ptr<EngineRenderView>(view));
    return view;
}

EngineRenderView* gpuPipeline::createOffscreenView(RendererType rtype, int w, int h) {
    // TODO: Store views, add removal
    auto renderer = getRenderer(rtype);
    if(!renderer) return nullptr;
    auto view = new EngineRenderView(gfxm::rect(0, 0, 1, 1), renderer, true);
    renderer->initView(view);
    // TODO: SIZE? (w, h)
    views.push_back(std::unique_ptr<EngineRenderView>(view));
    return view;
}

void gpuPipeline::drawSingleView(EngineRenderView* rv, float time) {
    gpuRenderer* renderer = rv->getRenderer();
    gpuRenderTarget* target = rv->getRenderTarget();
    gpuRenderBucket* bucket = rv->getRenderBucket();

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
        glBindBufferBase(GL_UNIFORM_BUFFER, ub->getDesc()->id, gl_id);
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
        glBindBufferBase(GL_UNIFORM_BUFFER, ub->getDesc()->id, gl_id);
    }
}
void gpuPipeline::bindParamBlocks() {
    for (auto kv : param_blocks) {
        auto ub = kv.second->ubuf;
        GLint gl_id = ub->gpu_buf.getId();
        glBindBufferBase(GL_UNIFORM_BUFFER, ub->getDesc()->id, gl_id);
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
