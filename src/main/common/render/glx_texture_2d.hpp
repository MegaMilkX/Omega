#ifndef GLX_TEXTURE_2D_HPP
#define GLX_TEXTURE_2D_HPP

#include <assert.h>
#include "platform/gl/glextutil.h"
#include "common/image/image.hpp"

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
    int width;
    int height;
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
            glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, selectFormat(internalFormat, channels), GL_UNSIGNED_BYTE, 0);
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // TODO Framebuffers dont work with GL_LINEAR_MIPMAP_LINEAR?
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glBindTexture(GL_TEXTURE_2D, 0);
    }
    ~gpuTexture2d() {
        glDeleteTextures(1, &id);
    }
    void changeFormat(GLint internalFormat, uint32_t width, uint32_t height, int channels) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, id);

        if (width && height) {
            glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, selectFormat(internalFormat, channels), GL_UNSIGNED_BYTE, 0);
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // TODO Framebuffers dont work with GL_LINEAR_MIPMAP_LINEAR?
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glBindTexture(GL_TEXTURE_2D, 0);
    }
    void setData(const ktImage* image) {
        setData(image->getData(), image->getWidth(), image->getHeight(), image->getChannelCount(), image->getChannelFormat());
    }
    void setData(const void* data, int width, int height, int channels, IMAGE_CHANNEL_FORMAT fmt = IMAGE_CHANNEL_UNSIGNED_BYTE) {
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
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, data);
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

inline gpuTexture2d* loadTexture(const char* path) {
    ktImage* img = loadImage(path);
    if(!img) {
        return 0;
    }
    gpuTexture2d* tex = new gpuTexture2d(GL_RGBA);
    tex->setData(img);
    delete img;
    return tex;
}


#endif
