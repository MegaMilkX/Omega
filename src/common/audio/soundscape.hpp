#pragma once

#include "sound_emitter3d.hpp"


class Soundscape {
    std::set<SoundEmitter3d*> emitters;
public:
    void addSoundEmitter(SoundEmitter3d*);
    void removeSoundEmitter(SoundEmitter3d*);
    void update(float dt);
};

