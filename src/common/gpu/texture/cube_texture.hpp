#pragma once

#include <assert.h>
#include "platform/gl/glextutil.h"
#include "image/image.hpp"
#include "log/log.hpp"
#include "gpu/texture/texture2d.hpp"
#include "reflection/reflection.hpp"
#include "resource_manager/loadable.hpp"
#include "resource_manager/resource_ref.hpp"


class gpuCubeTexture
: public Resource
, public ILoadable {
    GLuint id = 0;
public:
    TYPE_ENABLE();

    gpuCubeTexture();
    ~gpuCubeTexture();

    GLuint getId() const { return id; }
    
    void reserve(int side, GLint internal_format, GLenum format, GLenum type);
    void setData(const ktImage* image);
    void build(
        const ktImage* posx,
        const ktImage* negx,
        const ktImage* posy,
        const ktImage* negy,
        const ktImage* posz,
        const ktImage* negz
    );

    DEFINE_EXTENSIONS(e_hdr);
    bool load(byte_reader& in) override;
};

