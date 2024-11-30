#pragma once

#include "resource/res_cache_interface.hpp"

#include "gpu/gpu_material.hpp"
#include "gpu/gpu_pipeline.hpp"

#include <fstream>
#include <nlohmann/json.hpp>

#include "gpu/readwrite/rw_gpu_material.hpp"


class resCacheGpuMaterial : public resCacheInterfaceT<gpuMaterial> {
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
    virtual HSHARED_BASE* find(const char* name) override {
        auto it = materials.find(name);
        if (it == materials.end()) {
            return 0;
        }
        return &it->second;
    }
    virtual void store(const char* name, HSHARED<gpuMaterial> h) override {
        materials[name] = h;
    }
};
