#pragma once

#include <stdint.h>
#include <vector>
#include "math/gfxm.hpp"


struct TerrainBrushContext {
    float* region;
    uint8_t* mask;
    int width;
    int height;
    float radius;
    float strength;
};

class TerrainBrush {
public:
    virtual ~TerrainBrush() {}
    virtual void apply(TerrainBrushContext&) = 0;
};

class DrawTerrainBrush : public TerrainBrush {
public:
    void apply(TerrainBrushContext& ctx) override {        
        for (int i = 0; i < ctx.width * ctx.height; ++i) {
            ctx.region[i] += ctx.strength * (ctx.mask[i] / 255.f);
        }
    }
};

class SmoothTerrainBrush : public TerrainBrush {
    std::vector<float> src;
public:
    void apply(TerrainBrushContext& ctx) override {
        const int w = ctx.width;
        const int h = ctx.height;
        src.assign(ctx.region, ctx.region + w * h); //c++20
        
        bool invert = ctx.strength < .0f;
        const float k = gfxm::clamp(fabsf(ctx.strength), .0f, 1.f);

        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                const int i = x + y * w;
                const uint8_t m = ctx.mask[i];
                if(!m) continue;

                float sum = .0f;
                for (int dy = -1; dy <= 1; ++dy) {
                    const int yy = gfxm::iclamp(y + dy, 0, h - 1);
                    for (int dx = -1; dx <= 1; ++dx) {
                        const int xx = gfxm::iclamp(x + dx, 0, w - 1);
                        sum += src[xx + yy * w];
                    }
                }
                const float avg = sum * (1.f / 9.f);
                if(!invert) {
                    ctx.region[i] += (avg - src[i]) * (m / 255.f) * k;
                } else {
                    ctx.region[i] -= (avg - src[i]) * (m / 255.f) * k;
                }
            }
        }
    }
};

class FlattenTerrainBrush : public TerrainBrush {
public:
    void apply(TerrainBrushContext& ctx) override {
        const int w = ctx.width;
        const int h = ctx.height;
        const float k = gfxm::clamp(ctx.strength, .0f, 1.f);

        // Weighted centroid
        float W = .0f;
        float sx = .0f;
        float sz = .0f;
        float sh = .0f;
        for (int z = 0; z < h; ++z) {
            for (int x = 0; x < w; ++x) {
                const int i = x + z * w;
                const float wt = ctx.mask[i] / 255.f;
                W += wt;
                sx += wt * x;
                sz += wt * z;
                sh += wt * ctx.region[i];
            }
        }
        if (W < 1e-4f) {
            return;
        }
        const float mx = sx / W;
        const float mz = sz / W;
        const float mh = sh / W;

        //
        float Sxx = .0f;
        float Szz = .0f;
        float Sxz = .0f;
        float Sxh = .0f;
        float Szh = .0f;
        for (int z = 0; z < h; ++z) {
            for (int x = 0; x < w; ++x) {
                const int i = x + z * w;
                const float wt = ctx.mask[i] / 255.f;
                const float dx = x - mx;
                const float dz = z - mz;
                const float dh = ctx.region[i] - mh;
                Sxx += wt * dx * dx;
                Szz += wt * dz * dz;
                Sxz += wt * dx * dz;
                Sxh += wt * dx * dh;
                Szh += wt * dz * dh;
            }
        }

        //
        float a = .0f;
        float b = .0f;
        const float det = Sxx * Szz - Sxz * Sxz;
        if (det > 1e-6f * Sxx * Szz) {
            a = (Sxh * Szz - Szh * Sxz) / det;
            b = (Szh * Sxx - Sxh * Sxz) / det;
        }

        // Pull toward the plane
        for (int z = 0; z < h; ++z) {
            for (int x = 0; x < w; ++x) {
                const int i = x + z * w;
                const float plane = mh + a * (x - mx) + b * (z - mz);
                ctx.region[i] += (plane - ctx.region[i]) * (ctx.mask[i] / 255.f) * k;
            }
        }
    }
};

