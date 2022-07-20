#pragma once

#include "resource/res_cache_interface.hpp"

#include "animation/animation.hpp"

#include "animation/readwrite/rw_animation.hpp"


class resCacheAnimation : public resCacheInterface {
    std::map<std::string, HSHARED<Animation>> animations;

    bool loadAnimation(Animation* anim, const char* path) {
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

        return readAnimationBytes(&bytes[0], bytes.size(), anim);
    }

public:
    resCacheAnimation() {

    }
    HSHARED_BASE* get(const char* name) override {
        auto it = animations.find(name);
        if (it == animations.end()) {
            Handle<Animation> handle = HANDLE_MGR<Animation>::acquire();
            if (!loadAnimation(HANDLE_MGR<Animation>::deref(handle), name)) {
                HANDLE_MGR<Animation>::release(handle);
                return 0;
            }
            it = animations.insert(std::make_pair(std::string(name), HSHARED<Animation>(handle))).first;
            it->second.setReferenceName(name);
        }
        return &it->second;
    }
};
