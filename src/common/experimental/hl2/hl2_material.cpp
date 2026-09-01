#include "hl2_material.hpp"

#include <assert.h>
#include <stdio.h>
#include <vector>
#include <map>
#include "log/log.hpp"

#include "hl2_vtf.hpp"

#include "valve_data/valve_data.hpp"
#include "valve_data/parser/parse.hpp"

#include "gpu/shader_lib/shader_lib.hpp"
#include "resource_manager/resource_manager.hpp"
#include "resource/resource.hpp"
#include "gpu/material/pbr_material.hpp"
#include "gpu/material/vfx_material.hpp"
#include "gpu/material/water_material.hpp"


bool hl2LoadMaterialFromMemory(const void* data, uint64_t size, ResourceRef<gpuMaterial>& material, const char* path_hint) {
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

    int backface_culling = 1;
    int selfillum = 0;
    int basealphaenvmapmask = 0;
    int additive = 0;
    int translucent = 0;
    int alphatest = 0;
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

        gfxm::vec3 color = obj.get_vec3("$color", gfxm::vec3(1, 1, 1));
        gfxm::vec3 color2 = obj.get_vec3("$color2", gfxm::vec3(1, 1, 1));
        float alpha = obj.get_float("$alpha", 1.f);

        backface_culling = obj.get_string("$nocull") != "1";
        selfillum = obj.get_string("$selfillum") == "1";
        basealphaenvmapmask = obj.get_string("$basealphaenvmapmask") == "1";
        alphatest = obj.get_string("$alphatest") == "1";
        additive = obj.get_string("$additive") == "1";
        translucent = obj.get_string("$translucent") == "1";

        GPU_UVScrollMode scroll_mode = GPU_UVScrollMode::None;
        gfxm::vec2 uv_velo;
        GPU_UVScrollMode scroll_mode2 = GPU_UVScrollMode::None;
        gfxm::vec2 uv2_velo;
        if (auto proxies = obj.get("proxies")) {
            if (auto animatedtexture = proxies.get("animatedtexture")) {
                float framerate = animatedtexture.get_float("animatedtextureframerate");
                std::string framenumvar = animatedtexture.get_string("animatedtextureframenumvar");
                std::string texturevar = animatedtexture.get_string("animatedtexturevar");
                // TODO: ?
            }
            if(auto texturescroll = proxies.get("texturescroll")) {
                float angle = texturescroll.get_float("texturescrollangle");
                float rate = texturescroll.get_float("texturescrollrate");
                std::string scrollvar = texturescroll.get_string("texturescrollvar");
                gfxm::vec2 v = gfxm::vec2(cosf(gfxm::radian(angle)), sinf(gfxm::radian(angle)));
                v *= rate;
                if(scrollvar == "$texture2transform") {
                    uv2_velo = v;
                    scroll_mode2 = GPU_UVScrollMode::Smooth;
                } else {
                    uv_velo = v;
                    scroll_mode = GPU_UVScrollMode::Smooth;
                }
            }
        }

        bool is_lightmapped = false;

        const char* shader_name = "shaders/hl2/default_lightmapped.glsl";
        if (material_type == "water") {
            shader_name = "shaders/hl2/water.glsl";
        } else if (material_type == "vertexlitgeneric") {
            shader_name = "shaders/hl2/vertexlitgeneric.glsl";
        } else if (material_type == "lightmappedgeneric") {
            is_lightmapped = true;
            shader_name = "shaders/hl2/lightmappedgeneric.glsl";
        } else if (material_type == "eyes") {
            shader_name = "shaders/hl2/vertexlitgeneric.glsl";
        }

        int32_t alpha_mode = 0;
        if (alphatest) {
            alpha_mode = 0;
        } else if (basealphaenvmapmask) {
            alpha_mode = 1;
        } else if(selfillum) {
            alpha_mode = 2;
        }
        
        if (material_type == "water") {
            material = createResource<WaterMaterial>("");            
            material->setBackfaceCulling(backface_culling);

            auto watermat = dynamic_cast<WaterMaterial*>(material.get());

            std::string normalmap = obj.get_string("$normalmap");
            if(!normalmap.empty()) {
                ResourceRef<gpuTexture2d> htexture;
                std::string tex_name = MKSTR("experimental/hl2/materials/" << normalmap << ".vtf");
                for(int i = 0; i < tex_name.size(); ++i) {
                    tex_name[i] = std::tolower(tex_name[i]);
                }
                if (!hl2LoadTexture(tex_name.c_str(), htexture)) {
                    LOG("Not found: " << tex_name);
                }
                watermat->setNormalMap(htexture);
            } else {
                LOG_WARN("Failed to read $normalmap, material: '" << path_hint << "'");
            }
            /*
            pass->cull_faces = backface_culling;
            pass = material->addPass("Wireframe");
            //pass->setShaderProgram(resGet<gpuShaderProgram>("core/shaders/wireframe.glsl"));
            pass->addShaderSet(loadResource<gpuShaderSet>("file://core/shaders/wireframe.glsl"));
            */
        } else if(material_type == "unlittwotexture") {
            material = createResource<VFXMaterial>("");
            auto mat = dynamic_cast<VFXMaterial*>(material.get());

            //gfxm::vec4 rgb(color * color2, 1);
            //mat->setRGBA(gfxm::vec4(rgb, alpha));

            mat->setBackfaceCulling(backface_culling);

            mat->setUVScrollMode(scroll_mode);
            mat->setUVScrollVelocity(uv_velo);
            mat->setUV2ScrollMode(scroll_mode2);
            mat->setUV2ScrollVelocity(uv2_velo);

            std::string basetexture = obj.get_string("$basetexture");
            if(!basetexture.empty()) {
                ResourceRef<gpuTexture2d> htexture;
                std::string tex_name = MKSTR("experimental/hl2/materials/" << basetexture << ".vtf");
                for(int i = 0; i < tex_name.size(); ++i) {
                    tex_name[i] = std::tolower(tex_name[i]);
                }
                if (!hl2LoadTexture(tex_name.c_str(), htexture)) {
                    LOG("Not found: " << tex_name);
                }
                mat->setBaseTexture(htexture);
            } else {
                ResourceRef<gpuTexture2d> htexture = loadResource<gpuTexture2d>("core/textures/error_yellow");
                mat->setBaseTexture(htexture);

                LOG_WARN("Failed to read $basetexture, material: '" << path_hint << "'");
            }
            
            std::string texture2 = obj.get_string("$texture2");
            if(!texture2.empty()) {
                ResourceRef<gpuTexture2d> htexture;
                std::string tex_name = MKSTR("experimental/hl2/materials/" << texture2 << ".vtf");
                for(int i = 0; i < tex_name.size(); ++i) {
                    tex_name[i] = std::tolower(tex_name[i]);
                }
                if (!hl2LoadTexture(tex_name.c_str(), htexture)) {
                    LOG("Not found: " << tex_name);
                }
                mat->setTexture2(htexture);
            }
        } else {
            material = createResource<PBRMaterial>("");
            auto pbrmat = dynamic_cast<PBRMaterial*>(material.get());

            gfxm::vec4 rgb(color * color2, 1);
            pbrmat->setRGBA(gfxm::vec4(rgb, alpha));

            if (is_lightmapped) {
                pbrmat->setLightmapped(true);
            }

            pbrmat->setUVScrollMode(scroll_mode);
            pbrmat->setUVScrollVelocity(uv_velo);

            std::string basetexture = obj.get_string("$basetexture");
            if(!basetexture.empty()) {
                ResourceRef<gpuTexture2d> htexture;
                std::string tex_name = MKSTR("experimental/hl2/materials/" << basetexture << ".vtf");
                for(int i = 0; i < tex_name.size(); ++i) {
                    tex_name[i] = std::tolower(tex_name[i]);
                }
                if (!hl2LoadTexture(tex_name.c_str(), htexture)) {
                    LOG("Not found: " << tex_name);
                }
                pbrmat->setAlbedoMap(htexture);
            } else {
                ResourceRef<gpuTexture2d> htexture = loadResource<gpuTexture2d>("core/textures/error_yellow");
                pbrmat->setAlbedoMap(htexture);

                LOG_WARN("Failed to read $basetexture, material: '" << path_hint << "'");
            }

            std::string bumpmap = obj.get_string("$bumpmap");
            if (!bumpmap.empty()) {                
                ResourceRef<gpuTexture2d> htexture;
                std::string tex_name = MKSTR("experimental/hl2/materials/" << bumpmap << ".vtf");
                for(int i = 0; i < tex_name.size(); ++i) {
                    tex_name[i] = std::tolower(tex_name[i]);
                }
                if (!hl2LoadTexture(tex_name.c_str(), htexture)) {
                    LOG("Not found: " << tex_name);
                }
                pbrmat->setNormalMap(htexture);
            }

            std::string texture2 = obj.get_string("$texture2");
            if (!texture2.empty()) {
                ResourceRef<gpuTexture2d> htexture;
                std::string tex_name = MKSTR("experimental/hl2/materials/" << texture2 << ".vtf");
                for(int i = 0; i < tex_name.size(); ++i) {
                    tex_name[i] = std::tolower(tex_name[i]);
                }
                if (!hl2LoadTexture(tex_name.c_str(), htexture)) {
                    LOG("Not found: " << tex_name);
                }
                material->addSampler("texAlbedo2", htexture);
            } else {
                ResourceRef<gpuTexture2d> htexture = loadResource<gpuTexture2d>("core/textures/white");
                material->addSampler("texAlbedo2", htexture);

                LOG_WARN("Failed to read $texture2, material: '" << path_hint << "'");
            }

            material->setParamInt("alpha_mode", alpha_mode);
            material->setRoleOverride(GPU_Role_Geometry);
            if (alphatest) {
                pbrmat->setAlphaMode(GPU_AlphaMode::Discard);
                pbrmat->setBlendingMode(GPU_BLEND_MODE::OVERWRITE);
            }
            if (translucent) {
                pbrmat->setAlphaMode(GPU_AlphaMode::Blend);
                pbrmat->setBlendingMode(GPU_BLEND_MODE::BLEND);
                pbrmat->setDepthWrite(false);
                pbrmat->setTransparent(true);
            }
            if (additive) {
                pbrmat->setAlphaMode(GPU_AlphaMode::Blend);
                pbrmat->setBlendingMode(GPU_BLEND_MODE::ADD);
                pbrmat->setTransparent(true);
                pbrmat->setDepthWrite(false);
            }
            material->setBackfaceCulling(backface_culling);
            if (material_type == "lightmappedgeneric") {
                // TODO: Separate material class for HL2
                //material->setVertexExtension(loadResource<gpuShaderSet>("core/shaders/modular/lightmappedgeneric.vert"));
                //material->setFragmentExtension(loadResource<gpuShaderSet>("core/shaders/modular/lightmappedgeneric.frag"));
            } else if (material_type == "vertexlitgeneric") {
                //material->setFragmentExtension(loadResource<gpuShaderSet>("core/shaders/modular/vertexlitgeneric.frag"));
            } else if (material_type == "eyes") {
                //material->setFragmentExtension(loadResource<gpuShaderSet>("core/shaders/modular/vertexlitgeneric.frag"));
            } else if (material_type == "worldvertextransition") {
                //material->setVertexExtension(loadResource<gpuShaderSet>("core/shaders/modular/lightmappedgeneric.vert"));
                //material->setFragmentExtension(loadResource<gpuShaderSet>("core/shaders/modular/lightmappedgeneric.frag"));
            } else {
                //material->setFragmentExtension(loadResource<gpuShaderSet>("core/shaders/modular/vertexlitgeneric.frag"));
            }
            /*
            gpuMaterialPass* pass = 0;
            if (translucent) {
                pass = material->addPass("HL2/Translucent");

                pass->depth_write = 0;
                pass->addShaderSet(loadResource<gpuShaderSet>(std::string("file://") + shader_name));
                //pass->setShaderProgram(resGet<gpuShaderProgram>(shader_name));
                //pass->setShaderProgram(resGet<gpuShaderProgram>("shaders/hl2/translucent.glsl"));
            } else {
                pass = material->addPass("Default");
                pass->blend_mode = GPU_BLEND_MODE::OVERWRITE;
                pass->addShaderSet(loadResource<gpuShaderSet>(std::string("file://") + shader_name));
                //pass->setShaderProgram(resGet<gpuShaderProgram>(shader_name));
                
                if (basealphaenvmapmask) {
                    //pass->setShaderProgram(resGet<gpuShaderProgram>("shaders/hl2/default_lightmapped_roughness.glsl"));
                } else if (selfillum) {
                    //pass->setShaderProgram(resGet<gpuShaderProgram>("shaders/hl2/default_lightmapped_emission.glsl"));
                } else {
                    //pass->setShaderProgram(resGet<gpuShaderProgram>("shaders/hl2/default_lightmapped.glsl"));
                }
            }

            if (additive) {
                pass->blend_mode = GPU_BLEND_MODE::ADD;
            }

            pass->cull_faces = backface_culling;
            pass = material->addPass("Wireframe");
            //pass->setShaderProgram(resGet<gpuShaderProgram>("core/shaders/wireframe.glsl"));
            pass->addShaderSet(loadResource<gpuShaderSet>("file://core/shaders/wireframe.glsl"));
            */
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

bool hl2LoadMaterialImpl(const char* path, ResourceRef<gpuMaterial>& material) {
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
static std::map<std::string, ResourceRef<gpuMaterial>> s_materials;
bool hl2LoadMaterial(const char* path, ResourceRef<gpuMaterial>& material) {
    auto it = s_materials.find(path);
    if (it != s_materials.end()) {
        material = it->second;
        return true;
    }

    if (!hl2LoadMaterialImpl(path, material)) {
        material = loadResource<gpuMaterial>("materials/csg/missing");
        s_materials[path] = material;
        LOG_WARN("VMT not found: '" << path << "'");
        return false;
    }

    s_materials[path] = material;
    return true;
}

void hl2StoreMaterial(const char* path, ResourceRef<gpuMaterial>& material) {
    s_materials[path] = material;
}

