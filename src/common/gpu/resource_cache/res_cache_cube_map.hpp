#pragma once

#include <map>
#include <memory>
#include <string>
#include "image/image.hpp"
#include "resource/res_cache_interface.hpp"
#include "gpu/texture/cube_texture.hpp"


class resCacheCubeMap : public resCacheInterfaceT<gpuCubeTexture> {
    std::map<std::string, HSHARED<gpuCubeTexture>> textures;
public:
    Handle<gpuCubeTexture> load(const char* path) {
        ktImage img;
        if (!loadImage(&img, path)) {
            return Handle<gpuCubeTexture>();
        }
        Handle<gpuCubeTexture> handle = HANDLE_MGR<gpuCubeTexture>::acquire();
        HANDLE_MGR<gpuCubeTexture>::deref(handle)->setData(&img);
        return handle;
    }
    HSHARED_BASE* get(const char* name) override {
        auto it = textures.find(name);
        if (it == textures.end()) {
            auto handle = load(name);
            it = textures.insert(std::make_pair(std::string(name), HSHARED<gpuCubeTexture>(handle))).first;
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
    virtual void store(const char* name, HSHARED<gpuCubeTexture> h) override {
        textures[name] = h;
    }
};
