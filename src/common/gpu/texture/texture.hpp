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

enum class gpuTextureType {
    Tex1d,
    Tex2d,
    Tex3d,
    Array1d,
    Array2d,
    Cube,
    Buffer
};

class gpuTexture;
RESOURCE_BACKEND(gpuTexture, Texture2dResourceBackend);


class gpuTexture : public Resource, public PolymorphicResourceRoot<gpuTexture> {
protected:
    GLuint id;
    GLint internalFormat;
    int width = 0;
    int height = 0;
    int depth = 1;
public:
    virtual ~gpuTexture() {}
    GLuint getId() const { return id; }

    virtual gpuTextureType getType() const = 0;

    GLint getInternalFormat() const { return internalFormat; }

    int getWidth() const { return width; }
    int getHeight() const { return height; }
    int getDepth() const { return depth; }
    int layerCount() const { return depth; }

    void _setDbgName(const std::string& name);
};

