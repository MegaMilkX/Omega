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
    ResourceRef<AudioClip> getClip() const;

    void stop();
    void play();
    void setGain(float);
    float getGain() const;
    void setAttenuationRadius(float);
    float getAttenuationRadius() const;
    void setLooping(bool);
    bool isLooping() const;
    void setPosition(const gfxm::vec3& pos);
};

