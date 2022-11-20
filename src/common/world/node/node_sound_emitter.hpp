#pragma once

#include "world/world.hpp"

#include "resource/resource.hpp"
#include "audio/audio_clip.hpp"


class nodeSoundEmitter : public gameActorNode {
    TYPE_ENABLE(gameActorNode);
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
    void onUpdate(gameWorld* world, float dt) override {}
    void onDecay(gameWorld* world) override {
        audio().stop(chan);
    }
    void onSpawn(gameWorld* world) override {
        chan = audio().createChannel();
        audio().setAttenuationRadius(chan, attenuation_radius);
        audio().setLooping(chan, looping);
        audio().setBuffer(chan, clip->getBuffer());
        audio().setGain(chan, gain);
        audio().play3d(chan);
    }
    void onDespawn(gameWorld* world) override {
        audio().freeChannel(chan);
    }
};
STATIC_BLOCK{
    type_register<nodeSoundEmitter>("nodeSoundEmitter")
        .parent<gameActorNode>();
};
