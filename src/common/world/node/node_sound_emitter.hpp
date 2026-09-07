#pragma once

#include "node_sound_emitter.auto.hpp"
#include "world/world.hpp"

#include "resource/resource.hpp"
#include "audio/audio.hpp"
#include "audio/soundscape.hpp"


[[cppi_class]];
class SoundEmitterNode : public ActorNode {
    SoundEmitter3d emitter;

public:
    TYPE_ENABLE();

    [[cppi_decl]]
    bool play_on_spawn = true;


    SoundEmitterNode();
    
    void play() {
        emitter.play();
    }
    void stop() {
        emitter.stop();
    }

    [[cppi_decl, set("clip")]]
    void setClip(ResourceRef<AudioClip> clip) {
        emitter.setClip(clip);
        emitter.stop();
        requestRebuild();
    }
    [[cppi_decl, get("clip")]]
    ResourceRef<AudioClip> getClip() const { return emitter.getClip(); }
    
    [[cppi_decl, set("gain")]]
    void setGain(float gain) { emitter.setGain(gain); }
    [[cppi_decl, get("gain")]]
    float getGain() const { return emitter.getGain(); }

    [[cppi_decl, set("attenuation_radius")]]
    void setAttenuationRadius(float r) { emitter.setAttenuationRadius(r); }
    [[cppi_decl, get("attenuation_radius")]]
    float getAttenuationRadius() const { return emitter.getAttenuationRadius(); }

    [[cppi_decl, set("looping")]]
    void setLooping(bool l) { emitter.setLooping(l); }
    [[cppi_decl, get("looping")]]
    bool isLooping() const { return emitter.isLooping(); }

    void onSpawnActorNode(WorldSystemRegistry& reg) override {
        if (!emitter.getClip()) {
            return;
        }

        if (auto sys = reg.getSystem<Soundscape>()) {
            emitter.stop();
            sys->addSoundEmitter(&emitter);
            if(play_on_spawn) {
                emitter.play();
            }
        }
    }
    void onDespawnActorNode(WorldSystemRegistry& reg) override {
        if (!emitter.getClip()) {
            return;
        }
        
        if (auto sys = reg.getSystem<Soundscape>()) {
            sys->removeSoundEmitter(&emitter);
        }
        //audioFreeChannel(chan);
    }
};

