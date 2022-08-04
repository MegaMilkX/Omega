#pragma once

#include <map>
#include "resource/res_cache_interface.hpp"

#include "audio/audio_clip.hpp"


class resCacheAudioClip : public resCacheInterface {
    std::map<std::string, HSHARED<AudioClip>> clips;

    bool load(AudioClip* clip, const char* path) {
        FILE* f = fopen(path, "rb");
        if (!f) {
            return false;
        }
        fseek(f, 0, SEEK_END);
        long fsize = ftell(f);
        fseek(f, 0, SEEK_SET);
        std::vector<unsigned char> bytes(fsize, 0);
        fread(&bytes[0], fsize, 1, f);
        fclose(f);

        return clip->deserialize(&bytes[0], bytes.size());
    }

public:
    resCacheAudioClip() {}
    HSHARED_BASE* get(const char* name) override {
        auto it = clips.find(name);
        if (it == clips.end()) {
            Handle<AudioClip> handle = HANDLE_MGR<AudioClip>::acquire();
            if (!load(HANDLE_MGR<AudioClip>::deref(handle), name)) {
                HANDLE_MGR<AudioClip>::release(handle);
                return 0;
            }
            it = clips.insert(std::make_pair(std::string(name), HSHARED<AudioClip>(handle))).first;
            it->second.setReferenceName(name);
        }
        return &it->second;
    }
};