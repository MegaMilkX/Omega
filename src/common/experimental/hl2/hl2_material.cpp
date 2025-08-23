#include "hl2_material.hpp"

#include <assert.h>
#include <stdio.h>
#include <vector>
#include <map>
#include "log/log.hpp"

#include "hl2_vtf.hpp"

#include "valve_data/valve_data.hpp"
#include "valve_data/parser/parse.hpp"


bool hl2LoadMaterialFromMemory(const void* data, uint64_t size, RHSHARED<gpuMaterial>& material, const char* path_hint) {
    valve_data object;
    if (!valve::parse_material(object, (const char*)data, size)) {
        LOG_ERR("Failed to parse material");
        return false;
    }

    LOG("\n" << object.to_string());

    if (object.is_null()) {
        LOG_ERR("VMT is empty, failed to parse?");
        return false;
    }
    
    auto kv = object.begin();
    if (!kv.value().is_object()) {
        LOG_ERR(kv.key() << ": value must be an object");
        return false;
    }
    valve_data obj = kv.value();

    std::string material_type = kv.key();
    for (int i = 0; i < material_type.size(); ++i) {
        material_type[i] = std::tolower(material_type[i]);
    }
    LOG("Type: '" << material_type << "'");

    material.reset_acquire();

    int backface_culling = 1;
    int selfillum = 0;
    int basealphaenvmapmask = 0;
    int additive = 0;
    int translucent = 0;
    {
        if (material_type == "patch") {            
            std::string include = obj.get_string("include");
            if (!include.empty()) {
                include = MKSTR("experimental/hl2/" << include);
                LOG("Loading patched VMT: " << include);
                return hl2LoadMaterial(include.c_str(), material);
            } else {
                LOG_ERR("No 'include' key in material");
                return false;
            }
        }

        backface_culling = obj.get_string("$nocull") != "1";
        selfillum = obj.get_string("$selfillum") == "1";
        basealphaenvmapmask = obj.get_string("$basealphaenvmapmask") == "1";
        additive = obj.get_string("$additive") == "1";
        translucent = obj.get_string("$translucent") == "1";
        
        if (material_type == "water") {
            auto pass = material->addPass("HL2/Water");
            pass->setShaderProgram(resGet<gpuShaderProgram>("shaders/hl2/water.glsl"));

            std::string normalmap = obj.get_string("$normalmap");
            if(!normalmap.empty()) {
                RHSHARED<gpuTexture2d> htexture;// = resGet<gpuTexture2d>("core/textures/error_yellow.png");
                std::string tex_name = MKSTR("experimental/hl2/materials/" << normalmap << ".vtf");
                for(int i = 0; i < tex_name.size(); ++i) {
                    tex_name[i] = std::tolower(tex_name[i]);
                }
                if (!hl2LoadTexture(tex_name.c_str(), htexture)) {
                    LOG("Not found: " << tex_name);
                }
                material->addSampler("texNormal", htexture);
            } else {
                RHSHARED<gpuTexture2d> htexture = resGet<gpuTexture2d>("core/textures/error_yellow.png");
                material->addSampler("texNormal", htexture);

                LOG_WARN("Failed to read $normalmap, material: '" << path_hint << "'");
            }

            pass->cull_faces = backface_culling;
            pass = material->addPass("Wireframe");
            pass->setShaderProgram(resGet<gpuShaderProgram>("core/shaders/wireframe.glsl"));
        } else {
            std::string basetexture = obj.get_string("$basetexture");
            if(!basetexture.empty()) {
                RHSHARED<gpuTexture2d> htexture;// = resGet<gpuTexture2d>("core/textures/error_yellow.png");
                std::string tex_name = MKSTR("experimental/hl2/materials/" << basetexture << ".vtf");
                for(int i = 0; i < tex_name.size(); ++i) {
                    tex_name[i] = std::tolower(tex_name[i]);
                }
                if (!hl2LoadTexture(tex_name.c_str(), htexture)) {
                    LOG("Not found: " << tex_name);
                }
                material->addSampler("texAlbedo", htexture);
            } else {
                RHSHARED<gpuTexture2d> htexture = resGet<gpuTexture2d>("core/textures/error_yellow.png");
                material->addSampler("texAlbedo", htexture);

                LOG_WARN("Failed to read $basetexture, material: '" << path_hint << "'");
            }

            std::string texture2 = obj.get_string("$texture2");
            if (!texture2.empty()) {
                RHSHARED<gpuTexture2d> htexture;
                std::string tex_name = MKSTR("experimental/hl2/materials/" << texture2 << ".vtf");
                for(int i = 0; i < tex_name.size(); ++i) {
                    tex_name[i] = std::tolower(tex_name[i]);
                }
                if (!hl2LoadTexture(tex_name.c_str(), htexture)) {
                    LOG("Not found: " << tex_name);
                }
                material->addSampler("texAlbedo2", htexture);
            } else {
                RHSHARED<gpuTexture2d> htexture = resGet<gpuTexture2d>("core/textures/white.png");
                material->addSampler("texAlbedo2", htexture);

                LOG_WARN("Failed to read $texture2, material: '" << path_hint << "'");
            }

            gpuMaterialPass* pass = 0;
            if (translucent) {
                pass = material->addPass("HL2/Translucent");

                pass->depth_write = 0;
                pass->setShaderProgram(resGet<gpuShaderProgram>("shaders/hl2/translucent.glsl"));
            } else {
                pass = material->addPass("Default");

                if (basealphaenvmapmask) {
                    pass->setShaderProgram(resGet<gpuShaderProgram>("shaders/hl2/default_lightmapped_roughness.glsl"));
                } else if (selfillum) {
                    pass->setShaderProgram(resGet<gpuShaderProgram>("shaders/hl2/default_lightmapped_emission.glsl"));
                } else {
                    pass->setShaderProgram(resGet<gpuShaderProgram>("shaders/hl2/default_lightmapped.glsl"));
                }
            }

            if (additive) {
                pass->blend_mode = GPU_BLEND_MODE::ADD;
            }

            pass->cull_faces = backface_culling;
            pass = material->addPass("Wireframe");
            pass->setShaderProgram(resGet<gpuShaderProgram>("core/shaders/wireframe.glsl"));
        }

        {
            auto it = obj.find("$surfaceprop");
            if (it != obj.end()) {
                material->getExtraData()->operator[]("$surfaceprop") = it.value().as_string();
            }
        }
    }
    material->compile();

    return true;
}

bool hl2LoadMaterialImpl(const char* path, RHSHARED<gpuMaterial>& material) {
    LOG("Loading VMT: '" << path << "'");

    if (!path) {
        LOG_ERR("VMT file path is null");
        assert(false);
        return false;
    }

    FILE* f = fopen(path, "rb");
    if (!f) {
        LOG_ERR("Failed to open VMT file: " << path);
        //assert(false);
        return false;
    }

    fseek(f, 0, SEEK_END);
    uint64_t fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    std::vector<uint8_t> bytes(fsize);
    if (1 != fread(&bytes[0], fsize, 1, f)) {
        LOG_ERR("Failed to read VMT file");
        fclose(f);
        assert(false);
        return false;
    }
    fclose(f);

    return hl2LoadMaterialFromMemory(bytes.data(), bytes.size(), material, path);
}

#include <map>
static std::map<std::string, RHSHARED<gpuMaterial>> s_materials;
bool hl2LoadMaterial(const char* path, RHSHARED<gpuMaterial>& material) {
    auto it = s_materials.find(path);
    if (it != s_materials.end()) {
        material = it->second;
        return true;
    }

    if (!hl2LoadMaterialImpl(path, material)) {
        material = resGet<gpuMaterial>("materials/csg/missing.mat");
        s_materials[path] = material;
        LOG_WARN("VMT not found: '" << path << "'");
        return false;
    }

    s_materials[path] = material;
    return true;
}

void hl2StoreMaterial(const char* path, RHSHARED<gpuMaterial>& material) {
    s_materials[path] = material;
}

