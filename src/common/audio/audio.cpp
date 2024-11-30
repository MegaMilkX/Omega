
#include "audio.hpp"



static std::unique_ptr<AudioMixer> mixer;
AUDIO_STATS audio_stats;

bool audioInit() {
    mixer.reset(new AudioMixer());
    return mixer->init(44100, 16);
}

void audioCleanup() {
    mixer->cleanup();
    mixer.reset(0);
}

Handle<AudioChannel> audioCreateChannel() {
    return mixer->createChannel();
}
void audioFreeChannel(Handle<AudioChannel> chan) {
    mixer->freeChannel(chan);
}

void audioPlay(Handle<AudioChannel> chan) {
    mixer->play(chan);
}
void audioPlay3d(Handle<AudioChannel> chan) {
    mixer->play3d(chan);
}
bool audioIsPlaying(Handle<AudioChannel> chan) {
    return mixer->isPlaying(chan);
}
void audioStop(Handle<AudioChannel> chan) {
    mixer->stop(chan);
}

void audioSetListenerTransform(const gfxm::mat4& trans) {
    mixer->setListenerTransform(trans);
}

void audioSetPosition(Handle<AudioChannel> chan, const gfxm::vec3& pos) {
    mixer->setPosition(chan, pos);
}
void audioSetAttenuationRadius(Handle<AudioChannel> chan, float attenuation_radius) {
    mixer->setAttenuationRadius(chan, attenuation_radius);
}
void audioSetLooping(Handle<AudioChannel> chan, bool looping) {
    mixer->setLooping(chan, looping);
}
void audioSetBuffer(Handle<AudioChannel> chan, AudioBuffer* buf) {
    mixer->setBuffer(chan, buf);
}
void audioSetGain(Handle<AudioChannel> chan, float gain) {
    mixer->setGain(chan, gain);
}

void audioPlayOnce(AudioBuffer* buf, float vol, float pan) {
    mixer->playOnce(buf, vol, pan);
}
void audioPlayOnce3d(AudioBuffer* buf, const gfxm::vec3& pos, float vol, float attenuation_radius) {
    mixer->playOnce3d(buf, pos, vol, attenuation_radius);
}

const AUDIO_STATS& audioGetStats() {
    return audio_stats;
}

extern "C" {
#include "stb_vorbis.c"
}
