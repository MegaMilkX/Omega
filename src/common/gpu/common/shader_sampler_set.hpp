#pragma once

#include <stdint.h>
#include <vector>
#include "util/strid.hpp"


enum SHADER_SAMPLER_SOURCE {
    SHADER_SAMPLER_SOURCE_NONE,
    SHADER_SAMPLER_SOURCE_GPU,
    SHADER_SAMPLER_SOURCE_FRAME_IMAGE_STRING_ID,
    SHADER_SAMPLER_SOURCE_FRAME_IMAGE_IDX
};

enum SHADER_SAMPLER_TYPE {
    SHADER_SAMPLER_TEXTURE2D,
    SHADER_SAMPLER_TEXTURE_BUFFER
};


struct ShaderSamplerSet {
    struct Sampler {
        SHADER_SAMPLER_SOURCE source;
        SHADER_SAMPLER_TYPE type;
        int slot;
        union {
            GLuint texture_id;
            string_id frame_image_id;
            int frame_image_idx;
        };

        Sampler() : frame_image_id("") {}
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
            case SHADER_SAMPLER_SOURCE_FRAME_IMAGE_STRING_ID:
                frame_image_id = other.frame_image_id;
                break;
            case SHADER_SAMPLER_SOURCE_FRAME_IMAGE_IDX:
                frame_image_idx = other.frame_image_idx;
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

