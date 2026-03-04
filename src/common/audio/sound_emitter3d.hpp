#pragma once

#include "audio/audio.hpp"


class Soundscape;
class SoundEmitter3d {
    friend Soundscape;

    Handle<AudioChannel> chan;
    ResourceRef<AudioClip> clip;
    float attenuation_radius = 1.0f;
    float gain = .3f;
    bool looping = true;
    bool playing = false;
    gfxm::vec3 position;
public:
    void setClip(const ResourceRef<AudioClip>& clip);
    void stop();
    void play();
    void setGain(float);
    void setAttenuationRadius(float);
    void setLooping(bool);
    void setPosition(const gfxm::vec3& pos);
};

