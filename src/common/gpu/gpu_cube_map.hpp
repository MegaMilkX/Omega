#pragma once

#include <assert.h>
#include "platform/gl/glextutil.h"
#include "image/image.hpp"
#include "log/log.hpp"
#include "gpu/gpu_texture_2d.hpp"


class gpuCubeMap {
    GLuint id;
public:
    gpuCubeMap();
    ~gpuCubeMap();
    GLuint getId() { return id; }
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
};