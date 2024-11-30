#pragma once

#include "audio_mixer.hpp"
#include "audio_clip.hpp"


bool audioInit();
void audioCleanup();

Handle<AudioChannel> audioCreateChannel();
void audioFreeChannel(Handle<AudioChannel> chan);

void audioPlay(Handle<AudioChannel> chan);
void audioPlay3d(Handle<AudioChannel> chan);
bool audioIsPlaying(Handle<AudioChannel> chan);
void audioStop(Handle<AudioChannel> chan);

void audioSetListenerTransform(const gfxm::mat4& trans);

void audioSetPosition(Handle<AudioChannel> chan, const gfxm::vec3& pos);
void audioSetAttenuationRadius(Handle<AudioChannel> chan, float attenuation_radius);
void audioSetLooping(Handle<AudioChannel> chan, bool looping);
void audioSetBuffer(Handle<AudioChannel> chan, AudioBuffer* buf);
void audioSetGain(Handle<AudioChannel> chan, float gain);

void audioPlayOnce(AudioBuffer* buf, float vol = 1.f, float pan = .0f);
void audioPlayOnce3d(AudioBuffer* buf, const gfxm::vec3& pos, float vol = 1.0f, float attenuation_radius = 10.0f);

const AUDIO_STATS& audioGetStats();
