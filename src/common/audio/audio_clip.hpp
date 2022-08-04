#pragma once

#include <memory>
#include "audio_mixer.hpp"
#include "serialization/virtual_ibuf.hpp"

class AudioClip {
    std::unique_ptr<AudioBuffer> buf;
public:
    AudioBuffer* getBuffer() { return buf.get(); }

    virtual bool deserialize(const unsigned char* data, size_t sz) {
        int channels = 2;
        int sampleRate = 44100;
        short* decoded;
        int len;
        len = stb_vorbis_decode_memory(data, sz, &channels, &sampleRate, &decoded);

        buf.reset(new AudioBuffer(
            decoded, len * sizeof(short) * channels, sampleRate, channels
        ));

        free(decoded);
        return true;
    }
};