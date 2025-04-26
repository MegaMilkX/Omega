#include "rw_gpu_material.hpp"

#include "reflection/reflection.hpp"
#include "resource/resource.hpp"

#include "gpu/gpu.hpp"
#include "json/json.hpp"

bool readGpuMaterialJson(const nlohmann::json& json_, gpuMaterial* mat) {
    if (!json_.is_object()) {
        LOG_ERR("gpuMaterial root json is not an object");
        assert(false);
        return false;
    }

    nlohmann::json json = jsonPreprocessExtensions(json_);

    auto j = json.find("samplers") != json.end() ? json.at("samplers") : nlohmann::json();
    if (j.is_object()) {
        for (auto& it_sampler : j.get<nlohmann::json::object_t>()) {
            std::string name = it_sampler.first;
            HSHARED<gpuTexture2d> htex;
            type_get<HSHARED<gpuTexture2d>>().deserialize_json(it_sampler.second, &htex);
            mat->addSampler(name.c_str(), htex);
        }
    }

    auto it_passes = json.find("passes");
    while (it_passes != json.end()) {
        nlohmann::json jpasses = it_passes.value();
        if (!jpasses.is_object()) {
            LOG_ERR("'passes' must be an object");
            assert(false);
            break;
        }
        
        for (auto it = jpasses.begin(); it != jpasses.end(); ++it) {
            const std::string& key = it.key();
            const nlohmann::json& jpass = it.value();

            auto pass = mat->addPass(key.c_str());

            // Shader
            auto it_shader = jpass.find("shader");
            if (it_shader != jpass.end()) {
                const nlohmann::json jshader = it_shader.value();
                HSHARED<gpuShaderProgram> hprog;
                type_get<HSHARED<gpuShaderProgram>>().deserialize_json(jshader, &hprog);
                pass->setShader(hprog);
            }

            // Flags
            auto j = jpass.find("depth_test") != jpass.end() ? jpass.at("depth_test") : nlohmann::json();
            if (j.is_boolean()) {
                pass->depth_test = j.get<bool>() ? 1 : 0;
            }
            j = jpass.find("stencil_test") != jpass.end() ? jpass.at("stencil_test") : nlohmann::json();
            if (j.is_boolean()) {
                pass->stencil_test = j.get<bool>() ? 1 : 0;
            }
            j = jpass.find("cull_faces") != jpass.end() ? jpass.at("cull_faces") : nlohmann::json();
            if (j.is_boolean()) {
                pass->cull_faces = j.get<bool>() ? 1 : 0;
            }
            j = jpass.find("depth_write") != jpass.end() ? jpass.at("depth_write") : nlohmann::json();
            if (j.is_boolean()) {
                pass->depth_write = j.get<bool>() ? 1 : 0;
            }
            j = jpass.find("blend_mode") != jpass.end() ? jpass.at("blend_mode") : nlohmann::json();
            if (j.is_number_integer()) {
                pass->blend_mode = (GPU_BLEND_MODE)j.get<int>();
            }
        }

        break;
    }
    /*
    // === REPLACE BEGIN ===
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

        for (auto& it_pass : j_pass_array.get<nlohmann::json::array_t>()) {
            if (!it_pass.is_object()) {
                assert(false);
                LOG_ERR("Pass json node must be an object");
                continue;
            }

            auto pass = tech->addPass();

            auto j_shader = it_pass.find("shader") != it_pass.end() ? it_pass.at("shader") : nlohmann::json();
            HSHARED<gpuShaderProgram> hprog;
            type_get<HSHARED<gpuShaderProgram>>().deserialize_json(j_shader, &hprog);
            pass->setShader(hprog);

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

            j = it_pass.find("blend_mode") != it_pass.end() ? it_pass.at("blend_mode") : nlohmann::json();
            if (j.is_number_integer()) {
                pass->blend_mode = (GPU_BLEND_MODE)j.get<int>();
            }
        }
    }
    // === REPLACE END ===
    */
    mat->compile();
    return true;
}
bool writeGpuMaterialJson(nlohmann::json& j, gpuMaterial* mat) {
    j = nlohmann::json::object();

    //auto& jtechniques = j["techniques"];
    auto& jpasses     = j["passes"];
    auto& jsamplers   = j["samplers"];

    for (int i = 0; i < mat->passCount(); ++i) {
        auto pass = mat->getPass(i);
        nlohmann::json jpass;
        auto& hshader = pass->getShaderHandle();

        type_get<HSHARED<gpuShaderProgram>>().serialize_json(jpass["shader"], &hshader);
        jpass["depth_test"] = pass->depth_test != 0 ? true : false;
        jpass["stencil_test"] = pass->stencil_test != 0 ? true : false;
        jpass["cull_faces"] = pass->cull_faces != 0 ? true : false;
        jpass["depth_write"] = pass->depth_write != 0 ? true : false;
        jpass["blend_mode"] = (int)pass->blend_mode;

        jpasses[pass->getPath()] = jpass;
    }
    /*
    // === REPLACE BEGIN ===
    for (int i = 0; i < mat->techniqueCount(); ++i) {
        nlohmann::json jtech;
        auto tech = mat->getTechniqueByLocalId(i);
        auto tech_name = mat->getTechniqueName(i);
        for (int j = 0; j < tech->passCount(); ++j) {
            nlohmann::json jpass;
            auto pass = tech->getPass(j);
            auto& hshader = pass->getShaderHandle();
            
            type_get<HSHARED<gpuShaderProgram>>().serialize_json(jpass["shader"], &hshader);
            jpass["depth_test"] = pass->depth_test != 0 ? true : false;
            jpass["stencil_test"] = pass->stencil_test != 0 ? true : false;
            jpass["cull_faces"] = pass->cull_faces != 0 ? true : false;
            jpass["depth_write"] = pass->depth_write != 0 ? true : false;
            jpass["blend_mode"] = (int)pass->blend_mode;

            jtech.push_back(jpass);
        }
        jtechniques[tech_name] = jtech;
    }
    // === REPLACE END ===
    */
    for (int i = 0; i < mat->samplerCount(); ++i) {
        auto& hsampler = mat->getSampler(i);
        std::string sampler_name = mat->getSamplerName(i);
        nlohmann::json jh;
        type_get<HSHARED<gpuTexture2d>>().serialize_json(jh, &hsampler);
        jsamplers[sampler_name] = jh;
    }

    return true;
}