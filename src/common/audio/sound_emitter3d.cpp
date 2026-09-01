#include "sound_emitter3d.hpp"


void SoundEmitter3d::setClip(const ResourceRef<AudioClip>& clip) {
    this->clip = clip;
    if(!chan) return;
    // TODO: needs sync
    audioSetBuffer(chan, this->clip->getBuffer());
}
ResourceRef<AudioClip> SoundEmitter3d::getClip() const {
    return clip;
}

void SoundEmitter3d::stop() {
    playing = false;
    if(!chan) return;
    audioStop(chan);
}

void SoundEmitter3d::play() {
    playing = true;
    if(!chan) return;
    audioPlay3d(chan);
}

void SoundEmitter3d::setGain(float gain) {
    this->gain = gain;
    if(!chan) return;
    audioSetGain(chan, gain);
}
float SoundEmitter3d::getGain() const {
    return gain;
}

void SoundEmitter3d::setAttenuationRadius(float r) {
    attenuation_radius = r;
    if(!chan) return;
    audioSetAttenuationRadius(chan, r);
}
float SoundEmitter3d::getAttenuationRadius() const {
    return attenuation_radius;
}

void SoundEmitter3d::setLooping(bool l) {
    looping = l;
    if(!chan) return;
    audioSetLooping(chan, l);
}
bool SoundEmitter3d::isLooping() const {
    return looping;
}

void SoundEmitter3d::setPosition(const gfxm::vec3& pos) {
    position = pos;
    // TODO: dirty flag
}

