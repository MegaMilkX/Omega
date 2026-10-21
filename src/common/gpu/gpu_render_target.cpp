#include "gpu/gpu_render_target.hpp"

#include "gpu/gpu.hpp"
#include "gpu/gpu_pipeline.hpp"

static int channelCountFromFormat(GLint fmt) {
    switch (fmt) {
    case GL_RED:
    case GL_DEPTH_COMPONENT:
    case GL_DEPTH_COMPONENT16:
    case GL_DEPTH_COMPONENT24:
    case GL_DEPTH_COMPONENT32F:
        return 1;
    case GL_RG:
        return 2;
    case GL_RGB:
    case GL_SRGB:
    case GL_RGB16F:
    case GL_RGB32F:
        return 3;
    case GL_RGBA:
    case GL_RGBA16F:
    case GL_RGBA32F:
        return 4;
    }
    return 0;
}
static GLenum componentTypeFromFormat(GLint fmt) {
    switch (fmt) {
    case GL_RGB16F:
    case GL_RGB32F:
    case GL_RGBA16F:
    case GL_RGBA32F:
        return GL_FLOAT;
    }
    return GL_UNSIGNED_BYTE;
}
static void initTargetTexture2d(
    gpuTexture2d* tex,
    gpuPipeline::RenderChannel& rtdesc,
    int layer_idx, int width, int height
) {
    const int nchannels = channelCountFromFormat(rtdesc.format);
    if (nchannels == 0) {
        LOG_ERR("Render target format not supported!");
        assert(false);
        return;
    }
    const GLenum component_type = componentTypeFromFormat(rtdesc.format);

    tex->changeFormat(rtdesc.format, width, height, nchannels, component_type);

    tex->setWrapMode(rtdesc.wrap_mode);
    tex->setBorderColor(rtdesc.border_color);

    tex->_setDbgName(std::format("{}-{}", rtdesc.name, layer_idx));
    GL_CHECK(;);
}
static void initTargetArrayTexture2d(
    gpuArrayTexture2d* tex,
    gpuPipeline::RenderChannel& rtdesc,
    int layer_idx, int width, int height
) {
    const int nchannels = channelCountFromFormat(rtdesc.format);
    if (nchannels == 0) {
        LOG_ERR("Render target format not supported!");
        assert(false);
        return;
    }

    tex->init(rtdesc.format, width, height, rtdesc.depth, nchannels);

    tex->setWrapMode(rtdesc.wrap_mode);
    tex->setBorderColor(rtdesc.border_color);

    tex->_setDbgName(std::format("{}-{}", rtdesc.name, layer_idx));
    GL_CHECK(;);
}

gpuRenderTarget::~gpuRenderTarget() {
    assert(pipeline);
    pipeline->notifyRenderTargetDestroyed(this);
}

gpuRenderTarget::TextureLayer* gpuRenderTarget::ensureChannel(const std::string& name, bool double_buffered) {
    auto it = channel_map.find(name);
    if (it == channel_map.end()) {
        int pipe_ch_idx = gpuGetPipeline()->getChannelIndex(name.c_str());
        if (pipe_ch_idx < 0) {
            LOG_ERR("ensureChannel: channel '" << name << "' is not defined in the pipeline");
            assert(false);
            return nullptr;
        }

        int rt_ch_idx = channels.size();
        pipe_channel_to_local[pipe_ch_idx] = rt_ch_idx;

        // meh
        if (pipe_ch_idx == gpuGetPipeline()->_getOutputTargetIdx()) {
            default_output_texture = rt_ch_idx;
        }

        it = channel_map.insert(std::make_pair(name, rt_ch_idx)).first;
        auto& pch = channels.emplace_back();
        pch.reset(new TextureLayer);
        TextureLayer& ch = *pch.get();
        ch.idx = rt_ch_idx;

        auto pipe_ch = gpuGetPipeline()->getChannel(pipe_ch_idx);
        int width = this->width;
        int height = this->height;
        if (pipe_ch->explicit_width) { width = pipe_ch->explicit_width; }
        if (pipe_ch->explicit_height) { height = pipe_ch->explicit_height; }
        ch.explicit_width = pipe_ch->explicit_width;
        ch.explicit_height = pipe_ch->explicit_height;
        
        if (pipe_ch->depth == 1) {
            auto ptr = new gpuTexture2d;
            ch.textures[0].reset(ptr);
            initTargetTexture2d(ptr, *pipe_ch, 0, width, height);
        } else if (pipe_ch->depth > 1) {
            auto ptr = new gpuArrayTexture2d;
            ch.textures[0].reset(ptr);
            initTargetArrayTexture2d(ptr, *pipe_ch, 0, width, height);
        } else {
            LOG_ERR("Invalid target texture depth value: " << pipe_ch->depth);
            assert(false);
            return nullptr;
        }

        LOG_DBG("ensureChannel: '" << name << "'");
    }
    
    TextureLayer* ch = channels[it->second].get();
    if (!ch->double_buffered && double_buffered) {
        int pipe_ch_idx = gpuGetPipeline()->getChannelIndex(name.c_str());
        auto pipe_ch = gpuGetPipeline()->getChannel(pipe_ch_idx);
        int width = this->width;
        int height = this->height;
        if (pipe_ch->explicit_width) { width = pipe_ch->explicit_width; }
        if (pipe_ch->explicit_height) { height = pipe_ch->explicit_height; }
        if (pipe_ch->depth == 1) {
            auto ptr = new gpuTexture2d;
            ch->textures[1].reset(ptr);
            initTargetTexture2d(ptr, *pipe_ch, 0, width, height);
        } else if (pipe_ch->depth > 1) {
            auto ptr = new gpuArrayTexture2d;
            ch->textures[1].reset(ptr);
            initTargetArrayTexture2d(ptr, *pipe_ch, 0, width, height);
        }
        ch->double_buffered = true;
        LOG_DBG("ensureChannel: '" << name << "': double");
    }

    return ch;
}
gpuRenderTarget::TextureLayer* gpuRenderTarget::getChannel(const std::string& name) {
    auto it = channel_map.find(name);
    if (it == channel_map.end()) {
        return nullptr;
    }
    return channels[it->second].get();
}
gpuRenderTarget::TextureLayer* gpuRenderTarget::getChannel(int i) {
    return channels[i].get();
}
gpuRenderTarget::TextureLayer* gpuRenderTarget::getChannelByPipeId(int i) {
    assert(i >= 0 && i < int(pipe_channel_to_local.size()));
    int lcl_idx = pipe_channel_to_local[i];
    if (lcl_idx < 0) {
        return nullptr;
    }
    return channels[lcl_idx].get();
}
const gpuRenderTarget::TextureLayer* gpuRenderTarget::getChannelByPipeId(int i) const {
    assert(i >= 0 && i < int(pipe_channel_to_local.size()));
    int lcl_idx = pipe_channel_to_local[i];
    if (lcl_idx < 0) {
        return nullptr;
    }
    return channels[lcl_idx].get();
}

void gpuRenderTarget::updateDirty() {
    assert(pipeline);
    pipeline->updateDirty();
    // TODO: Handle resize here
}

void gpuRenderTarget::setDefaultOutput(const char* name, RT_OUTPUT output_mode) {
    auto ch = getChannel(name);
    if (!ch) {
        LOG_WARN("'" << name << "': not found");
        return;
    }
    LOG_DBG("'" << name << "': " << ch->idx);
    default_output_texture = ch->idx;
    default_output_mode = output_mode;
}

gpuTexture2d* gpuRenderTarget::getTexture(const char* name, int buffer_idx) {
    int idx = gpuGetPipeline()->getChannelIndex(name);
    auto ch = getChannelByPipeId(idx);
    if(!ch) return nullptr;
    if (buffer_idx == RT_BUFFER_LAST_WRITTEN) {
        return dynamic_cast<gpuTexture2d*>(ch->textures[ch->lwt].get());
    }
    return dynamic_cast<gpuTexture2d*>(ch->textures[buffer_idx].get());
}

void gpuRenderTarget::setSize(int width, int height) {
    if (this->width == width && this->height == height) {
        return;
    }
    this->width = width;
    this->height = height;
    for (int i = 0; i < channels.size(); ++i) {
        auto& layer = channels[i];
        int w = width;
        int h = height;
        if (layer->explicit_width) {
            w = layer->explicit_width;
        }
        if (layer->explicit_height) {
            h = layer->explicit_height;
        }

        // TODO: Anything other than a 2d texture will not be resized
        gpuTexture2d* a = dynamic_cast<gpuTexture2d*>(layer->textures[0].get());
        gpuTexture2d* b = dynamic_cast<gpuTexture2d*>(layer->textures[1].get());
        if(a) a->resize(w, h);
        if(b) b->resize(w, h);
    }
}

void gpuRenderTarget::setDebugRenderGeometryRange(int begin, int end) {
    dbg_geomRangeBegin = begin;
    dbg_geomRangeEnd = end;
}