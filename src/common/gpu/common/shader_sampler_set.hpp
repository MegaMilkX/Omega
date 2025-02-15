#pragma once

#include <stdint.h>
#include <vector>
#include "util/strid.hpp"


enum SHADER_SAMPLER_SOURCE {
    SHADER_SAMPLER_SOURCE_NONE,
    SHADER_SAMPLER_SOURCE_GPU,
    SHADER_SAMPLER_SOURCE_CHANNEL_STRING_ID,
    SHADER_SAMPLER_SOURCE_CHANNEL_IDX
};

enum SHADER_SAMPLER_TYPE {
    SHADER_SAMPLER_TEXTURE2D,
    SHADER_SAMPLER_CUBE_MAP,
    SHADER_SAMPLER_TEXTURE_BUFFER
};


struct ShaderSamplerSet {
    struct ChannelBufferIdx {
        int idx = 0;
        int buffer_idx = 0;
    };
    struct ChannelBufferStringId {
        string_id id;
        int buffer_idx = 0;
    };
    struct Sampler {
        SHADER_SAMPLER_SOURCE source;
        SHADER_SAMPLER_TYPE type;
        int slot;
        union {
            GLuint texture_id;
            ChannelBufferStringId string_id;
            ChannelBufferIdx channel_idx;
        };

        Sampler() {}
        Sampler(const Sampler& other)
            : source(other.source),
            type(other.type),
            slot(other.slot) {
            switch (source) {
            case SHADER_SAMPLER_SOURCE_NONE:
                break;
            case SHADER_SAMPLER_SOURCE_GPU:
                texture_id = other.texture_id;
                break;
            case SHADER_SAMPLER_SOURCE_CHANNEL_STRING_ID:
                string_id = other.string_id;
                break;
            case SHADER_SAMPLER_SOURCE_CHANNEL_IDX:
                channel_idx = other.channel_idx;
                break;
            default:
                // Unsupported
                assert(false);
            }
        }
    };

    std::vector<Sampler> samplers;

    size_t count() const {
        return samplers.size();
    }

    void add(const Sampler& sampler) {
        samplers.push_back(sampler);
    }
    const Sampler& get(int i) const {
        return samplers[i];
    }

    void clear() {
        samplers.clear();
    }
};

