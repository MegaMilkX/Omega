#pragma once

#include <stdint.h>

typedef uint64_t ResourceHandle;

enum class ResourceType {
    TEXTURE2D,
    TEXTURE_SAMPLER2D,
    TEXTURE_ATLAS2D,
    SPRITE,
    SHADER_PROGRAM,
    RENDER_MATERIAL
};

class IResourceCache {
public:
    virtual ~IResourceCache() {}

    virtual ResourceHandle alloc() = 0;
    virtual void free(ResourceHandle id) = 0;
    virtual void* deref(ResourceHandle id) = 0;
};

template<typename T>
class ResourcePtr {
public:
    ResourcePtr() {}
    ~ResourcePtr() {}
};

#include "common/render/glx_texture_2d.hpp"
#include <vector>
class ResourceCacheTexture2d : public IResourceCache {
    std::vector<gpuTexture2d*> textures;
public:
    ResourceHandle alloc() override {
        ResourceHandle h = textures.size();
        textures.push_back(new gpuTexture2d);
        return h;
    }
    void free(ResourceHandle id) override {
        delete textures[id];
        textures.erase(textures.begin() + id);
    }
    void* deref(ResourceHandle id) override {
        return (void*)textures[id];
    }
};

bool resourceCacheInit(ResourceType type, IResourceCache* cache);
void resourceCacheCleanup();

void resourceCacheGet(ResourceType type, const char* name);
void resourceCacheSet(ResourceType type, const char* name);


inline void resourceCacheTest() {
    resourceCacheInit(ResourceType::TEXTURE2D, new ResourceCacheTexture2d);
}