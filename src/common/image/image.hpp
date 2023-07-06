#ifndef KT_IMAGE_HPP
#define KT_IMAGE_HPP

#include <vector>
#include <assert.h>

enum IMAGE_CHANNEL_FORMAT {
    IMAGE_CHANNEL_UNSIGNED_BYTE,
    IMAGE_CHANNEL_FLOAT
};
constexpr int IMAGE_CHANNEL_FORMAT_SIZES[] = {
    1,
    4
};

class ktImage {
    IMAGE_CHANNEL_FORMAT channel_fmt;
    int width       = 0;
    int height      = 0;
    int channels    = 0;
    std::vector<unsigned char> buffer;
public:
    ktImage() {}
    ~ktImage() {}
    void reserve(int width, int height, int channels, IMAGE_CHANNEL_FORMAT fmt = IMAGE_CHANNEL_UNSIGNED_BYTE) {
        channel_fmt = fmt;
        this->width = width;
        this->height = height;
        this->channels = channels;
        buffer.resize(width * height * channels * IMAGE_CHANNEL_FORMAT_SIZES[fmt]);
    }
    void setData(const void* data, int width, int height, int channels, IMAGE_CHANNEL_FORMAT fmt = IMAGE_CHANNEL_UNSIGNED_BYTE) {
        reserve(width, height, channels, fmt);
        memcpy(buffer.data(), data, width * height * channels * IMAGE_CHANNEL_FORMAT_SIZES[fmt]);
    }

    void blit(
        void* srcData,
        unsigned srcW,
        unsigned srcH,
        unsigned srcBpp,
        unsigned xOffset,
        unsigned yOffset
    ) {
        assert(width && height && channels);
        assert(channel_fmt == IMAGE_CHANNEL_UNSIGNED_BYTE); // TODO
        unsigned destW = width;
        unsigned destH = height;
        unsigned destBpp = channels;

        unsigned char* dst = (unsigned char*)buffer.data();
        unsigned char* src = (unsigned char*)srcData;
        for (unsigned y = 0; (y + yOffset) < destH && y < srcH; ++y)
        {
            unsigned index = ((y + yOffset) * destW + xOffset) * destBpp;
            unsigned srcIndex = (y * srcW) * srcBpp;
            for (unsigned x = 0; (x + xOffset) < destW && x < srcW; ++x)
            {                
                for (unsigned b = 0; b < destBpp && b < srcBpp; ++b)
                {
                    dst[index + b] = src[srcIndex + b];
                }
                if (destBpp == 4 && srcBpp < 4)
                    dst[index + 3] = 255;

                index += destBpp;
                srcIndex += srcBpp;
            }
        }
    }

    const void* getData() const {
        return buffer.data();
    }
    int getWidth() const {
        return width;
    }
    int getHeight() const {
        return height;
    }
    int getChannelCount() const {
        return channels;
    }
    IMAGE_CHANNEL_FORMAT getChannelFormat() const {
        return channel_fmt;
    }
    int getSizeBytes() const {
        return width * height * channels * IMAGE_CHANNEL_FORMAT_SIZES[channel_fmt];
    }
};

#include <stb_image.h>
inline bool loadImage(ktImage* img, const void* data, size_t sz, bool flip_y = true) {
    const int CHANNELS = 4;
    int w, h;
    int ch;
    stbi_set_flip_vertically_on_load(flip_y ? 1 : 0);
    stbi_uc* stbi_buf = stbi_load_from_memory((stbi_uc*)data, sz, &w, &h, &ch, CHANNELS);
    //stbi_uc* stbi_buf = stbi_load_from_file(f, &w, &h, &ch, CHANNELS);
    if (!stbi_buf) {
        return false;
    }
    img->setData(stbi_buf, w, h, CHANNELS);
    stbi_image_free(stbi_buf);
    return true;
}
inline bool loadImage(ktImage* img, const char* path, bool flip_y = true) {
    FILE* f = fopen(path, "rb");
    if(!f) {
        return false;
    }
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    std::vector<unsigned char> bytes(fsize, 0);
    fread(&bytes[0], fsize, 1, f);
    fclose(f);

    return loadImage(img, bytes.data(), bytes.size(), flip_y);
}

#include <stb_image_write.h>
inline void writeImagePng(std::vector<unsigned char>& buf, ktImage* img) {
    void(*pfn_stbi_write_func)(void *context, void *data, int size) = [](void *context, void *data, int size) {
        std::vector<unsigned char>* pbuf = (std::vector<unsigned char>*)context;
        pbuf->insert(pbuf->end(), (unsigned char*)data, ((unsigned char*)data) + size);
    };

    stbi_flip_vertically_on_write(true);
    stbi_write_png_to_func(pfn_stbi_write_func, &buf, img->getWidth(), img->getHeight(), img->getChannelCount(), img->getData(), 0);
}

#endif
