#pragma once

#include "res_cache_interface.hpp"

#include "common/render/gpu_material.hpp"
#include "common/render/gpu_pipeline.hpp"

#include <fstream>
#include <nlohmann/json.hpp>


class resCacheGpuMaterial : public resCacheInterface {
    gpuPipeline* pipeline = 0;
    std::map<std::string, std::unique_ptr<gpuMaterial>> materials;

    bool loadMaterialJson(gpuMaterial* mat, const char* path) {
        std::ifstream f(path);
        nlohmann::json json;
        f >> json;

        if (!json.is_object()) {
            LOG_ERR("gpuMaterial root json is not an object");
            assert(false);
            return false;
        }

        auto j = json.find("samplers") != json.end() ? json.at("samplers") : nlohmann::json();
        if (j.is_object()) {
            for (auto& it_sampler : j.get<nlohmann::json::object_t>()) {
                std::string name = it_sampler.first;
                if (it_sampler.second.is_string()) {
                    std::string path = it_sampler.second.get<std::string>();
                    mat->addSampler(name.c_str(), resGet<gpuTexture2d>(path.c_str()));
                }
            }
        }

        auto j_techniques = json.find("techniques") != json.end() ? json.at("techniques") : nlohmann::json();
        for (auto& it : j_techniques.get<nlohmann::json::object_t>()) {
            if (!it.second.is_array()) {
                assert(false);
                LOG_ERR("Technique json node '" << it.first << "' must be an array");
                continue;
            }
            std::string tech_name = it.first;
            nlohmann::json j_pass_array = it.second;

            auto tech = mat->addTechnique(tech_name.c_str());

            for (auto& it_pass : j_pass_array) {
                if (!it_pass.is_object()) {
                    assert(false);
                    LOG_ERR("Pass json node must be an object");
                    continue;
                }

                auto pass = tech->addPass();

                auto j_shader = it_pass.find("shader") != it_pass.end() ? it_pass.at("shader") : nlohmann::json();
                if (j_shader.is_string()) {
                    std::string shader_path = j_shader.get<std::string>();
                    pass->setShader(resGet<gpuShaderProgram>(shader_path.c_str()));
                }

                auto j = it_pass.find("depth_test") != it_pass.end() ? it_pass.at("depth_test") : nlohmann::json();
                if (j.is_boolean()) {
                    pass->depth_test = j.get<bool>() ? 1 : 0;
                }

                j = it_pass.find("stencil_test") != it_pass.end() ? it_pass.at("stencil_test") : nlohmann::json();
                if (j.is_boolean()) {
                    pass->stencil_test = j.get<bool>() ? 1 : 0;
                }

                j = it_pass.find("cull_faces") != it_pass.end() ? it_pass.at("cull_faces") : nlohmann::json();
                if (j.is_boolean()) {
                    pass->cull_faces = j.get<bool>() ? 1 : 0;
                }

                j = it_pass.find("depth_write") != it_pass.end() ? it_pass.at("depth_write") : nlohmann::json();
                if (j.is_boolean()) {
                    pass->depth_write = j.get<bool>() ? 1 : 0;
                }
            }
        }

        mat->compile();
        return true;
    }

public:
    resCacheGpuMaterial(gpuPipeline* pipeline)
    : pipeline(pipeline) {
        
    }
    void* get(const char* name) override {
        auto it = materials.find(name);
        if (it == materials.end()) {
            gpuMaterial* ptr = pipeline->createMaterial();
            if (!loadMaterialJson(ptr, name)) {
                //delete ptr;
                return 0;
            }
            it = materials.insert(std::make_pair(std::string(name), std::unique_ptr<gpuMaterial>(ptr))).first;
        }
        return (void*)it->second.get();
    }
};
