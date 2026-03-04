#include "soundscape.hpp"


void Soundscape::addSoundEmitter(SoundEmitter3d* e) {
    e->chan = audioCreateChannel();
    audioSetBuffer(e->chan, e->clip->getBuffer());
    audioSetGain(e->chan, e->gain);
    audioSetLooping(e->chan, e->looping);
    audioSetPosition(e->chan, e->position);
    audioSetAttenuationRadius(e->chan, e->attenuation_radius);
    if (e->playing) {
        audioPlay3d(e->chan);
    }

    emitters.insert(e);
}

void Soundscape::removeSoundEmitter(SoundEmitter3d* e) {
    emitters.erase(e);

    audioFreeChannel(e->chan);
}

void Soundscape::update(float dt) {
    for (auto e : emitters) {
        audioSetPosition(e->chan, e->position);
    }
}

