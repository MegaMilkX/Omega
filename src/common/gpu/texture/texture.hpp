#pragma once

#include "platform/gl/glextutil.h"

#include "resource_manager/resource.hpp"
#include "resource_manager/resource_ref.hpp"
#include "resource_manager/resource_root.hpp"
#include "gpu/texture/texture_resource_backend.hpp"


enum GPU_TEXTURE_FILTER {
    GPU_TEXTURE_FILTER_NEAREST,
    GPU_TEXTURE_FILTER_LINEAR,
    GPU_TEXTURE_FILTER_MIPMAP_LINEAR
};
enum GPU_TEXTURE_WRAP {
    GPU_TEXTURE_WRAP_CLAMP,
    GPU_TEXTURE_WRAP_REPEAT,
    GPU_TEXTURE_WRAP_CLAMP_BORDER
};


class gpuTexture;
RESOURCE_BACKEND(gpuTexture, Texture2dResourceBackend);


class gpuTexture : public Resource, public PolymorphicResourceRoot<gpuTexture> {
protected:
    GLuint id;
public:
    virtual ~gpuTexture() {}
    GLuint getId() const { return id; }

    void _setDbgName(const std::string& name);
};

