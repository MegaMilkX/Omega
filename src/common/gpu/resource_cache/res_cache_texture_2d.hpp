#pragma once

#include <map>
#include <memory>
#include <string>
#include "image/image.hpp"
#include "resource/res_cache_interface.hpp"


class resCacheTexture2d : public resCacheInterfaceT<gpuTexture2d> {
    std::map<std::string, HSHARED<gpuTexture2d>> textures;
public:
    Handle<gpuTexture2d> load(const char* path) {
        ktImage img;
        if (!loadImage(&img, path)) {
            return Handle<gpuTexture2d>();
        }
        Handle<gpuTexture2d> handle = HANDLE_MGR<gpuTexture2d>::acquire();
        HANDLE_MGR<gpuTexture2d>::deref(handle)->setData(&img);
        HANDLE_MGR<gpuTexture2d>::deref(handle)->generateMipmaps();
        return handle;
    }
    HSHARED_BASE* get(const char* name) override {
        auto it = textures.find(name);
        if (it == textures.end()) {
            auto handle = load(name);
            if (!handle) {
                LOG_ERR("Failed to load texture " << name);
                return 0;
            }
            it = textures.insert(std::make_pair(std::string(name), HSHARED<gpuTexture2d>(handle))).first;
            it->second.setReferenceName(name);
        }
        return &it->second;
    }
    virtual HSHARED_BASE* find(const char* name) override {
        auto it = textures.find(name);
        if (it == textures.end()) {
            return 0;
        }
        return &it->second;
    }
    virtual void store(const char* name, HSHARED<gpuTexture2d> h) override {
        textures[name] = h;
    }
};
