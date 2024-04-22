#pragma once

#include "node_sound_emitter.auto.hpp"
#include "world/world.hpp"

#include "resource/resource.hpp"
#include "audio/audio.hpp"


[[cppi_class]];
class SoundEmitterNode : public gameActorNode {
    RHSHARED<AudioClip> clip;
    Handle<AudioChannel> chan;
    float attenuation_radius = 5.0f;
    float gain = .3f;
    bool looping = true;
public:
    TYPE_ENABLE();
    void stop() {
        audioStop(chan);
    }

    void setClip(RHSHARED<AudioClip> clip) {
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
        audioSetPosition(chan, getWorldTranslation());
    }
    void onUpdate(RuntimeWorld* world, float dt) override {}
    void onDecay(RuntimeWorld* world) override {
        audioStop(chan);
    }
    void onSpawn(RuntimeWorld* world) override {
        chan = audioCreateChannel();
        audioSetAttenuationRadius(chan, attenuation_radius);
        audioSetLooping(chan, looping);
        audioSetBuffer(chan, clip->getBuffer());
        audioSetGain(chan, gain);
        audioPlay3d(chan);
    }
    void onDespawn(RuntimeWorld* world) override {
        audioFreeChannel(chan);
    }
};
