#pragma once

#include "resource/res_cache_interface.hpp"

#include "common/render/gpu_material.hpp"
#include "common/render/gpu_pipeline.hpp"

#include <fstream>
#include <nlohmann/json.hpp>

#include "game/resource/readwrite/rw_gpu_material.hpp"


class resCacheGpuMaterial : public resCacheInterface {
    gpuPipeline* pipeline = 0;
    std::map<std::string, HSHARED<gpuMaterial>> materials;

    bool loadMaterialJson(gpuMaterial* mat, const char* path) {
        std::ifstream f(path);
        if (!f) {
            LOG_ERR("Material file not found '" << path << "'");
            assert(false);
            return false;
        }
        nlohmann::json json;
        f >> json;

        return readGpuMaterialJson(json, mat);
    }

public:
    resCacheGpuMaterial(gpuPipeline* pipeline)
    : pipeline(pipeline) {
        
    }
    HSHARED_BASE* get(const char* name) override {
        auto it = materials.find(name);
        if (it == materials.end()) {            
            Handle<gpuMaterial> handle = HANDLE_MGR<gpuMaterial>::acquire();
            if (!loadMaterialJson(HANDLE_MGR<gpuMaterial>::deref(handle), name)) {
                HANDLE_MGR<gpuMaterial>::release(handle);
                return 0;
            }
            it = materials.insert(std::make_pair(std::string(name), HSHARED<gpuMaterial>(handle))).first;
            it->second.setReferenceName(name);
        }
        return &it->second;
    }
};
