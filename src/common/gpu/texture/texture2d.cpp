#include "texture2d.hpp"



gpuTexture2d::FormatInfo gpuTexture2d::selectFormat2(GLint internalFormat, int channels, bool bgr) const {
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


gpuTexture2d::gpuTexture2d(GLint internalFormat, uint32_t width, uint32_t height, int channels)
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
gpuTexture2d::~gpuTexture2d() {
    //LOG_WARN("Deleting texture " << id);
    glDeleteTextures(1, &id);
}

void gpuTexture2d::changeFormat(GLint internalFormat, uint32_t width, uint32_t height, int channels, GLenum type) {
    //assert(width > 0 && height > 0);

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

void gpuTexture2d::resize(uint32_t width, uint32_t height) {
    //assert(width > 0 && height > 0);

    auto fmt = selectFormat2(internalFormat, bpp);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, id);
    GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, fmt.format, GL_UNSIGNED_BYTE, 0));

    glBindTexture(GL_TEXTURE_2D, 0);

    this->width = width;
    this->height = height;
}

void gpuTexture2d::setData(const ktImage* image) {
    setData(image->getData(), image->getWidth(), image->getHeight(), image->getChannelCount(), image->getChannelFormat());
}

void gpuTexture2d::setDataDXT1RGB(const void* data, int mip_level, int width, int height, int byte_count) {
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

void gpuTexture2d::setDataDXT1RGBA(const void* data, int mip_level, int width, int height, int byte_count) {
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

void gpuTexture2d::setDataDXT5(const void* data, int mip_level, int width, int height, int byte_count) {
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

void gpuTexture2d::setData(const void* data, int width, int height, int channels, IMAGE_CHANNEL_FORMAT fmt, bool bgr) {
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

void gpuTexture2d::setData(const void* data, int mip, int width, int height, int channels, IMAGE_CHANNEL_FORMAT fmt, bool bgr) {
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

void gpuTexture2d::setFilter(GPU_TEXTURE_FILTER filter) {
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

void gpuTexture2d::setWrapMode(GPU_TEXTURE_WRAP wrap) {
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

void gpuTexture2d::setBorderColor(const gfxm::vec4& color) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, id);

    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, (float*)&color);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void gpuTexture2d::generateMipmaps() {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void gpuTexture2d::getData(ktImage* image) const {        
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

bool gpuTexture2d::load(byte_reader& in) {
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

