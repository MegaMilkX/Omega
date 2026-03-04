#include "audio_clip.hpp"

#include "math/fft.hpp"
/*
struct PIPE_TEST {
    std::vector<std::vector<gfxm::complex>> spectrums;
};

static PIPE_TEST& getPipe() {
    static PIPE_TEST pipe;
    return pipe;
}

static bool initPipe(PIPE_TEST& pipe) {
    short* decoded = 0;
    int len = 0;
    int channels = 2;
    {
        FILE* f = fopen("audio/sfx/pipe.ogg", "rb");
        if (!f) {
            return false;
        }
        fseek(f, 0, SEEK_END);
        long fsize = ftell(f);
        fseek(f, 0, SEEK_SET);
        std::vector<unsigned char> bytes(fsize, 0);
        fread(&bytes[0], fsize, 1, f);
        fclose(f);

        const unsigned char* data = bytes.data();
        size_t sz = bytes.size();
        
        constexpr int targetSampleRate = 44100;
        int sampleRate = targetSampleRate;
        len = stb_vorbis_decode_memory(data, sz, &channels, &sampleRate, &decoded);
    }

    pipe.spectrums.resize(channels);
    for (int i = 0; i < channels; ++i) {
        std::vector<gfxm::complex> samples(len / channels);
        for (int j = 0; j < len / channels; ++j) {
            samples[j] = decoded[j * channels + i] / float(SHORT_MAX);
        }

        pipe.spectrums[i].resize(len / channels);
        for (int j = 0; j < len / channels; ++j) {
            pipe.spectrums[i][j] = samples[j];
        }

        gfxm::fft(pipe.spectrums[i].data(), pipe.spectrums.size());
    }

    return true;
}
*/

AudioClip::AudioClip() {

}

bool AudioClip::load(byte_reader& reader) {
    //assert(reader.hint() == e_ogg);

    auto view = reader.try_slurp();
    if (!view) {
        return false;
    }

    const unsigned char* data = view.data;
    size_t sz = view.size;


    constexpr int targetSampleRate = 44100;

    int channels = 2;
    int sampleRate = targetSampleRate;
    short* decoded;
    int len;
    len = stb_vorbis_decode_memory(data, sz, &channels, &sampleRate, &decoded);
    if (len == 0) {
        return false;
    }

    buf.reset(new AudioBuffer(
        decoded, len * sizeof(short) * channels, sampleRate, channels
    ));

    free(decoded);
    return true;
}

