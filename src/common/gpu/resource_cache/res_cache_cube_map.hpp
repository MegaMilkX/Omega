#pragma once

#include <map>
#include <memory>
#include <string>
#include "image/image.hpp"
#include "resource/res_cache_interface.hpp"
#include "gpu/gpu_cube_map.hpp"


class resCacheCubeMap : public resCacheInterface {
    std::map<std::string, HSHARED<gpuCubeMap>> textures;
public:
    Handle<gpuCubeMap> load(const char* path) {
        ktImage img;
        if (!loadImage(&img, path)) {
            return Handle<gpuCubeMap>();
        }
        Handle<gpuCubeMap> handle = HANDLE_MGR<gpuCubeMap>::acquire();
        HANDLE_MGR<gpuCubeMap>::deref(handle)->setData(&img);
        return handle;
    }
    HSHARED_BASE* get(const char* name) override {
        auto it = textures.find(name);
        if (it == textures.end()) {
            auto handle = load(name);
            it = textures.insert(std::make_pair(std::string(name), HSHARED<gpuCubeMap>(handle))).first;
            it->second.setReferenceName(name);
        }
        return &it->second;
    }
};
