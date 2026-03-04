#include "image.hpp"


void ktImage::blit(
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

gfxm::vec4 ktImage::samplef(float u, float v) {
    float fu = gfxm::fract(width * u);
    float fv = gfxm::fract(height * v);

    int bpt = channels * IMAGE_CHANNEL_FORMAT_SIZES[channel_fmt];
    void* p0 = nullptr;
    void* p1 = nullptr;
    void* p2 = nullptr;
    void* p3 = nullptr;
    {
        int iu = int(width * u) % width;
        int iv = int(height * v) % height;
        int itexel = iu + iv * width;
        p0 = buffer.data() + itexel * bpt;
    }
    {
        int iu = int(width * u + 1) % width;
        int iv = int(height * v) % height;
        int itexel = iu + iv * width;
        p1 = buffer.data() + itexel * bpt;
    }
    {
        int iu = int(width * u + 1) % width;
        int iv = int(height * v + 1) % height;
        int itexel = iu + iv * width;
        p2 = buffer.data() + itexel * bpt;
    }
    {
        int iu = int(width * u) % width;
        int iv = int(height * v + 1) % height;
        int itexel = iu + iv * width;
        p3 = buffer.data() + itexel * bpt;
    }

    gfxm::vec4 px0;
    gfxm::vec4 px1;
    gfxm::vec4 px2;
    gfxm::vec4 px3;
    if (channel_fmt == IMAGE_CHANNEL_UNSIGNED_BYTE) {
        for (int i = 0; i < channels; ++i) {
            px0[i] = ((uint8_t*)p0)[i] / 255.f;
            px1[i] = ((uint8_t*)p1)[i] / 255.f;
            px2[i] = ((uint8_t*)p2)[i] / 255.f;
            px3[i] = ((uint8_t*)p3)[i] / 255.f;
        }
    } else if (channel_fmt == IMAGE_CHANNEL_FLOAT) {
        for (int i = 0; i < channels; ++i) {
            px0[i] = ((float*)p0)[i];
            px1[i] = ((float*)p1)[i];
            px2[i] = ((float*)p2)[i];
            px3[i] = ((float*)p3)[i];
        }
    } else {
        assert(false);
    }

    gfxm::vec4 px;
    px0 = gfxm::lerp(px0, px1, fu);
    px3 = gfxm::lerp(px3, px2, fu);
    px = gfxm::lerp(px0, px3, fv);

    return px;
}

