#include "audio_mixer.hpp"


bool AudioMixer::init(int sampleRate, int bps) {
    this->sampleRate = sampleRate;
    this->bitPerSample = 32;
    this->nChannels = 2;
    //memset(buffer, 0, sizeof(buffer));
    int blockAlign = (bitPerSample * nChannels) / 8;

    //front = &buffer[0];
    //back = &buffer_back[0];

    gfxm::mat4 lis_transform = gfxm::mat4(1.0f);

    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        // NOTE: It's ok to fail here, means someone else already did it
        //LOG_ERR("Failed to init COM: " << hr);
        //return false;
    }
#if(_WIN32_WINNT < 0x602)
#ifdef _DEBUG
    HMODULE xAudioDll = LoadLibraryExW(L"XAudioD2_7.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
#else
    HMODULE xAudioDll = LoadLibraryExW(L"XAudio2_7.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
#endif
    if (!xAudioDll) {
        LOG_ERR("Failed to find XAudio2.7 dll");
        CoUninitialize();
        return 1;
    }
#endif
    UINT32 flags = 0;
#if (_WIN32_WINNT < 0x0602 /*_WIN32_WINNT_WIN8*/) && defined(_DEBUG)
    flags |= XAUDIO2_DEBUG_ENGINE;
#endif

    hr = XAudio2Create(&pXAudio2, flags);
    if (FAILED(hr)) {
        LOG_ERR("Failed to init XAudio2: " << hr);
        CoUninitialize();
        return false;
    }

    if (FAILED(hr = pXAudio2->CreateMasteringVoice(&pMasteringVoice)))
    {
        LOG_ERR("Failed to create mastering voice: " << hr);
        //pXAudio2.Reset();
        CoUninitialize();
        return false;
    }
    pMasteringVoice->SetVolume(1.00f);

    CoUninitialize();

    return true;
}
void AudioMixer::cleanup() {
    pXAudio2->StopEngine();
    pXAudio2->Release();
}


void __stdcall AudioMixer::OnBufferEnd(void * pBufferContext) {
    AudioVoiceData* pVoiceData = (AudioVoiceData*)pBufferContext;
    extern AUDIO_STATS audio_stats;
        
    timer timer_;
    timer_.start();

    {
        lis_trans_cache = *listener_transform_front;
        std::lock_guard<std::mutex> lock(sync);

        auto ptr = listener_transform_front;
        listener_transform_front = listener_transform_back;
        listener_transform_back = ptr;
    }

    std::lock_guard<std::mutex> lock_voice(pVoiceData->sync);

    memset(pVoiceData->buffer_f, 0, sizeof(pVoiceData->buffer_f));
    memset(pVoiceData->back, 0, sizeof(pVoiceData->buffer));
    size_t buf_len = sizeof(pVoiceData->buffer_f) / sizeof(pVoiceData->buffer_f[0]);
    for(auto it = pVoiceData->emitters.begin(); it != pVoiceData->emitters.end();) {
        Handle<AudioChannel> ei = (*it);
        AudioChannel* em = HANDLE_MGR<AudioChannel>::deref(ei);
        if(!em->buf) {
            if (ei->is_orphan) {
                HANDLE_MGR<AudioChannel>::release(ei);
            }
            it = pVoiceData->emitters.erase(it);
            continue;
        }

        size_t src_cur = em->cursor;
        short* data = em->buf->getPtr();
        size_t src_len = em->buf->sampleCount();
        int sample_rate = em->buf->sampleRate();
        int channel_count = em->buf->channelCount();
        bool looping = em->looping;
            
        size_t advance = 0;
        if (channel_count == 2) {
            advance = mix(
                pVoiceData->buffer_f, buf_len, data, src_len,
                src_cur, em->volume, em->panning,
                looping
            );
        } else if (channel_count == 1) {
            advance = mix_mono_(
                pVoiceData->buffer_f, buf_len,
                data, src_len, src_cur, sample_rate,
                em->volume, looping
            );
        }
            
        em->cursor += advance;
        if(em->cursor >= em->buf->sampleCount()) {
            if(!em->looping) {
                if (ei->is_orphan) {
                    HANDLE_MGR<AudioChannel>::release(ei);
                }
                it = pVoiceData->emitters.erase(it);
                continue;
            }
        } 
        em->cursor = em->cursor % src_len;
        ++it;
    }
    for(auto it = pVoiceData->emitters3d.begin(); it != pVoiceData->emitters3d.end();) {
        Handle<AudioChannel> ei = (*it);
        AudioChannel* em = HANDLE_MGR<AudioChannel>::deref(ei);
        if(!em->buf) {
            if (ei->is_orphan) {
                HANDLE_MGR<AudioChannel>::release(ei);
            }
            it = pVoiceData->emitters3d.erase(it);
            continue;
        }

        gfxm::vec3 p_;
        {
            //std::lock_guard<std::mutex> lock(sync);
            p_ = em->pos;
        }
            
        size_t src_cur = em->cursor;
        short* data = em->buf->getPtr();
        size_t src_len = em->buf->sampleCount();
        int src_sample_rate = em->buf->sampleRate();
        int src_channel_count = em->buf->channelCount();
        bool looping = em->looping;

        size_t advance = 0;
        if (src_channel_count == 2) {
            advance = mix3d_stereo(
                pVoiceData->buffer_f, buf_len,
                data, src_len, src_cur, src_sample_rate,
                em->volume, em->attenuation_radius, looping, p_, lis_trans_cache
            );
        } else if(src_channel_count == 1) {
            advance = mix3d_mono(
                pVoiceData->buffer_f, buf_len,
                data, src_len, src_cur, src_sample_rate,
                em->volume, em->attenuation_radius, looping, p_, lis_trans_cache
            );
        }

        em->cursor += advance;
        if(em->cursor >= em->buf->sampleCount()) {
            if(!em->looping) {
                if (ei->is_orphan) {
                    HANDLE_MGR<AudioChannel>::release(ei);
                }
                it = pVoiceData->emitters3d.erase(it);
                continue;
            }
        }
        em->cursor = em->cursor % em->buf->sampleCount();
        ++it;
    }

    for(int i = 0; i < AUDIO_BUFFER_SZ; ++i) {
        pVoiceData->back[i] = pVoiceData->buffer_f[i];
    }

    float* tmp = pVoiceData->front;
    pVoiceData->front = pVoiceData->back;
    pVoiceData->back = tmp;
        
    XAUDIO2_BUFFER buf = { 0 };
    buf.AudioBytes = sizeof(pVoiceData->buffer);
    buf.pAudioData = (BYTE*)pVoiceData->front;
    buf.LoopCount = 0;
    buf.pContext = pVoiceData;
    pVoiceData->pSourceVoice->SubmitSourceBuffer(&buf);

    audio_stats.buffer_update_time = timer_.stop();
}



size_t AudioMixer::mix(
    float* dest, 
    size_t dest_len, 
    short* src, 
    size_t src_len,
    size_t cur,
    float vol,
    float panning,
    bool looping
) {
    size_t sample_len = src_len < dest_len ? src_len : dest_len;
    size_t overflow = (cur + sample_len) > src_len ? (cur + sample_len) - src_len : 0;

    size_t tgt0sz = sample_len - overflow;
    size_t tgt1sz = overflow;
    short* tgt0 = src + cur;
    short* tgt1 = src;

    float mul = 1.0f / (float)SHORT_MAX;

    for(size_t i = 0; i < tgt0sz; ++i) {
        int lr = (i % 2) * 2 - 1;
        float pan = std::min(fabs(lr + panning), 1.0f);
            
        dest[i] += tgt0[i] * mul * vol * pan;
    }

    if (!looping) {
        return sample_len;
    }

    for(size_t i = 0; i < tgt1sz; ++i) {
        int lr = (i % 2) * 2 - 1;
        float pan = std::min(fabs(lr + panning), 1.0f);
        (dest + tgt0sz)[i] += tgt1[i] * mul * vol * pan;
    }

    return sample_len;
}
size_t AudioMixer::mix_mono_(
    float* dst, size_t dst_len,
    short* src, size_t src_len,
    size_t src_cur, int src_sampleRate,
    float gain, bool looping
) {
    constexpr int dst_channelCount = 2;
    constexpr float flt_convert = 1.0f / (float)SHORT_MAX;
    float sampleRatio = src_sampleRate / (float)this->sampleRate;
    int invSampleRatio = 1;

    for (int di = 0; di < dst_len; di += dst_channelCount) {
        int step = di / dst_channelCount;
        int si = (int)((src_cur + step / invSampleRatio)) % src_len;
        float s = src[si] * flt_convert * gain;
        dst[di    ] += s; // left
        dst[di + 1] += s; // right
    }
    // TODO: Handle non-looping

    return (int)(dst_len / dst_channelCount / invSampleRatio);
}
size_t AudioMixer::mix3d_stereo(
    float* dst, size_t dst_len,
    short* src, size_t src_len,
    size_t src_cur, int src_sampleRate,
    float gain, float atten_radius, bool looping,
    const gfxm::vec3& pos,
    const gfxm::mat4& listener_transform
) {
    constexpr int dst_channelCount = 2;
    constexpr float flt_convert = 1.0f / (float)SHORT_MAX;
    float sampleRatio = src_sampleRate / (float)this->sampleRate;
    int invSampleRatio = 1;

    gfxm::vec3 ears[2] = {
        gfxm::vec3(-0.1075f, .0f, .0f),
        gfxm::vec3(0.1075f, .0f, .0f)
    };
    ears[0] = listener_transform * gfxm::vec4(ears[0], 1.0f);
    ears[1] = listener_transform * gfxm::vec4(ears[1], 1.0f);
    float att = 1.0f / atten_radius;
    float falloff[2] = {
        std::min(1.0f / pow((gfxm::length(ears[0] - pos) * att), 2.0f), 1.0f),
        std::min(1.0f / pow((gfxm::length(ears[1] - pos) * att), 2.0f), 1.0f)
    };

    size_t len = dst_len;
    if (!looping) {
        len = std::min(dst_len, src_len);
    }

    for (int di = 0; di < len; di += dst_channelCount) {
        int step = di;
        int si = (int)((src_cur + step / invSampleRatio));// % src_len;
        if(!looping && si >= src_len) {
            continue;
        }
        si %= src_len;
        float s0 = src[si] * flt_convert * gain;
        float s1 = src[si + 1] * flt_convert * gain;
        dst[di] += (s0 + s1) * falloff[0]; // left
        dst[di + 1] += (s0 + s1) * falloff[1]; // right
    }
    // TODO: Handle non-looping

    return (int)(dst_len / invSampleRatio);
}
size_t AudioMixer::mix3d_mono(
    float* dst, size_t dst_len,
    short* src, size_t src_len,
    size_t src_cur, int src_sampleRate,
    float gain, float atten_radius, bool looping,
    const gfxm::vec3& pos,
    const gfxm::mat4& listener_transform
) {
    constexpr int dst_channelCount = 2;
    constexpr float flt_convert = 1.0f / (float)SHORT_MAX;
    float sampleRatio = src_sampleRate / (float)this->sampleRate;
    int invSampleRatio = 1;

    gfxm::vec3 ears[2] = {
        gfxm::vec3(-0.1075f, .0f, .0f),
        gfxm::vec3(0.1075f, .0f, .0f)
    };
    ears[0] = listener_transform * gfxm::vec4(ears[0], 1.0f);
    ears[1] = listener_transform * gfxm::vec4(ears[1], 1.0f);
    constexpr float EMITTER_RADIUS = 0.5f;
    float att = 1.0f / atten_radius;
    float falloff[2] = {
        std::min(1.0f / pow((gfxm::length(ears[0] - pos) * att), 2.0f), 1.0f),
        std::min(1.0f / pow((gfxm::length(ears[1] - pos) * att), 2.0f), 1.0f)
    };        

    int samples_till_end = src_len - src_cur;
    int len_to_sample = looping ? dst_len : std::min((int)dst_len, samples_till_end);
    for (int di = 0; di < len_to_sample; di += dst_channelCount) {
        int step = di / dst_channelCount;
        int si = (int)((src_cur + step / invSampleRatio)) % src_len;
        float s = src[si] * flt_convert * gain;
        dst[di] += s * falloff[0]; // left
        dst[di + 1] += s * falloff[1]; // right
    }

    return (int)(dst_len / dst_channelCount / invSampleRatio);
}

