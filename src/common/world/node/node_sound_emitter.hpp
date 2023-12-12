#pragma once

#include "world/world.hpp"

#include "resource/resource.hpp"
#include "audio/audio_clip.hpp"


class SoundEmitterNode : public gameActorNode {
    TYPE_ENABLE();
    RHSHARED<AudioClip> clip;
    Handle<AudioChannel> chan;
    float attenuation_radius = 5.0f;
    float gain = .3f;
    bool looping = true;
public:
    void stop() {
        audio().stop(chan);
    }

    void setClip(RHSHARED<AudioClip>& clip) {
        this->clip = clip;
    }
    void setGain(float gain) {
        // TODO
    }
    void setAttenuationRadius(float r) {
        attenuation_radius = r;
    }
    void setLooping(bool l) {
        looping = l;
    }
    void onDefault() override {}
    void onUpdateTransform() override {
        audio().setPosition(chan, getWorldTranslation());
    }
    void onUpdate(RuntimeWorld* world, float dt) override {}
    void onDecay(RuntimeWorld* world) override {
        audio().stop(chan);
    }
    void onSpawn(RuntimeWorld* world) override {
        chan = audio().createChannel();
        audio().setAttenuationRadius(chan, attenuation_radius);
        audio().setLooping(chan, looping);
        audio().setBuffer(chan, clip->getBuffer());
        audio().setGain(chan, gain);
        audio().play3d(chan);
    }
    void onDespawn(RuntimeWorld* world) override {
        audio().freeChannel(chan);
    }
};
