#ifndef GLX_TEXTURE_2D_HPP
#define GLX_TEXTURE_2D_HPP

#include <assert.h>
#include "platform/gl/glextutil.h"
#include "image/image.hpp"
#include "log/log.hpp"
#include "reflection/reflection.hpp"
#include "resource_manager/loadable.hpp"
#include "resource_manager/resource_ref.hpp"

#include "gpu/texture2d_resource_backend.hpp"


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

class gpuTexture2d;
RESOURCE_BACKEND(gpuTexture2d, Texture2dResourceBackend);

class gpuTexture2d : public ILoadable {
    GLuint id;
    GLint internalFormat;
    int width = 0;
    int height = 0;
    int bpp;

    struct FormatInfo {
        GLenum format;
        int channels;
    };

    FormatInfo selectFormat2(GLint internalFormat, int channels, bool bgr = false) const {
        if (internalFormat == GL_DEPTH_COMPONENT) {
            return { GL_DEPTH_COMPONENT, 1 };
        }
        if (channels == 1) {
            return { GL_RED, 1 };
        }

        if (bgr) {
            switch (channels) {
            case 3: return { GL_BGR, 3 };
            case 4: return { GL_BGRA, 4 };
            default:
                assert(false && "selectFormat: BGR requires 3 or 4 channels");
                return { 0, 0 };
            }
        }

        switch (channels) {
        case 2: return { GL_RG, 2 };
        case 3: return { GL_RGB, 3 };
        case 4: return { GL_RGBA, 4 };
        default:
            assert(false && "selectFormat: unsupported channel count");
            return { 0, 0 };
        }
    }
public:
    TYPE_ENABLE();

    gpuTexture2d(GLint internalFormat = GL_RGBA, uint32_t width = 0, uint32_t height = 0, int channels = 3)
    : internalFormat(internalFormat) {
        glGenTextures(1, &id);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, id);

        if (width && height) {
            auto fmt = selectFormat2(internalFormat, channels);
            GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, fmt.format, GL_UNSIGNED_BYTE, 0));
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
            auto fmt = selectFormat2(internalFormat, channels);
            GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, fmt.format, type, 0));
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

        auto fmt = selectFormat2(internalFormat, bpp);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, id);
        GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, fmt.format, GL_UNSIGNED_BYTE, 0));

        glBindTexture(GL_TEXTURE_2D, 0);
    }
    void setData(const ktImage* image) {
        setData(image->getData(), image->getWidth(), image->getHeight(), image->getChannelCount(), image->getChannelFormat());
    }
    void getData(ktImage* image) const {        
        GLint prev_binding = 0;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &prev_binding);
        glBindTexture(GL_TEXTURE_2D, id);

        // 
        GLint tex_width = 0, tex_height = 0;
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &tex_width);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &tex_height);

        auto fmt = selectFormat2(internalFormat, bpp);

        GLint prev_alignment = 4;
        glGetIntegerv(GL_PACK_ALIGNMENT, &prev_alignment);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);

        const size_t buf_size = static_cast<size_t>(tex_width) * tex_height * fmt.channels;
        std::vector<unsigned char> buf(buf_size, 0);

        glGetTexImage(GL_TEXTURE_2D, 0, fmt.format, GL_UNSIGNED_BYTE, buf.data());

        GLenum err = glGetError();
        assert(err == GL_NO_ERROR && "glGetTexImage failed");

        glPixelStorei(GL_PACK_ALIGNMENT, prev_alignment);
        glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(prev_binding));

        if (err == GL_NO_ERROR) {
            image->setData(buf.data(), tex_width, tex_height, fmt.channels, IMAGE_CHANNEL_UNSIGNED_BYTE);
        }
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
        auto fmtinfo = selectFormat2(internalFormat, channels, bgr);

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
        GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, fmtinfo.format, type, data));
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
        auto fmtinfo = selectFormat2(internalFormat, channels, bgr);

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
        GL_CHECK(glTexImage2D(GL_TEXTURE_2D, mip, internalFormat, width, height, 0, fmtinfo.format, type, data));

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

    DEFINE_EXTENSIONS(e_png, e_jpg, e_jpeg, e_gif, e_bmp, e_dds, e_tiff, e_tga);
    bool load(byte_reader& in) override {
        auto view = in.try_slurp();
        if (!view) {
            return false;
        }

        ktImage img;
        bool ret = loadImage(&img, view.data, view.size);
        if (!ret) {
            assert(false);
            return false;
        }
        setData(&img);
        generateMipmaps();
        return true;
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
