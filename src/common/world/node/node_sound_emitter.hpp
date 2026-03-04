#pragma once

#include "node_sound_emitter.auto.hpp"
#include "world/world.hpp"

#include "resource/resource.hpp"
#include "audio/audio.hpp"
#include "audio/soundscape.hpp"


// TODO: World does not have an Audio system,
// so no reason to inherit TActorNode, yet
[[cppi_class]];
class SoundEmitterNode : public ActorNode {
    SoundEmitter3d emitter;
    /*
    RHSHARED<AudioClip> clip;
    Handle<AudioChannel> chan;
    float attenuation_radius = 5.0f;
    float gain = .3f;
    bool looping = true;*/
public:
    TYPE_ENABLE();

    SoundEmitterNode();
    
    void play() {
        emitter.play();
    }
    void stop() {
        emitter.stop();
        //audioStop(chan);
    }

    void setClip(ResourceRef<AudioClip> clip) {
        emitter.setClip(clip);
        //this->clip = clip;
    }
    void setGain(float gain) {
        emitter.setGain(gain);
        //audioSetGain(chan, gain);
    }
    void setAttenuationRadius(float r) {
        emitter.setAttenuationRadius(r);
        //attenuation_radius = r;
    }
    void setLooping(bool l) {
        emitter.setLooping(l);
        //looping = l;
    }

    void onSpawnActorNode(WorldSystemRegistry& reg) override {
        if (auto sys = reg.getSystem<Soundscape>()) {
            sys->addSoundEmitter(&emitter);
            emitter.play();
        }
        /*
        chan = audioCreateChannel();
        audioSetAttenuationRadius(chan, attenuation_radius);
        audioSetLooping(chan, looping);
        audioSetBuffer(chan, clip->getBuffer());
        audioSetGain(chan, gain);
        audioPlay3d(chan);
        */
    }
    void onDespawnActorNode(WorldSystemRegistry& reg) override {
        if (auto sys = reg.getSystem<Soundscape>()) {
            sys->removeSoundEmitter(&emitter);
        }
        //audioFreeChannel(chan);
    }
};

