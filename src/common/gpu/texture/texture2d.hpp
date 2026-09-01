#ifndef GLX_TEXTURE_2D_HPP
#define GLX_TEXTURE_2D_HPP

#include <assert.h>
#include "platform/gl/glextutil.h"
#include "image/image.hpp"
#include "log/log.hpp"
#include "reflection/reflection.hpp"
#include "resource_manager/loadable.hpp"

#include "gpu/texture/texture.hpp"


inline void glxBindTexture2d(int layer, GLuint texture) {
    glActiveTexture(GL_TEXTURE0 + layer);
    glBindTexture(GL_TEXTURE_2D, texture);
}

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

class gpuTexture2d : public gpuTexture, public ILoadable {
    GLuint id;
    GLint internalFormat;
    int width = 0;
    int height = 0;
    int bpp;

    struct FormatInfo {
        GLenum format;
        int channels;
    };

    FormatInfo selectFormat2(GLint internalFormat, int channels, bool bgr = false) const;
public:
    TYPE_ENABLE();

    gpuTexture2d(GLint internalFormat = GL_RGBA, uint32_t width = 0, uint32_t height = 0, int channels = 3);
    ~gpuTexture2d();

    GLuint getId() const { return id; }

    gfxm::ivec2 getSize() const { return gfxm::ivec2(width, height); }
    int getWidth() const { return width; }
    int getHeight() const { return height; }

    GLint getInternalFormat() const { return internalFormat; }

    float getAspectRatio() const { if (height == 0) return 1.f; return width / (float)height; }

    void changeFormat(GLint internalFormat, uint32_t width, uint32_t height, int channels, GLenum type = GL_UNSIGNED_BYTE);
    void resize(uint32_t width, uint32_t height);

    void setData(const ktImage* image);
    void setDataDXT1RGB(const void* data, int mip_level, int width, int height, int byte_count);
    void setDataDXT1RGBA(const void* data, int mip_level, int width, int height, int byte_count);
    void setDataDXT5(const void* data, int mip_level, int width, int height, int byte_count);
    void setData(const void* data, int width, int height, int channels, IMAGE_CHANNEL_FORMAT fmt = IMAGE_CHANNEL_UNSIGNED_BYTE, bool bgr = false);
    void setData(const void* data, int mip, int width, int height, int channels, IMAGE_CHANNEL_FORMAT fmt = IMAGE_CHANNEL_UNSIGNED_BYTE, bool bgr = false);

    // TODO:
    void setFilter(GPU_TEXTURE_FILTER filter);
    void setWrapMode(GPU_TEXTURE_WRAP wrap);
    void setBorderColor(const gfxm::vec4& color);
    void generateMipmaps();

    void getData(ktImage* image) const;

    void bind(int layer) {
        glxBindTexture2d(layer, id);
    }

    DEFINE_EXTENSIONS(e_png, e_jpg, e_jpeg, e_gif, e_bmp, e_dds, e_tiff, e_tga);
    bool load(byte_reader& in) override;
};


inline gpuTexture2d* loadTexture(const char* path) {
    ktImage img;    
    if(!loadImage(&img, path)) {
        return 0;
    }
    gpuTexture2d* tex = new gpuTexture2d(GL_RGBA);
    tex->setData(&img);
    return tex;
}


#endif
