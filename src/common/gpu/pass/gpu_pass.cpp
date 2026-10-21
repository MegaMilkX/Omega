#include "gpu_pass.hpp"

int gpuDbgScopedGroup::msg_counter = 0;


static bool attachColorTarget(
    gpuFrameBuffer* fb,
    int attachment_index,
    const std::string& name,
    gpuTexture* texture,
    int layer
) {
    auto type = texture->getType();
    if (type == gpuTextureType::Tex2d) {
        gpuTexture2d* tex2d = dynamic_cast<gpuTexture2d*>(texture);
        fb->addColorTarget(attachment_index, name.c_str(), tex2d);
    } else if (type == gpuTextureType::Array2d) {
        gpuArrayTexture2d* array_tex2d = dynamic_cast<gpuArrayTexture2d*>(texture);
        fb->addColorTarget(attachment_index, name.c_str(), array_tex2d, layer);
    } else {
        LOG_ERR("Unsupported color target: " << int(type));
        assert(false);
        return false;
    }
    return true;
}
static void attachClearChannel(
    gpuFrameBuffer* fb, const gpuPass::ChannelDesc* d,
    gpuRenderTarget::TextureLayer* layer, int k, int& attachment_idx
) {
    if (layer->double_buffered) {
        attachColorTarget(fb, attachment_idx++, std::format("{}{}", d->target_local_name, 0), layer->textures[0].get(), k);
        attachColorTarget(fb, attachment_idx++, std::format("{}{}", d->target_local_name, 1), layer->textures[1].get(), k);
    } else {
        attachColorTarget(fb, attachment_idx++, d->target_local_name, layer->textures[0].get(), k);
    }
}

static void attachDepth(gpuFrameBuffer* fb, gpuTexture* tex, int k) {
    if (tex->getType() == gpuTextureType::Tex2d) {
        fb->addDepthTarget(dynamic_cast<gpuTexture2d*>(tex));
    } else if (tex->getType() == gpuTextureType::Array2d) {
        fb->addDepthTarget(dynamic_cast<gpuArrayTexture2d*>(tex), k);
    }
}

static std::unique_ptr<gpuFrameBuffer> buildFramebuffer(gpuRenderTarget* rt, gpuPassInstance& inst, uint32_t key, int k) {    
    gpuPass* pass = inst.pass;
    auto fb = std::make_unique<gpuFrameBuffer>();

    int color_attachment_idx = 0;

    for (int i = 0; i < inst.channels.size(); ++i) {
        const gpuPassInstance::ChannelDesc* ch_inst = &inst.channels[i];
        const gpuPass::ChannelDesc* ch_pass = pass->getChannelDesc(i);
        if (!ch_pass->writes) {
            continue;
        }
        gpuRenderTarget::TextureLayer* layer = rt->getChannel(ch_inst->render_target_channel_idx);

        if (pass->hasFlags(PASS_FLAG_CLEAR_PASS)) {
            LOG_DBG(std::format("\t[C{:}/L{:}] '{}'", color_attachment_idx, k, ch_pass->pipeline_channel_name));
            attachClearChannel(fb.get(), ch_pass, layer, k, color_attachment_idx);
        } else {
            const int ibuf = layer->double_buffered ? int((key >> i) & 1u) : 0;
            LOG_DBG(std::format("\t[C{:}/L{:}/B{:}] '{}'", ch_pass->fb_attachment_idx, k, ibuf, ch_pass->pipeline_channel_name));
            attachColorTarget(
                fb.get(), ch_pass->fb_attachment_idx, ch_pass->target_local_name,
                layer->textures[ibuf].get(), k
            );
        }
    }

    if (pass->hasDepthTarget()) {
        auto* layer = rt->getChannel(inst.depth_target_idx);
        assert(layer);
        LOG_DBG(std::format("\t[D/L{:}/B{:}] '{}'", k, 0, inst.depth_layer_name));
        attachDepth(fb.get(), layer->textures[0].get(), k);
    }

    if (!fb->validate()) {
        LOG_ERR("FrameBuffer validation failed: pass " << pass->getFullName() << ", layer " << k);
        //assert(false);
    }
    return fb;
}

void gpuPassInstance::selectFramebuffers(gpuRenderTarget* tgt) {
    uint32_t key = 0;
    const bool is_clear_pass = pass->hasAnyFlags(PASS_FLAG_CLEAR_PASS);
    if (!is_clear_pass) {
        for (int i = 0; i < int(channels.size()); ++i) {
            const ChannelDesc& ch_inst = channels[i];
            const gpuPass::ChannelDesc* ch_pass = pass->getChannelDesc(i);
            if (!ch_pass->writes) {
                continue;
            }
            gpuRenderTarget::TextureLayer* layer = tgt->getChannel(ch_inst.render_target_channel_idx);
            if (!layer || !layer->double_buffered) {
                continue;
            }
            const int write_buf = ch_pass->reads ? (layer->lwt ^ 1) : layer->lwt;
            key |= uint32_t(write_buf) << i;
        }
    }

    auto& set = fb_sets[key];
    if (!set) {
        LOG_DBG(pass->getFullName() << ": new framebuffer set, key " << key);
        set.reset(new fb_set_t);
        for (int k = 0; k < n_invocations; ++k) {
            set->push_back(buildFramebuffer(tgt, *this, key, k));
        }
    }
    framebuffers = set.get();
}

void gpuPassInstance::commit(gpuRenderTarget* tgt) {
    for (int i = 0; i < pass->channelCount(); ++i) {
        gpuPass::ChannelDesc* ch = pass->getChannelDesc(i);
        if (!ch->writes) {
            continue;
        }

        ChannelDesc* ch_inst = &channels[i];
        
        auto* layer = tgt->getChannel(ch_inst->render_target_channel_idx);
        if (!layer || !layer->double_buffered) {
            continue;
        }

        if (pass->hasAnyFlags(PASS_FLAG_CLEAR_PASS)) {
            layer->lwt = 0;
        } else if(ch->reads) {
            layer->lwt ^= 1;
        }
    }
}

void gpuPass::addBaseShaderSet(const ResourceRef<gpuShaderSet>& shaders) {
    base_shader_sets.push_back(shaders);
    // TODO: invalidate?
}
gpuShaderProgram* gpuPass::getProgram() {
    return default_program.get();
}

void gpuPass::sortCommands(gpuRenderCmd* commands, size_t count, const DRAW_PARAMS& params) {
    switch (sort_mode) {
    case GPU_SORT_MODE::NONE:
        std::sort(commands, commands + count, [](const gpuRenderCmd& a, const gpuRenderCmd& b)->bool {
            return a.layer < b.layer;
        });
        break;
    case GPU_SORT_MODE::STATE_CHANGE: {
        std::sort(commands, commands + count, [](const gpuRenderCmd& a, const gpuRenderCmd& b)->bool {
            if(a.layer == b.layer) {
                if (a.program_id == b.program_id) {
                    if (a.state_id == b.state_id) {
                        return a.sampler_set_id < b.sampler_set_id;
                    }
                    return a.state_id < b.state_id;
                }
                return a.program_id < b.program_id;
            }
            return a.layer < b.layer;
        });
        break;
    }
    case GPU_SORT_MODE::FRONT_TO_BACK: {
        const gfxm::mat4 cam_transform = gfxm::inverse(params.view);
        const gfxm::vec3 cam_forward = -cam_transform[2];
        const gfxm::vec3 cam_pos = cam_transform[3];
        for (int i = 0; i < count; ++i) {
            auto& cmd = commands[i];
            cmd.depth = gfxm::dot(cam_forward, cmd.renderable->getSortHint() - cam_pos);
        }
        std::sort(commands, commands + count, [](const gpuRenderCmd& a, const gpuRenderCmd& b)->bool {
            if(a.layer == b.layer) {
                if (fabsf(a.depth) - fabsf(b.depth) < 1e-6f) {
                    return a.sort_bias < b.sort_bias;
                }
                return a.depth < b.depth;
            }
            return a.layer < b.layer;
        });
        break;
    }
    case GPU_SORT_MODE::BACK_TO_FRONT: {
        const gfxm::mat4 cam_transform = gfxm::inverse(params.view);
        const gfxm::vec3 cam_forward = -cam_transform[2];
        const gfxm::vec3 cam_pos = cam_transform[3];
        for (int i = 0; i < count; ++i) {
            auto& cmd = commands[i];
            cmd.depth = gfxm::dot(cam_forward, cmd.renderable->getSortHint() - cam_pos);
        }
        std::sort(commands, commands + count, [](const gpuRenderCmd& a, const gpuRenderCmd& b)->bool {
            if(a.layer == b.layer) {
                if (fabsf(a.depth) - fabsf(b.depth) < 1e-6f) {
                    return a.sort_bias < b.sort_bias;
                }
                return a.depth > b.depth;
            }
            return a.layer < b.layer;
        });
        break;
    }
    default:
        assert(false);
    }
}