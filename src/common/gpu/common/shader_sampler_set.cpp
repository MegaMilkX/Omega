#include "shader_sampler_set.hpp"

#include <unordered_map>
#include "gpu/gpu_shader_program.hpp"


static const int MAX_SAMPLERS = 16;

void ShaderSamplerSet::addTexture2d(gpuShaderProgram* prog, const std::string& sampler_name, const ResourceRef<gpuTexture2d>& tex) {
    if(!tex) return;
    addTexture2d(prog, sampler_name, tex->getId());
}
void ShaderSamplerSet::addTexture2d(gpuShaderProgram* prog, const std::string& sampler_name, GLuint tex_id) {
    int slot = prog->getDefaultSamplerSlot(sampler_name.c_str());
    if (slot < 0) {
        return;
    }

    ShaderSamplerSet::Sampler sampler;
    sampler.source = SHADER_SAMPLER_SOURCE_GPU;
    sampler.type = SHADER_SAMPLER_TEXTURE2D;
    sampler.slot = slot;
    sampler.texture_id = tex_id;
    add(sampler);
}
void ShaderSamplerSet::addCubemap(gpuShaderProgram* prog, const std::string& sampler_name, const ResourceRef<gpuCubeMap>& tex) {
    if(!tex) return;
    addCubemap(prog, sampler_name, tex->getId());
}
void ShaderSamplerSet::addCubemap(gpuShaderProgram* prog, const std::string& sampler_name, GLuint tex_id) {
    int slot = prog->getDefaultSamplerSlot(sampler_name.c_str());
    if (slot < 0) {
        return;
    }

    ShaderSamplerSet::Sampler sampler;
    sampler.source = SHADER_SAMPLER_SOURCE_GPU;
    sampler.type = SHADER_SAMPLER_CUBE_MAP;
    sampler.slot = slot;
    sampler.texture_id = tex_id;
    add(sampler);
}

struct SamplerSetIdentityNode {
    std::unordered_map<uint64_t, std::unique_ptr<SamplerSetIdentityNode>> children;
    uint32_t uid;
    bool has_uid = false;

    uint32_t getUid() {
        static uint32_t next = 0;
        if (!has_uid) {
            has_uid = true;
            uid = next++;
        }
        return uid;
    }
};

uint32_t ShaderSamplerSet::resolveIdentity() const {
    static SamplerSetIdentityNode root;
    uint64_t keys[MAX_SAMPLERS] = { 0 };

    int slot_count = 0;
    for (int i = 0; i < samplers.size(); ++i) {
        const Sampler* s = &samplers[i];
        if (s->slot >= MAX_SAMPLERS) {
            assert(false);
            continue;
        }
        if (s->slot < 0) {
            assert(false);
            continue;
        }
        keys[s->slot] = uint64_t(s->key) | (uint64_t(s->type) << 32);
        slot_count = slot_count < (s->slot + 1) ? (s->slot + 1) : slot_count;
    }

    // Walk the tree with sampler keys as edges slot0 -> slot1 -> ... -> slotN -> unique_index
    SamplerSetIdentityNode* cur = &root;
    for (int i = 0; i < slot_count; ++i) {
        auto k = keys[i];
        auto it = cur->children.find(k);
        if (it == cur->children.end()) {
            it = cur->children.insert(
                std::make_pair(k, std::make_unique<SamplerSetIdentityNode>())
            ).first;
        }
        cur = it->second.get();
    }

    return cur->getUid();
}

