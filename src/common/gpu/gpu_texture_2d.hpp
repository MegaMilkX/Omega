#ifndef GLX_TEXTURE_2D_HPP
#define GLX_TEXTURE_2D_HPP

#include <assert.h>
#include "platform/gl/glextutil.h"
#include "image/image.hpp"
#include "log/log.hpp"
#include "reflection/reflection.hpp"

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

class gpuTexture2d {
    GLuint id;
    GLint internalFormat;
    int width = 0;
    int height = 0;
    int bpp;

    GLenum selectFormat(GLint internalFormat, int channels, bool bgr = false) {
        GLenum format = 0;
        if (internalFormat == GL_DEPTH_COMPONENT) {
            format = GL_DEPTH_COMPONENT;
        } else if(channels == 1) {
            format = GL_RED;
        } else if(!bgr) {
            if (channels == 2) format = GL_RG;
            else if (channels == 3) format = GL_RGB;
            else if (channels == 4) format = GL_RGBA;
        } else {
            if (channels == 2) assert(false);
            else if (channels == 3) format = GL_BGR;
            else if (channels == 4) format = GL_BGRA;
        }
        return format;
    }
public:
    TYPE_ENABLE();

    gpuTexture2d(GLint internalFormat = GL_RGBA, uint32_t width = 0, uint32_t height = 0, int channels = 3)
    : internalFormat(internalFormat) {
        glGenTextures(1, &id);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, id);

        if (width && height) {
            GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, selectFormat(internalFormat, channels), GL_UNSIGNED_BYTE, 0));
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // TODO Framebuffers dont work with GL_LINEAR_MIPMAP_LINEAR?
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glBindTexture(GL_TEXTURE_2D, 0);
    }
    ~gpuTexture2d() {
        //LOG_WARN("Deleting texture " << id);
        glDeleteTextures(1, &id);
    }

    GLint getInternalFormat() const {
        return internalFormat;
    }

    float getAspectRatio() const {
        if (height == 0) {
            return 1.f;
        }
        return width / (float)height;
    }

    void changeFormat(GLint internalFormat, uint32_t width, uint32_t height, int channels, GLenum type = GL_UNSIGNED_BYTE) {
        assert(width > 0 && height > 0);

        this->internalFormat = internalFormat;
        this->bpp = channels;

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, id);

        if (width && height) {
            GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, selectFormat(internalFormat, channels), type, 0));
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // TODO Framebuffers dont work with GL_LINEAR_MIPMAP_LINEAR?
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glBindTexture(GL_TEXTURE_2D, 0);
        this->width = width;
        this->height = height;
    }
    void resize(uint32_t width, uint32_t height) {
        //assert(width > 0 && height > 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, id);
        GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, selectFormat(internalFormat, bpp), GL_UNSIGNED_BYTE, 0));

        glBindTexture(GL_TEXTURE_2D, 0);
    }
    void setData(const ktImage* image) {
        setData(image->getData(), image->getWidth(), image->getHeight(), image->getChannelCount(), image->getChannelFormat());
    }
    void getData(ktImage* image) {
        std::vector<unsigned char> buf(width * height * bpp, 0);
        glBindTexture(GL_TEXTURE_2D, id);
        glGetTexImage(GL_TEXTURE_2D, 0, selectFormat(internalFormat, bpp), GL_UNSIGNED_BYTE, &buf[0]);
        glBindTexture(GL_TEXTURE_2D, 0);
        image->setData(&buf[0], width, height, bpp, IMAGE_CHANNEL_UNSIGNED_BYTE);
    }
    void setDataDXT1RGB(const void* data, int mip_level, int width, int height, int byte_count) {
        assert(width > 0 && height > 0);
        this->width = width;
        this->height = height;
        this->bpp = 4;

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, id);
        // TODO: only do glPixelStorei when texture doesn't actually align
        // When a RGB image with 3 color channels is loaded to a texture object and 3*width is not divisible by 4, GL_UNPACK_ALIGNMENT has to be set to 1, before specifying the texture image with glTexImage2D:
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        
        GL_CHECK(glCompressedTexImage2D(
            GL_TEXTURE_2D,
            mip_level,
            GL_COMPRESSED_RGB_S3TC_DXT1_EXT,
            width, height,
            0,
            byte_count,
            data
        ));
        //glGenerateMipmap(GL_TEXTURE_2D);

        glBindTexture(GL_TEXTURE_2D, 0);
    }
    void setDataDXT1RGBA(const void* data, int mip_level, int width, int height, int byte_count) {
        assert(width > 0 && height > 0);
        this->width = width;
        this->height = height;
        this->bpp = 4;

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, id);
        // TODO: only do glPixelStorei when texture doesn't actually align
        // When a RGB image with 3 color channels is loaded to a texture object and 3*width is not divisible by 4, GL_UNPACK_ALIGNMENT has to be set to 1, before specifying the texture image with glTexImage2D:
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        
        GL_CHECK(glCompressedTexImage2D(
            GL_TEXTURE_2D,
            mip_level,
            GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,
            width, height,
            0,
            byte_count,
            data
        ));
        //glGenerateMipmap(GL_TEXTURE_2D);

        glBindTexture(GL_TEXTURE_2D, 0);
    }
    void setDataDXT5(const void* data, int mip_level, int width, int height, int byte_count) {
        assert(width > 0 && height > 0);
        this->width = width;
        this->height = height;
        this->bpp = 4;

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, id);
        // TODO: only do glPixelStorei when texture doesn't actually align
        // When a RGB image with 3 color channels is loaded to a texture object and 3*width is not divisible by 4, GL_UNPACK_ALIGNMENT has to be set to 1, before specifying the texture image with glTexImage2D:
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        
        GL_CHECK(glCompressedTexImage2D(
            GL_TEXTURE_2D,
            mip_level,
            GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,
            width, height,
            0,
            byte_count,
            data
        ));
        //glGenerateMipmap(GL_TEXTURE_2D);

        glBindTexture(GL_TEXTURE_2D, 0);
    }
    void setData(const void* data, int width, int height, int channels, IMAGE_CHANNEL_FORMAT fmt = IMAGE_CHANNEL_UNSIGNED_BYTE, bool bgr = false) {
        assert(width > 0 && height > 0);
        assert(channels > 0);
        assert(channels <= 4);
        this->width = width;
        this->height = height;
        this->bpp = channels;
        GLenum format = selectFormat(internalFormat, channels, bgr);

        GLenum type = GL_UNSIGNED_BYTE;
        switch (fmt) {
        case IMAGE_CHANNEL_UNSIGNED_BYTE: type = GL_UNSIGNED_BYTE; break;
        case IMAGE_CHANNEL_FLOAT: type = GL_FLOAT; break;
        };

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, id);

        // TODO: only do glPixelStorei when texture doesn't actually align
        // When a RGB image with 3 color channels is loaded to a texture object and 3*width is not divisible by 4, GL_UNPACK_ALIGNMENT has to be set to 1, before specifying the texture image with glTexImage2D:
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, data));
        glGenerateMipmap(GL_TEXTURE_2D);

        glBindTexture(GL_TEXTURE_2D, 0);
    }
    void setData(const void* data, int mip, int width, int height, int channels, IMAGE_CHANNEL_FORMAT fmt = IMAGE_CHANNEL_UNSIGNED_BYTE, bool bgr = false) {
        assert(width > 0 && height > 0);
        assert(channels > 0);
        assert(channels <= 4);
        this->width = width;
        this->height = height;
        this->bpp = channels;
        GLenum format = selectFormat(internalFormat, channels, bgr);

        GLenum type = GL_UNSIGNED_BYTE;
        switch (fmt) {
        case IMAGE_CHANNEL_UNSIGNED_BYTE: type = GL_UNSIGNED_BYTE; break;
        case IMAGE_CHANNEL_FLOAT: type = GL_FLOAT; break;
        };

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, id);

        // TODO: only do glPixelStorei when texture doesn't actually align
        // When a RGB image with 3 color channels is loaded to a texture object and 3*width is not divisible by 4, GL_UNPACK_ALIGNMENT has to be set to 1, before specifying the texture image with glTexImage2D:
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        GL_CHECK(glTexImage2D(GL_TEXTURE_2D, mip, internalFormat, width, height, 0, format, type, data));

        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // TODO:
    void setFilter(GPU_TEXTURE_FILTER filter) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, id);

        GLint minfilter = GL_NEAREST;
        GLint magfilter = GL_NEAREST;
        switch (filter) {
        case GPU_TEXTURE_FILTER_NEAREST:
            minfilter = GL_NEAREST;
            magfilter = GL_NEAREST;
            break;
        case GPU_TEXTURE_FILTER_LINEAR:
            minfilter = GL_LINEAR;
            magfilter = GL_LINEAR;
            break;
        case GPU_TEXTURE_FILTER_MIPMAP_LINEAR:
            minfilter = GL_LINEAR_MIPMAP_LINEAR;
            magfilter = GL_LINEAR;
            break;
        default: assert(false); return;
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minfilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magfilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glBindTexture(GL_TEXTURE_2D, 0);
    }
    void setWrapMode(GPU_TEXTURE_WRAP wrap) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, id);

        GLint w = GL_REPEAT;
        switch (wrap) {
        case GPU_TEXTURE_WRAP_CLAMP: w = GL_CLAMP_TO_EDGE; break;
        case GPU_TEXTURE_WRAP_REPEAT: w = GL_REPEAT; break;
        case GPU_TEXTURE_WRAP_CLAMP_BORDER: w = GL_CLAMP_TO_BORDER; break;
        default: assert(false); return;
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, w);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, w);

        glBindTexture(GL_TEXTURE_2D, 0);
    }
    void setBorderColor(const gfxm::vec4& color) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, id);

        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, (float*)&color);

        glBindTexture(GL_TEXTURE_2D, 0);
    }
    void generateMipmaps() {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    GLuint getId() const {
        return id;
    }

    int getWidth() const {
        return width;
    }
    int getHeight() const {
        return height;
    }

    void bind(int layer) {
        glxBindTexture2d(layer, id);
    }
};

#include "gpu_buffer.hpp"
class gpuBufferTexture1d {
    GLuint id;
    int width;
    gpuBuffer buffer;
public:
    gpuBufferTexture1d() {
        glGenTextures(1, &id);
        glActiveTexture(GL_TEXTURE0);
        GL_CHECK(glBindTexture(GL_TEXTURE_BUFFER, id));

        // Empty buffer not accepted
        //GL_CHECK(glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, buffer.getId()));

        glBindTexture(GL_TEXTURE_BUFFER, 0);
    }
    ~gpuBufferTexture1d() {
        glDeleteTextures(1, &id);
    }
    GLuint getId() const { return id; }
    void setData(void* data, size_t byteCount) {
        width = byteCount / (sizeof(float) * 4);
        buffer.setTextureData(data, byteCount);
        glActiveTexture(GL_TEXTURE0);
        GL_CHECK(glBindTexture(GL_TEXTURE_BUFFER, id));
        GL_CHECK(glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, buffer.getId()));
        glBindTexture(GL_TEXTURE_BUFFER, 0);
    }
    void setData(const gfxm::vec4* data, size_t count) {
        setData((void*)data, count * sizeof(*data));
    }
    void setData(const float* data, size_t count) {
        setData((void*)data, count * sizeof(*data));
    }
};

// TODO
class gpuLut4f {
    GLuint id;
    int width;
public:
    gpuLut4f() : width(0) {
        glGenTextures(1, &id);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, id);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glBindTexture(GL_TEXTURE_2D, 0);
    }
    ~gpuLut4f() {
        glDeleteTextures(1, &id);
    }
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
