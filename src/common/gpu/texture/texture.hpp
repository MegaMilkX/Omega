#pragma once

#include "platform/gl/glextutil.h"

#include "resource_manager/resource.hpp"
#include "resource_manager/resource_ref.hpp"
#include "resource_manager/resource_root.hpp"
#include "gpu/texture/texture_resource_backend.hpp"


class gpuTexture;
RESOURCE_BACKEND(gpuTexture, Texture2dResourceBackend);


class gpuTexture : public Resource, public PolymorphicResourceRoot<gpuTexture> {
    GLuint id;
public:
    virtual ~gpuTexture() {}
    GLuint getId() const { return id; }
};

