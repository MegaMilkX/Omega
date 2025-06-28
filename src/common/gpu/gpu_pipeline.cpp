#include "gpu_pipeline.hpp"

#include "platform/platform.hpp"


void gpuPipeline::updatePassSequence() {
    for (int i = 0; i < channelCount(); ++i) {
        auto channel = getChannel(i);
        channel->lwt = 0;
    }

    // Set 'last written to' indices for double buffered channels
    for (int i = 0; i < linear_passes.size(); ++i) {
        auto pass = linear_passes[i];
        if (pass->hasAnyFlags(PASS_FLAG_DISABLED)) {
            continue;
        }

        for (int j = 0; j < pass->channelCount(); ++j) {
            gpuPass::ChannelDesc* ch_desc = pass->getChannelDesc(j);
            const RenderChannel* pipeline_channel = getChannel(ch_desc->render_target_channel_idx);

            int& lwt = getChannel(ch_desc->render_target_channel_idx)->lwt;
            ch_desc->lwt_buffer_idx = lwt;

            if (!pipeline_channel->is_double_buffered) {
                continue;
            }

            if (ch_desc->reads && ch_desc->writes) {
                lwt = (lwt + 1) % 2;
            }
        }
    }

    for (int i = 0; i < linear_passes.size(); ++i) {
        auto pass = linear_passes[i];

        // Shader sampler sets
        for (int j = 0; j < pass->shaderCount(); ++j) {
            const gpuShaderProgram* shader = pass->getShader(j);

            ShaderSamplerSet* sampler_set = pass->getSamplerSet(j);
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
                sampler.channel_idx
                    = ShaderSamplerSet::ChannelBufferIdx{ ch_desc->render_target_channel_idx, ch_desc->lwt_buffer_idx };
                sampler_set->add(sampler);
            }
        }
    }
}
void gpuPipeline::createFramebuffers(gpuRenderTarget* rt) {
    LOG("Deleting old framebuffers");

    rt->framebuffers.clear();
    rt->framebuffers.resize(linear_passes.size());

    LOG("Creating framebuffers");
    // TODO: DOUBLE BUFFERED RT LAYERS
    // READ + WRITE = read from the last written to, WRITE becomes lwt (last written to)
    // WRITE = write to the last written to, lwt does not change
    // READ = read from the last written to, lwt does not change

    //std::vector<int> lwt_data(rt->layers.size()); // Last written to indices, for double buffered layers
    //std::fill(lwt_data.begin(), lwt_data.end(), 0);
    for (int j = 0; j < linear_passes.size(); ++j) {
        auto pass = linear_passes[j];

        if (pass->hasAnyFlags(PASS_FLAG_DISABLED)) {
            continue;
        }

        auto fb = new gpuFrameBuffer;
        rt->framebuffers[j].reset(fb);

        for (int k = 0; k < pass->channelCount(); ++k) {
            const gpuPass::ChannelDesc* ch_desc = pass->getChannelDesc(k);
            const std::string& ch_name = ch_desc->pipeline_channel_name;
            const gpuPipeline::RenderChannel* pipeline_channel = rt->getPipeline()->getChannel(ch_desc->render_target_channel_idx);
            /* const */ gpuRenderTarget::TextureLayer& rt_layer = rt->layers[ch_desc->render_target_channel_idx];

            if (!ch_desc->writes) {
                continue;
            }

            if (pass->hasFlags(PASS_FLAG_CLEAR_PASS)) {
                assert(!ch_desc->target_local_name.empty());

                if (pipeline_channel->is_double_buffered) {
                    // NOTE: two addColorTarget() with same name
                    // is ok (for now) since we do not use those names
                    // to retrieve buffers, but retrieve names using indices
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
                    rt_layer.textures[(ch_desc->lwt_buffer_idx + 1) % 2].get()
                );
            } else if(ch_desc->writes) {
                assert(!ch_desc->target_local_name.empty());
                fb->addColorTarget(
                    ch_desc->target_local_name.c_str(),
                    rt_layer.textures[ch_desc->lwt_buffer_idx].get()
                );
            }
        }

        if (pass->hasDepthTarget()) {
            fb->addDepthTarget(
                rt->layers[pass->getDepthTargetTextureIndex()].textures[0]
            );
        }

        if (!fb->validate()) {
            assert(false);
            LOG_ERR("FrameBuffer validation failed");
            continue;
        }
        //fb->prepare();
    }
}

void gpuPipeline::addColorChannel(
    const char* name,
    GLint format,
    bool is_double_buffered,
    const gfxm::vec4& clear_color
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
        .lwt = 0,
        .is_depth = false,
        .is_double_buffered = is_double_buffered,
        .clear_color = clear_color
        });
    rt_map[name] = index;
}

void gpuPipeline::addDepthChannel(const char* name) {
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
        .lwt = 0,
        .is_depth = true,
        .is_double_buffered = false
        });
    rt_map[name] = index;
}

void gpuPipeline::setOutputChannel(const char* render_target_name) {
    auto it = rt_map.find(render_target_name);
    if (it == rt_map.end()) {
        LOG_ERR("setOutputSource(): render target '" << render_target_name << "' does not exist");
        assert(false);
        return;
    }
    output_target = it->second;
}

gpuPass* gpuPipeline::addPass(const char* path, gpuPass* pass) {
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

    pass->framebuffer_id = linear_passes.size();
    pass->id = linear_passes.size();
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

bool gpuPipeline::compile() {
    for (int i = 0; i < linear_passes.size(); ++i) {
        auto pass = linear_passes[i];

        // Set render target channel indices
        for (int j = 0; j < pass->channelCount(); ++j) {
            gpuPass::ChannelDesc* ch_desc = pass->getChannelDesc(j);
            const std::string& ch_name = ch_desc->pipeline_channel_name;
            auto it = rt_map.find(ch_name);
            if (it == rt_map.end()) {
                LOG_ERR("Pipeline channel '" << ch_name << "' does not exist");
                assert(false);
                ch_desc->render_target_channel_idx = -1;
                continue;
            }
            ch_desc->render_target_channel_idx = it->second;
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

    updatePassSequence();

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
    LOG("Pipeline was changed, updating");
    updatePassSequence();
    for (auto rt : render_targets) {
        createFramebuffers(rt);
    }
    is_pipeline_dirty = false;
}

void gpuPipeline::initRenderTarget(gpuRenderTarget* rt) {
    LOG("Initializing render target");
    rt->pipeline = this;
    rt->default_output_texture = output_target;

    LOG("Creating render target textures");
    for (int i = 0; i < render_channels.size(); ++i) {
        auto rtdesc = render_channels[i];

        gpuRenderTarget::TextureLayer layer;
        layer.textures[0] = HSHARED<gpuTexture2d>(HANDLE_MGR<gpuTexture2d>::acquire());
        if (rtdesc.is_double_buffered) {
            layer.textures[1] = HSHARED<gpuTexture2d>(HANDLE_MGR<gpuTexture2d>::acquire());
        }

        /*rt->textures.push_back(
        HSHARED<gpuTexture2d>(HANDLE_MGR<gpuTexture2d>::acquire())
        );*/
        // TODO: DERIVE CHANNEL COUNT FROM FORMAT
        if (rtdesc.format == GL_RGB) {
            layer.textures[0]->changeFormat(rtdesc.format, rt->width, rt->height, 3);
            layer.textures[0]->setWrapMode(GPU_TEXTURE_WRAP_CLAMP);
            if (rtdesc.is_double_buffered) {
                layer.textures[1]->changeFormat(rtdesc.format, rt->width, rt->height, 3);
                layer.textures[1]->setWrapMode(GPU_TEXTURE_WRAP_CLAMP);
            }
        } else if (rtdesc.format == GL_SRGB) {
            layer.textures[0]->changeFormat(rtdesc.format, rt->width, rt->height, 3);
            layer.textures[0]->setWrapMode(GPU_TEXTURE_WRAP_CLAMP);
            if (rtdesc.is_double_buffered) {
                layer.textures[1]->changeFormat(rtdesc.format, rt->width, rt->height, 3);
                layer.textures[1]->setWrapMode(GPU_TEXTURE_WRAP_CLAMP);
            }
        } else if(rtdesc.format == GL_RED) {
            layer.textures[0]->changeFormat(rtdesc.format, rt->width, rt->height, 1);
            layer.textures[0]->setWrapMode(GPU_TEXTURE_WRAP_CLAMP);
            if (rtdesc.is_double_buffered) {
                layer.textures[1]->changeFormat(rtdesc.format, rt->width, rt->height, 1);
                layer.textures[1]->setWrapMode(GPU_TEXTURE_WRAP_CLAMP);
            }
        } else if (rtdesc.format == GL_RG) {
            layer.textures[0]->changeFormat(rtdesc.format, rt->width, rt->height, 2);
            layer.textures[0]->setWrapMode(GPU_TEXTURE_WRAP_CLAMP);
            if (rtdesc.is_double_buffered) {
                layer.textures[1]->changeFormat(rtdesc.format, rt->width, rt->height, 2);
                layer.textures[1]->setWrapMode(GPU_TEXTURE_WRAP_CLAMP);
            }
        } else if(rtdesc.format == GL_RGB32F) {
            layer.textures[0]->changeFormat(rtdesc.format, rt->width, rt->height, 3, GL_FLOAT);
            layer.textures[0]->setWrapMode(GPU_TEXTURE_WRAP_CLAMP);
            if (rtdesc.is_double_buffered) {
                layer.textures[1]->changeFormat(rtdesc.format, rt->width, rt->height, 3, GL_FLOAT);
                layer.textures[1]->setWrapMode(GPU_TEXTURE_WRAP_CLAMP);
            }
        } else if(rtdesc.format == GL_RGBA32F) {
            layer.textures[0]->changeFormat(rtdesc.format, rt->width, rt->height, 4, GL_FLOAT);
            layer.textures[0]->setWrapMode(GPU_TEXTURE_WRAP_CLAMP);
            if (rtdesc.is_double_buffered) {
                layer.textures[1]->changeFormat(rtdesc.format, rt->width, rt->height, 4, GL_FLOAT);
                layer.textures[1]->setWrapMode(GPU_TEXTURE_WRAP_CLAMP);
            }
        } else if(rtdesc.format == GL_DEPTH_COMPONENT) {
            layer.textures[0]->changeFormat(rtdesc.format, rt->width, rt->height, 1);
            layer.textures[0]->setWrapMode(GPU_TEXTURE_WRAP_CLAMP);
            if (rtdesc.is_double_buffered) {
                layer.textures[1]->changeFormat(rtdesc.format, rt->width, rt->height, 1);
                layer.textures[1]->setWrapMode(GPU_TEXTURE_WRAP_CLAMP);
            }
        } else {
            assert(false);
            LOG_ERR("Render target format not supported!");
        }
        if (rtdesc.is_depth) {
            if (rtdesc.is_double_buffered) {
                assert(false);
                LOG_ERR("!!! Depth texture cannot be double buffered !!!");
            }
            rt->depth_texture = layer.textures[0].get();
        }
        rt->layers.push_back(layer);
    }

    createFramebuffers(rt);

    render_targets.insert(rt);
    LOG("Render target initialized");
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
int gpuPipeline::getFrameBufferIndex(const char* pass_path) {
    const gpuPass* pass = findPass(pass_path);
    if (!pass) {
        return -1;
    }
    return pass->framebuffer_id;
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

int gpuPipeline::uniformBufferCount() const {
    return uniform_buffer_descs.size();
}
int gpuPipeline::passCount() const {
    return linear_passes.size();
}

gpuUniformBufferDesc* gpuPipeline::getUniformBuffer(int i) {
    return uniform_buffer_descs[i].get();
}
gpuPass* gpuPipeline::getPass(int pass) {
    return linear_passes[pass];
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
