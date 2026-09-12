#pragma once

#include "audio_clip.auto.hpp"
#include <memory>
#include "resource_manager/resource.hpp"
#include "resource_manager/resource_manager.hpp"
#include "audio_mixer.hpp"
#include "serialization/virtual_ibuf.hpp"

[[cppi_class]];
class AudioClip
: public Resource
, public ILoadable {
    std::unique_ptr<AudioBuffer> buf;
public:
    AudioClip();
    AudioBuffer* getBuffer() { return buf.get(); }

    DEFINE_EXTENSIONS(e_ogg);
    bool load(byte_reader&) override;
};

