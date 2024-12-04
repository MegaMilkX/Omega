#ifndef AUDIO_MIXER_HPP
#define AUDIO_MIXER_HPP

#include <algorithm>
#include <thread>
#include <mutex>
#include <unordered_set>
#include <unordered_map>

#include "handle/handle.hpp"

#include <stdint.h>
#include <xaudio2.h>
#pragma comment(lib, "xaudio2.lib")

#include "math/gfxm.hpp"
#include "log/log.hpp"

#define STB_VORBIS_HEADER_ONLY
extern "C" {
#include "stb_vorbis.c"
}
#undef STB_VORBIS_HEADER_ONLY

#define AUDIO_BUFFER_SZ 256

#include "audio_buffer.hpp"

#include "util/timer.hpp"

struct AUDIO_STATS {
    std::atomic<float> buffer_update_time = .0f;
};

static const int SHORT_MAX = std::numeric_limits<short>().max();

struct AudioChannel {
    size_t cursor = 0;
    AudioBuffer* buf = 0;
    float volume = 1.0f;
    float panning = 0.0f;
    float attenuation_radius = 10.0f;
    gfxm::vec3 pos;
    bool looping = false;
    bool is_orphan = false;

    void setPosition(const gfxm::vec3& p) {
        pos = p;
    }
};

struct AudioVoiceData {
    float* front;
    float* back;

    float buffer_f[AUDIO_BUFFER_SZ];
    float buffer[AUDIO_BUFFER_SZ];
    float buffer_back[AUDIO_BUFFER_SZ];

    float buffer_float[AUDIO_BUFFER_SZ];
    float buffer_back_float[AUDIO_BUFFER_SZ];

    IXAudio2SourceVoice* pSourceVoice;

    std::unordered_set<Handle<AudioChannel>> emitters;
    std::unordered_set<Handle<AudioChannel>> emitters3d;

    std::mutex sync;
};

class AudioMixer : public IXAudio2VoiceCallback {
    int sampleRate;
    int bitPerSample;
    int nChannels;

    // key - sample rate
    std::unordered_map<int, std::unique_ptr<AudioVoiceData>> voices;
    /*
    float* front;
    float* back;

    float buffer_f[AUDIO_BUFFER_SZ];
    float buffer[AUDIO_BUFFER_SZ];
    float buffer_back[AUDIO_BUFFER_SZ];
    */
    IXAudio2* pXAudio2;
    IXAudio2MasteringVoice* pMasteringVoice = 0;
    //IXAudio2SourceVoice* pSourceVoice;

    //std::unordered_set<Handle<AudioChannel>> emitters;
    //std::unordered_set<Handle<AudioChannel>> emitters3d;

    //std::atomic<gfxm::mat4> lis_transform;
    gfxm::mat4 lis_trans_cache;
    std::mutex sync;
    gfxm::mat4 listener_transform_buffers[2];
    gfxm::mat4* listener_transform_front = &listener_transform_buffers[0];
    gfxm::mat4* listener_transform_back = &listener_transform_buffers[1];
    
    bool createSourceVoice(AudioVoiceData* voice, int sampleRate) {
        const int blockAlign = (bitPerSample * nChannels) / 8;

        WAVEFORMATEX wfx = {
            WAVE_FORMAT_IEEE_FLOAT,
            (WORD)nChannels,
            (DWORD)sampleRate,
            (DWORD)(sampleRate * blockAlign),
            (WORD)blockAlign,
            (WORD)bitPerSample,
            0
        };
        HRESULT hr;
        if(FAILED(hr = pXAudio2->CreateSourceVoice(&voice->pSourceVoice, &wfx, 0, 1.0f, this)))
        {
            LOG_ERR("Failed to create source voice: " << hr);
            return false;
        }

        voice->front = &voice->buffer[0];
        voice->back = &voice->buffer_back[0];
        memset(voice->buffer, 0, sizeof(voice->buffer));
    
        
        XAUDIO2_BUFFER buf = { 0 };
        buf.AudioBytes = sizeof(voice->buffer);
        buf.pAudioData = (BYTE*)voice->front;
        buf.LoopCount = 0;
        buf.pContext = voice;

        hr = voice->pSourceVoice->SubmitSourceBuffer(&buf);
        voice->pSourceVoice->Start(0, 0);
    }
    AudioVoiceData* createVoiceForSampleRate(int sampleRate) {
        AudioVoiceData* voiceData = new AudioVoiceData;
        createSourceVoice(voiceData, sampleRate);
        voices.insert(std::make_pair(sampleRate, std::unique_ptr<AudioVoiceData>(voiceData)));
        LOG("Created source voice for sample rate of " << sampleRate);
        return voiceData;
    }
    AudioVoiceData* getVoiceDataBySampleRate(int sampleRate) {
        AudioVoiceData* voiceData = 0;
        auto it = voices.find(sampleRate);
        if (it == voices.end()) {
            voiceData = createVoiceForSampleRate(sampleRate);
        } else {
            voiceData = it->second.get();
        }
        return voiceData;
    }
public:
    AudioMixer() {}
    void setListenerTransform(const gfxm::mat4& t) {
        std::lock_guard<std::mutex> lock(sync);
        *listener_transform_back = t;
    }

    Handle<AudioChannel> createChannel() {
        return HANDLE_MGR<AudioChannel>::acquire();
    }
    void freeChannel(Handle<AudioChannel> h) {
        stop(h);
        HANDLE_MGR<AudioChannel>::release(h);
    }

    void play(Handle<AudioChannel> ch) {
        auto vd = getVoiceDataBySampleRate(ch->buf->sampleRate());
        std::lock_guard<std::mutex> lock(vd->sync);
        vd->emitters.insert(ch);
    }
    void play3d(Handle<AudioChannel> ch) {
        auto vd = getVoiceDataBySampleRate(ch->buf->sampleRate());
        std::lock_guard<std::mutex> lock(vd->sync);
        vd->emitters3d.insert(ch);
    }
    void stop(Handle<AudioChannel> ch) {
        auto vd = getVoiceDataBySampleRate(ch->buf->sampleRate());
        std::lock_guard<std::mutex> lock(vd->sync);
        vd->emitters.erase(ch);
        vd->emitters3d.erase(ch);
    }
    void resetCursor(Handle<AudioChannel> ch) {
        HANDLE_MGR<AudioChannel>::deref(ch)->cursor = 0;
    }

    void setBuffer(Handle<AudioChannel> ch, AudioBuffer* buf) {
        HANDLE_MGR<AudioChannel>::deref(ch)->buf = buf;
        HANDLE_MGR<AudioChannel>::deref(ch)->cursor = 0;
    }
    void setAttenuationRadius(Handle<AudioChannel> ch, float radius) {
        HANDLE_MGR<AudioChannel>::deref(ch)->attenuation_radius = radius;
    }
    void setGain(Handle<AudioChannel> ch, float gain) {
        HANDLE_MGR<AudioChannel>::deref(ch)->volume = gain;
    }
    void setLooping(Handle<AudioChannel> ch, bool v) {
        HANDLE_MGR<AudioChannel>::deref(ch)->looping = v;
    }
    void setPosition(Handle<AudioChannel> ch, const gfxm::vec3& pos) {
        std::lock_guard<std::mutex> lock(sync);
        HANDLE_MGR<AudioChannel>::deref(ch)->setPosition(pos);
    }

    bool isPlaying(Handle<AudioChannel> ch) {
        auto vd = getVoiceDataBySampleRate(ch->buf->sampleRate());
        std::lock_guard<std::mutex> lock(vd->sync);
        return vd->emitters.count(ch) || vd->emitters3d.count(ch);
    }
    bool isLooping(Handle<AudioChannel> ch) {
        return HANDLE_MGR<AudioChannel>::deref(ch)->looping;
    }

    void playOnce(AudioBuffer* buf, float vol = 1.0f, float pan = .0f) {
        auto vd = getVoiceDataBySampleRate(buf->sampleRate());
        std::lock_guard<std::mutex> lock(vd->sync);

        Handle<AudioChannel> em = HANDLE_MGR<AudioChannel>::acquire();
        HANDLE_MGR<AudioChannel>::deref(em)->buf = buf;
        HANDLE_MGR<AudioChannel>::deref(em)->volume = vol;
        HANDLE_MGR<AudioChannel>::deref(em)->panning = pan;
        HANDLE_MGR<AudioChannel>::deref(em)->is_orphan = true;

        vd->emitters.insert(em);
    }
    void playOnce3d(AudioBuffer* buf, const gfxm::vec3& pos, float vol = 1.0f, float attenuation_radius = 10.0f) {
        auto vd = getVoiceDataBySampleRate(buf->sampleRate());
        std::lock_guard<std::mutex> lock(vd->sync);

        Handle<AudioChannel> em = HANDLE_MGR<AudioChannel>::acquire();
        auto emp = HANDLE_MGR<AudioChannel>::deref(em);
        emp->buf = buf;
        emp->volume = vol;
        emp->setPosition(pos);
        emp->attenuation_radius = attenuation_radius;
        emp->is_orphan = true;

        vd->emitters3d.insert(em);
    }

    bool init(int sampleRate, int bps);
    void cleanup();

    void __stdcall OnStreamEnd() {   }

    //Unused methods are stubs
    void __stdcall OnVoiceProcessingPassEnd() {}
    void __stdcall OnVoiceProcessingPassStart(UINT32 SamplesRequired) {}
    void __stdcall OnBufferEnd(void* pBufferContext);
    void __stdcall OnBufferStart(void * pBufferContext) {}
    void __stdcall OnLoopEnd(void * pBufferContext) {}
    void __stdcall OnVoiceError(void * pBufferContext, HRESULT Error) {
        LOG_ERR("XAudio2 voice error: " << Error);
     }
private:
    size_t mix(
        float* dest,
        size_t dest_len,
        short* src,
        size_t src_len,
        size_t cur,
        float vol,
        float panning,
        bool looping
    );
    size_t mix_mono_(
        float* dst, size_t dst_len,
        short* src, size_t src_len,
        size_t src_cur, int src_sampleRate,
        float gain, bool looping
    );
    size_t mix3d_stereo(
        float* dst, size_t dst_len,
        short* src, size_t src_len,
        size_t src_cur, int src_sampleRate,
        float gain, float atten_radius, bool looping,
        const gfxm::vec3& pos,
        const gfxm::mat4& listener_transform
    );
    size_t mix3d_mono(
        float* dst, size_t dst_len,
        short* src, size_t src_len,
        size_t src_cur, int src_sampleRate,
        float gain, float atten_radius, bool looping,
        const gfxm::vec3& pos,
        const gfxm::mat4& listener_transform
    );
};

#endif
