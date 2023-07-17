#pragma once

#include <memory>
#include "resource/resource.hpp"
#include "audio_mixer.hpp"
#include "serialization/virtual_ibuf.hpp"

class AudioClip {
    std::unique_ptr<AudioBuffer> buf;
public:
    AudioBuffer* getBuffer() { return buf.get(); }

    virtual bool deserialize(const unsigned char* data, size_t sz) {
        constexpr int targetSampleRate = 44100;

        int channels = 2;
        int sampleRate = targetSampleRate;
        short* decoded;
        int len;
        len = stb_vorbis_decode_memory(data, sz, &channels, &sampleRate, &decoded);

        // TODO: ?
        if (sampleRate > targetSampleRate) {
            LOG_WARN("Audio clip downsampling on load from " << sampleRate << " to " << targetSampleRate);
            //assert(channels == 1);
            // TODO: Fix for stereo
            float resampleRate = float(targetSampleRate) / (float)sampleRate;
            std::vector<short> resampled;
            int resampled_len = (len * resampleRate + 1) * channels;
            resampled.resize(resampled_len);
            for (int i = 0; i < resampled_len; ++i) {
                float jf = i / resampleRate;
                int j0 = floor(jf);
                int j1 = ceil(jf);
                float diff = jf - (float)j0;
                int sample = gfxm::lerp(decoded[j0], +decoded[j1], diff);
                resampled[i] = (short)sample;
            }

            buf.reset(new AudioBuffer(
                resampled.data(), resampled_len * sizeof(short), targetSampleRate, channels
            ));
        } else if(sampleRate < 44100) {
            LOG_WARN("Audio clip upsampling on load from " << sampleRate << " to " << targetSampleRate);
            //assert(channels == 1);
            // TODO: Fix for stereo
            float resampleRate = float(targetSampleRate) / float(sampleRate);
            std::vector<short> resampled;
            int resampled_len = (len * resampleRate) * (float)channels;
            resampled.resize(resampled_len);
            for (int i = 0; i < resampled_len; ++i) {
                float jf = i / resampleRate;
                int j0 = floor(jf);
                int j1 = ceil(jf);
                float diff = jf - (float)j0;
                int sample = decoded[j0];// gfxm::lerp(decoded[j0], decoded[j1], diff);
                resampled[i] = (short)sample;
            }

            buf.reset(new AudioBuffer(
                resampled.data(), resampled_len * sizeof(short), targetSampleRate, channels
            ));
        } else {
            buf.reset(new AudioBuffer(
                decoded, len * sizeof(short) * channels, sampleRate, channels
            ));
        }

        free(decoded);
        return true;
    }
};
inline RHSHARED<AudioClip> getAudioClip(const char* path) {
    return resGet<AudioClip>(path);
}