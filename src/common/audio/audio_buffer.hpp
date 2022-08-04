#ifndef AUDIO_BUFFER_HPP
#define AUDIO_BUFFER_HPP

#include <vector>

struct AudioBuffer {
    AudioBuffer(const void* data, size_t sz, int sampleRate, int nChannels) {
        this->data = std::vector<short>((short*)data, (short*)((char*)data + sz));
        sample_rate = sampleRate;
        n_channels = nChannels;
    }
    short* getPtr() {
        return data.data();
    }
    size_t sampleCount() {
        return data.size();
    }
    int channelCount() {
        return n_channels;
    }
    int sampleRate() {
        return sample_rate;
    }
private:
    std::vector<short> data;
    int sample_rate;
    int n_channels = 2;
};

#endif
