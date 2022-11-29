#ifndef GLX_TEXTURE_2D_HPP
#define GLX_TEXTURE_2D_HPP

#include <assert.h>
#include "platform/gl/glextutil.h"
#include "image/image.hpp"
#include "log/log.hpp"

inline void glxBindTexture2d(int layer, GLuint texture) {
    glActiveTexture(GL_TEXTURE0 + layer);
    glBindTexture(GL_TEXTURE_2D, texture);
}

enum GPU_TEXTURE_FILTER {
    GPU_TEXTURE_FILTER_NEAREST,
    GPU_TEXTURE_FILTER_LINEAR
};

class gpuTexture2d {
    GLuint id;
    GLint internalFormat;
    int width = 0;
    int height = 0;
    int bpp;
    GLenum selectFormat(GLint internalFormat, int channels) {
        GLenum format = 0;
        if (internalFormat == GL_DEPTH_COMPONENT) {
            format = GL_DEPTH_COMPONENT;
        } else {
            if (channels == 1) format = GL_RED;
            else if (channels == 2) format = GL_RG;
            else if (channels == 3) format = GL_RGB;
            else if (channels == 4) format = GL_RGBA;
        }
        return format;
    }
public:
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
        LOG_WARN("Deleting texture " << id);
        glDeleteTextures(1, &id);
    }
    void changeFormat(GLint internalFormat, uint32_t width, uint32_t height, int channels) {
        assert(width > 0 && height > 0);

        this->internalFormat = internalFormat;
        this->bpp = channels;

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
        this->width = width;
        this->height = height;
    }
    void resize(uint32_t width, uint32_t height) {
        assert(width > 0 && height > 0);

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
    void setData(const void* data, int width, int height, int channels, IMAGE_CHANNEL_FORMAT fmt = IMAGE_CHANNEL_UNSIGNED_BYTE) {
        assert(width > 0 && height > 0);
        assert(channels > 0);
        assert(channels <= 4);
        this->width = width;
        this->height = height;
        this->bpp = channels;
        GLenum format = selectFormat(internalFormat, channels);

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

    void setFilter(GPU_TEXTURE_FILTER filter) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, id);

        GLint f = GL_NEAREST;
        switch (filter) {
        case GPU_TEXTURE_FILTER_NEAREST: f = GL_NEAREST; break;
        case GPU_TEXTURE_FILTER_LINEAR: f = GL_LINEAR; break;
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, f);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, f);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

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
