#pragma once

#include "complex.hpp"


namespace gfxm {

inline void fft_impl(gfxm::complex* data, int count, bool inverse = false) {
    for (int i = 1, j = 0; i < count; ++i) {
        int bit = count >> 1;
        for(; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if(i < j) std::swap(data[i], data[j]);
    }

    const float sign = inverse ? -1.f : 1.f;

    for (int len = 2; len <= count; len <<= 1) {
        float exp = sign * -2.f * gfxm::pi / float(len);
        gfxm::complex wlen(cosf(exp), sinf(exp));

        for (int i = 0; i < count; i += len) {
            gfxm::complex w(1.f, .0f);
            
            for (int j = 0; j < len / 2; ++j) {
                int u = i + j;
                int v = u + len / 2;

                gfxm::complex t = w * data[v];
                gfxm::complex uval = data[u];

                data[u] = uval + t;
                data[v] = uval - t;

                w *= wlen;
            }
        }
    }

    if (inverse) {
        for (int i = 0; i < count; ++i) {
            data[i] /= float(count);
        }
    }
}

inline void fft(gfxm::complex* data, int count, bool inverse = false) {
    const int WINDOW_LEN = 4096;
    const int WINDOW_COUNT = count / WINDOW_LEN;
    for (int i = 0; i < WINDOW_COUNT; ++i) {
        fft_impl(&data[i * WINDOW_LEN], WINDOW_LEN, inverse);
    }
}

inline void dft(gfxm::complex* in_samples, gfxm::complex* out_spectrum, int count) {
    // for every sample i, add to spectrum bin j
    for (int i = 0; i < count; ++i) {
        for (int j = 0; j < count; ++j) {
            float t = float(j) / float(count);
            float exp = -2.f * gfxm::pi * t * float(i);
            out_spectrum[j] += in_samples[i] * gfxm::complex(cosf(exp), sinf(exp));
        }
    }
}

inline void idft(gfxm::complex* in_spectrum, gfxm::complex* out_samples, int count) {
    for (int i = 0; i < count; ++i) {
        for (int j = 0; j < count; ++j) {
            float t = float(j) / float(count);
            float exp = 2.f * gfxm::pi * t * float(i);
            out_samples[j] += in_spectrum[i] * gfxm::complex(cosf(exp), sinf(exp));
        }
    }
    for (int i = 0; i < count; ++i) {
        out_samples[i] /= float(count);
    }
}

}

