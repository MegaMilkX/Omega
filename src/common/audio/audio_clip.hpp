#pragma once

#include <memory>
#include "resource_manager/resource_manager.hpp"
#include "audio_mixer.hpp"
#include "serialization/virtual_ibuf.hpp"

class AudioClip : public ILoadable {
    std::unique_ptr<AudioBuffer> buf;
public:
    AudioClip();
    AudioBuffer* getBuffer() { return buf.get(); }

    DEFINE_EXTENSIONS(e_ogg);
    bool load(byte_reader&) override;
};

