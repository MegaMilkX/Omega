#pragma once

#include <stdint.h>
#include <vector>
#include "platform/gl/glextutil.h"
#include "util/strid.hpp"
#include "gpu/texture/texture2d.hpp"
#include "gpu/texture/cube_texture.hpp"


enum SHADER_SAMPLER_SOURCE {
    SHADER_SAMPLER_SOURCE_NONE,
    SHADER_SAMPLER_SOURCE_GPU,
    SHADER_SAMPLER_SOURCE_CHANNEL_IDX,
};

enum SHADER_SAMPLER_TYPE {
    SHADER_SAMPLER_TEXTURE2D,
    SHADER_SAMPLER_TEXTURE2D_REF,
    SHADER_SAMPLER_CUBE_MAP,
    SHADER_SAMPLER_TEXTURE_BUFFER
};

class gpuShaderProgram;
struct ShaderSamplerSet {
    struct ChannelBufferIdx {
        int16_t idx = 0;
        int16_t buffer_idx = 0;
    };
    struct Sampler {
        SHADER_SAMPLER_SOURCE source;
        SHADER_SAMPLER_TYPE type;
        int slot;
        union {
            GLuint texture_id;
            uint32_t pipe_channel_index;
            //ChannelBufferIdx channel_idx;
            uint32_t key;
        };
        ResourceRef<gpuTexture2d> tex_ref;

        Sampler() {}
        Sampler(const Sampler& other)
            : source(other.source),
            type(other.type),
            slot(other.slot)
        {
            switch (source) {
            case SHADER_SAMPLER_SOURCE_NONE:
                break;
            case SHADER_SAMPLER_SOURCE_GPU:
                texture_id = other.texture_id;
                break;
            case SHADER_SAMPLER_SOURCE_CHANNEL_IDX:
                //channel_idx = other.channel_idx;
                pipe_channel_index = other.pipe_channel_index;
                break;
            default:
                // Unsupported
                assert(false);
            }

            tex_ref = other.tex_ref;
        }
    };

    std::vector<Sampler> samplers;

    size_t count() const {
        return samplers.size();
    }

    void add(const Sampler& sampler) {
        samplers.push_back(sampler);
    }
    void addTexture2dRef(gpuShaderProgram* prog, const std::string& sampler_name, const ResourceRef<gpuTexture2d>& tex);
    void addTexture2d(gpuShaderProgram* prog, const std::string& sampler_name, const ResourceRef<gpuTexture2d>& tex);
    void addTexture2d(gpuShaderProgram* prog, const std::string& sampler_name, GLuint tex_id);
    void addCubemap(gpuShaderProgram* prog, const std::string& sampler_name, const ResourceRef<gpuCubeTexture>& tex);
    void addCubemap(gpuShaderProgram* prog, const std::string& sampler_name, GLuint tex_id);
    const Sampler& get(int i) const {
        return samplers[i];
    }

    void clear() {
        samplers.clear();
    }

    uint32_t resolveIdentity() const;
};

