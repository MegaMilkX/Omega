#pragma once

#include <map>
#include <memory>
#include <string>
#include "common/image/image.hpp"
#include "res_cache_interface.hpp"


class resCacheTexture2d : public resCacheInterface {
    std::map<std::string, std::unique_ptr<gpuTexture2d>> textures;
public:
    void* load(const char* path) {
        ktImage img;
        if (!loadImage(&img, path)) {
            return 0;
        }
        auto ptr = new gpuTexture2d;
        ptr->setData(&img);
        return ptr;
    }
    void* get(const char* name) override {
        auto it = textures.find(name);
        if (it == textures.end()) {
            gpuTexture2d* ptr = (gpuTexture2d*)load(name);
            it = textures.insert(std::make_pair(std::string(name), std::unique_ptr<gpuTexture2d>(ptr))).first;
        }
        return (void*)it->second.get();
    }
};
