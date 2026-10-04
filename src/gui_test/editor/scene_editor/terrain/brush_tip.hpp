#pragma once

#include <stdint.h>
#include "math/gfxm.hpp"
#include "image/image.hpp"


class BrushTip {
public:
    virtual ~BrushTip() {}
    virtual void rasterize(uint8_t* out, int width, int height, const gfxm::vec2& center_fract, float angle) = 0;
};


class RadialBrushTip : public BrushTip {
public:
    void rasterize(uint8_t* out, int width, int height, const gfxm::vec2& center_fract, float angle) override {
        for (int y = 0; y < height; ++y) {
            float fy = float(y - center_fract.y) / (height - 1) * 2.f - 1.f;
            for (int x = 0; x < width; ++x) {
                float fx = float(x - center_fract.x) / (width - 1) * 2.f - 1.f;
                float dist2 = fx * fx + fy * fy;
                float dist = gfxm::sqrt(dist2);
                float f = gfxm::_max(.0f, 1.f - dist);
                out[x + y * width] = uint8_t(f * 255.f);
            }
        }
    }
};


class ImageBrushTip : public BrushTip {
    ktImage img;
public:
    ImageBrushTip() {
        loadImage(&img, "textures/decals/fire_circle.png", false);
    }
    void rasterize(uint8_t* out, int width, int height, const gfxm::vec2& center_fract, float angle) override {
        for (int y = 0; y < height; ++y) {
            float fy = float(y - center_fract.y) / (height - 1);
            for (int x = 0; x < width; ++x) {
                float fx = float(x - center_fract.x) / (width - 1);
                auto sample = img.samplef(fx, fy);
                float gray = (sample.x + sample.y + sample.z) / 3.f;
                out[x + y * width] = uint8_t(gray * 255.f);
            }
        }
    }
};

