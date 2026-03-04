#include "rw_gpu_material.hpp"

#include "reflection/reflection.hpp"
#include "resource/resource.hpp"

#include "gpu/gpu.hpp"
#include "gpu/shader_lib/shader_lib.hpp"
#include "json/json.hpp"

#include "resource_manager/resource_manager.hpp"
#include "gpu/shader_set.hpp"


static bool readParamFromJson(gpuMaterial* mat, const std::string& name, GLenum type, const nlohmann::json& j) {
    switch (type) {
    case GL_FLOAT: {   // float
        if (!j.is_number()) {
            LOG_ERR("Material param '" << name << "' must be a number");
            return false;
        }
        mat->setParamFloat(name, j.get<float>());
        break;
    }
    case GL_FLOAT_VEC2: {   // vec2
        if (!j.is_array()) {
            LOG_ERR("Material param '" << name << "' must be an array");
            return false;
        }
        if (j.size() != 2) {
            LOG_ERR("Material param '" << name << "' must be of size 2");
            return false;
        }
        if (!j[0].is_number() || !j[1].is_number()) {
            LOG_ERR("Material param '" << name << "': all elements must be numbers");
            return false;
        }
        mat->setParamVec2(name, gfxm::vec2(j[0].get<float>(), j[1].get<float>()));
        break;
    }
    case GL_FLOAT_VEC3: {   // vec3
        if (!j.is_array()) {
            LOG_ERR("Material param '" << name << "' must be an array");
            return false;
        }
        if (j.size() != 3) {
            LOG_ERR("Material param '" << name << "' must be of size 3");
            return false;
        }
        if (!j[0].is_number() || !j[1].is_number() || !j[2].is_number()) {
            LOG_ERR("Material param '" << name << "': all elements must be numbers");
            return false;
        }
        mat->setParamVec3(name, gfxm::vec3(j[0].get<float>(), j[1].get<float>(), j[2].get<float>()));
        break;
    }
    case GL_FLOAT_VEC4: {   // vec4
        if (!j.is_array()) {
            LOG_ERR("Material param '" << name << "' must be an array");
            return false;
        }
        if (j.size() != 4) {
            LOG_ERR("Material param '" << name << "' must be of size 4");
            return false;
        }
        if (!j[0].is_number() || !j[1].is_number() || !j[2].is_number() || !j[3].is_number()) {
            LOG_ERR("Material param '" << name << "': all elements must be numbers");
            return false;
        }
        mat->setParamVec4(name, gfxm::vec4(j[0].get<float>(), j[1].get<float>(), j[2].get<float>(), j[3].get<float>()));
        break;
    }/*
    case GL_DOUBLE: {   // double                
        break;
    }
    case GL_DOUBLE_VEC2: {       // dvec2
        break;
    }
    case GL_DOUBLE_VEC3: {       // dvec3
        break;
    }
    case GL_DOUBLE_VEC4: {       // dvec4
        break;
    }*/
    case GL_INT: {       // int
        if (!j.is_number()) {
            LOG_ERR("Material param '" << name << "' must be a number");
            return false;
        }
        mat->setParamInt(name, j.get<int>());
        break;
    }
    case GL_INT_VEC2: {   // ivec2
        if (!j.is_array()) {
            LOG_ERR("Material param '" << name << "' must be an array");
            return false;
        }
        if (j.size() != 2) {
            LOG_ERR("Material param '" << name << "' must be of size 2");
            return false;
        }
        if (!j[0].is_number() || !j[1].is_number()) {
            LOG_ERR("Material param '" << name << "': all elements must be numbers");
            return false;
        }
        mat->setParamVec2i(name, gfxm::ivec2(j[0].get<int>(), j[1].get<int>()));
        break;
    }
    case GL_INT_VEC3: {   // ivec3
        if (!j.is_array()) {
            LOG_ERR("Material param '" << name << "' must be an array");
            return false;
        }
        if (j.size() != 3) {
            LOG_ERR("Material param '" << name << "' must be of size 3");
            return false;
        }
        if (!j[0].is_number() || !j[1].is_number() || !j[2].is_number()) {
            LOG_ERR("Material param '" << name << "': all elements must be numbers");
            return false;
        }
        mat->setParamVec3i(name, gfxm::ivec3(j[0].get<int>(), j[1].get<int>(), j[2].get<int>()));
        break;
    }
    case GL_INT_VEC4: {   // ivec4
        if (!j.is_array()) {
            LOG_ERR("Material param '" << name << "' must be an array");
            return false;
        }
        if (j.size() != 4) {
            LOG_ERR("Material param '" << name << "' must be of size 4");
            return false;
        }
        if (!j[0].is_number() || !j[1].is_number() || !j[2].is_number() || !j[3].is_number()) {
            LOG_ERR("Material param '" << name << "': all elements must be numbers");
            return false;
        }
        mat->setParamVec4i(name, gfxm::ivec4(j[0].get<int>(), j[1].get<int>(), j[2].get<int>(), j[3].get<int>()));
        break;
    }
    case GL_UNSIGNED_INT: {   // unsigned int
        if (!j.is_number()) {
            LOG_ERR("Material param '" << name << "' must be a number");
            return false;
        }
        mat->setParamInt(name, j.get<int>());
        break;
    }
    case GL_UNSIGNED_INT_VEC2: {   // uvec2
        if (!j.is_array()) {
            LOG_ERR("Material param '" << name << "' must be an array");
            return false;
        }
        if (j.size() != 2) {
            LOG_ERR("Material param '" << name << "' must be of size 2");
            return false;
        }
        if (!j[0].is_number() || !j[1].is_number()) {
            LOG_ERR("Material param '" << name << "': all elements must be numbers");
            return false;
        }
        mat->setParamVec2i(name, gfxm::ivec2(j[0].get<int>(), j[1].get<int>()));
        break;
    }
    case GL_UNSIGNED_INT_VEC3: {   // uvec3
        if (!j.is_array()) {
            LOG_ERR("Material param '" << name << "' must be an array");
            return false;
        }
        if (j.size() != 3) {
            LOG_ERR("Material param '" << name << "' must be of size 3");
            return false;
        }
        if (!j[0].is_number() || !j[1].is_number() || !j[2].is_number()) {
            LOG_ERR("Material param '" << name << "': all elements must be numbers");
            return false;
        }
        mat->setParamVec3i(name, gfxm::ivec3(j[0].get<int>(), j[1].get<int>(), j[2].get<int>()));
        break;
    }
    case GL_UNSIGNED_INT_VEC4: {   // uvec4
        if (!j.is_array()) {
            LOG_ERR("Material param '" << name << "' must be an array");
            return false;
        }
        if (j.size() != 4) {
            LOG_ERR("Material param '" << name << "' must be of size 4");
            return false;
        }
        if (!j[0].is_number() || !j[1].is_number() || !j[2].is_number() || !j[3].is_number()) {
            LOG_ERR("Material param '" << name << "': all elements must be numbers");
            return false;
        }
        mat->setParamVec4i(name, gfxm::ivec4(j[0].get<int>(), j[1].get<int>(), j[2].get<int>(), j[3].get<int>()));
        break;
    }/*
    case GL_BOOL: {  //  bool
        break;
    }
    case GL_BOOL_VEC2: {   // bvec2
        break;
    }
    case GL_BOOL_VEC3: {   // bvec3
        break;
    }
    case GL_BOOL_VEC4: {   // bvec4
        break;
    }*/
    case GL_FLOAT_MAT2: {   // mat2
        const int COUNT = 4;
        if (!j.is_array()) {
            LOG_ERR("Material param '" << name << "' must be an array");
            return false;
        }
        if (j.size() != COUNT) {
            LOG_ERR("Material param '" << name << "' must be of size " << COUNT);
            return false;
        }
        for (int i = 0; i < COUNT; ++i) {
            if (!j[i].is_number()) {
                LOG_ERR("Material param '" << name << "': all elements must be numbers");
                return false;
            }
        }
        float m[COUNT];
        for (int i = 0; i < COUNT; ++i) {
            m[i] = j[i].get<float>();
        }
        mat->setParamMat2(name, m);
        break;
    }
    case GL_FLOAT_MAT3: {   // mat3
        const int COUNT = 9;
        if (!j.is_array()) {
            LOG_ERR("Material param '" << name << "' must be an array");
            return false;
        }
        if (j.size() != COUNT) {
            LOG_ERR("Material param '" << name << "' must be of size " << COUNT);
            return false;
        }
        for (int i = 0; i < COUNT; ++i) {
            if (!j[i].is_number()) {
                LOG_ERR("Material param '" << name << "': all elements must be numbers");
                return false;
            }
        }
        float m[COUNT];
        for (int i = 0; i < COUNT; ++i) {
            m[i] = j[i].get<float>();
        }
        mat->setParamMat3(name, m);
        break;
    }
    case GL_FLOAT_MAT4: {   // mat4
        const int COUNT = 16;
        if (!j.is_array()) {
            LOG_ERR("Material param '" << name << "' must be an array");
            return false;
        }
        if (j.size() != COUNT) {
            LOG_ERR("Material param '" << name << "' must be of size " << COUNT);
            return false;
        }
        for (int i = 0; i < COUNT; ++i) {
            if (!j[i].is_number()) {
                LOG_ERR("Material param '" << name << "': all elements must be numbers");
                return false;
            }
        }
        float m[COUNT];
        for (int i = 0; i < COUNT; ++i) {
            m[i] = j[i].get<float>();
        }
        mat->setParamMat4(name, m);
        break;
    }
    case GL_FLOAT_MAT2x3: {   // mat2x3
        const int COUNT = 6;
        if (!j.is_array()) {
            LOG_ERR("Material param '" << name << "' must be an array");
            return false;
        }
        if (j.size() != COUNT) {
            LOG_ERR("Material param '" << name << "' must be of size " << COUNT);
            return false;
        }
        for (int i = 0; i < COUNT; ++i) {
            if (!j[i].is_number()) {
                LOG_ERR("Material param '" << name << "': all elements must be numbers");
                return false;
            }
        }
        float m[COUNT];
        for (int i = 0; i < COUNT; ++i) {
            m[i] = j[i].get<float>();
        }
        mat->setParamMat2x3(name, m);
        break;
    }
    case GL_FLOAT_MAT2x4: {   // mat2x4
        const int COUNT = 8;
        if (!j.is_array()) {
            LOG_ERR("Material param '" << name << "' must be an array");
            return false;
        }
        if (j.size() != COUNT) {
            LOG_ERR("Material param '" << name << "' must be of size " << COUNT);
            return false;
        }
        for (int i = 0; i < COUNT; ++i) {
            if (!j[i].is_number()) {
                LOG_ERR("Material param '" << name << "': all elements must be numbers");
                return false;
            }
        }
        float m[COUNT];
        for (int i = 0; i < COUNT; ++i) {
            m[i] = j[i].get<float>();
        }
        mat->setParamMat2x4(name, m);
        break;
    }
    case GL_FLOAT_MAT3x2: {   // mat3x2
        const int COUNT = 6;
        if (!j.is_array()) {
            LOG_ERR("Material param '" << name << "' must be an array");
            return false;
        }
        if (j.size() != COUNT) {
            LOG_ERR("Material param '" << name << "' must be of size " << COUNT);
            return false;
        }
        for (int i = 0; i < COUNT; ++i) {
            if (!j[i].is_number()) {
                LOG_ERR("Material param '" << name << "': all elements must be numbers");
                return false;
            }
        }
        float m[COUNT];
        for (int i = 0; i < COUNT; ++i) {
            m[i] = j[i].get<float>();
        }
        mat->setParamMat3x2(name, m);
        break;
    }
    case GL_FLOAT_MAT3x4: {   // mat3x4
        const int COUNT = 12;
        if (!j.is_array()) {
            LOG_ERR("Material param '" << name << "' must be an array");
            return false;
        }
        if (j.size() != COUNT) {
            LOG_ERR("Material param '" << name << "' must be of size " << COUNT);
            return false;
        }
        for (int i = 0; i < COUNT; ++i) {
            if (!j[i].is_number()) {
                LOG_ERR("Material param '" << name << "': all elements must be numbers");
                return false;
            }
        }
        float m[COUNT];
        for (int i = 0; i < COUNT; ++i) {
            m[i] = j[i].get<float>();
        }
        mat->setParamMat3x4(name, m);
        break;
    }
    case GL_FLOAT_MAT4x2: {   // mat4x2
        const int COUNT = 8;
        if (!j.is_array()) {
            LOG_ERR("Material param '" << name << "' must be an array");
            return false;
        }
        if (j.size() != COUNT) {
            LOG_ERR("Material param '" << name << "' must be of size " << COUNT);
            return false;
        }
        for (int i = 0; i < COUNT; ++i) {
            if (!j[i].is_number()) {
                LOG_ERR("Material param '" << name << "': all elements must be numbers");
                return false;
            }
        }
        float m[COUNT];
        for (int i = 0; i < COUNT; ++i) {
            m[i] = j[i].get<float>();
        }
        mat->setParamMat4x2(name, m);
        break;
    }
    case GL_FLOAT_MAT4x3: {   // mat4x3
        const int COUNT = 12;
        if (!j.is_array()) {
            LOG_ERR("Material param '" << name << "' must be an array");
            return false;
        }
        if (j.size() != COUNT) {
            LOG_ERR("Material param '" << name << "' must be of size " << COUNT);
            return false;
        }
        for (int i = 0; i < COUNT; ++i) {
            if (!j[i].is_number()) {
                LOG_ERR("Material param '" << name << "': all elements must be numbers");
                return false;
            }
        }
        float m[COUNT];
        for (int i = 0; i < COUNT; ++i) {
            m[i] = j[i].get<float>();
        }
        mat->setParamMat4x3(name, m);
        break;
    }/*
    case GL_DOUBLE_MAT2: {       // dmat2
        break;
    }
    case GL_DOUBLE_MAT3: {       // dmat3
        break;
    }
    case GL_DOUBLE_MAT4: {       // dmat4
        break;
    }
    case GL_DOUBLE_MAT2x3: {   // dmat2x3
        break;
    }
    case GL_DOUBLE_MAT2x4: {   // dmat2x4
        break;
    }
    case GL_DOUBLE_MAT3x2: {   // dmat3x2
        break;
    }
    case GL_DOUBLE_MAT3x4: {   // dmat3x4
        break;
    }
    case GL_DOUBLE_MAT4x2: {   // dmat4x2
        break;
    }
    case GL_DOUBLE_MAT4x3: {   // dmat4x3
        break;
    }*//*
    case GL_SAMPLER_1D: {   // sampler1D
        break;
    }
    case GL_SAMPLER_2D: {   // sampler2D
        break;
    }
    case GL_SAMPLER_3D: {   // sampler3D
        break;
    }
    case GL_SAMPLER_CUBE: {   // samplerCube
        break;
    }
    case GL_SAMPLER_1D_SHADOW: {   // sampler1DShadow
        break;
    }
    case GL_SAMPLER_2D_SHADOW: {   // sampler2DShadow
        break;
    }
    case GL_SAMPLER_1D_ARRAY: {   // sampler1DArray
        break;
    }
    case GL_SAMPLER_2D_ARRAY: {   // sampler2DArray
        break;
    }
    case GL_SAMPLER_1D_ARRAY_SHADOW: {       // sampler1DArrayShadow
        break;
    }
    case GL_SAMPLER_2D_ARRAY_SHADOW: {       // sampler2DArrayShadow
        break;
    }
    case GL_SAMPLER_2D_MULTISAMPLE: {   // sampler2DMS
        break;
    }
    case GL_SAMPLER_2D_MULTISAMPLE_ARRAY: {   // sampler2DMSArray
        break;
    }
    case GL_SAMPLER_CUBE_SHADOW: {       // samplerCubeShadow
        break;
    }
    case GL_SAMPLER_BUFFER: {   // samplerBuffer
        break;
    }
    case GL_SAMPLER_2D_RECT: {       // sampler2DRect
        break;
    }
    case GL_SAMPLER_2D_RECT_SHADOW: {   // sampler2DRectShadow
        break;
    }
    case GL_INT_SAMPLER_1D: {   // isampler1D
        break;
    }
    case GL_INT_SAMPLER_2D: {   // isampler2D
        break;
    }
    case GL_INT_SAMPLER_3D: {   // isampler3D
        break;
    }
    case GL_INT_SAMPLER_CUBE: {   // isamplerCube
        break;
    }
    case GL_INT_SAMPLER_1D_ARRAY: {   // isampler1DArray
        break;
    }
    case GL_INT_SAMPLER_2D_ARRAY: {   // isampler2DArray
        break;
    }
    case GL_INT_SAMPLER_2D_MULTISAMPLE: {   // isampler2DMS
        break;
    }
    case GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY: {   // isampler2DMSArray
        break;
    }
    case GL_INT_SAMPLER_BUFFER: {   // isamplerBuffer
        break;
    }
    case GL_INT_SAMPLER_2D_RECT: {       // isampler2DRect
        break;
    }
    case GL_UNSIGNED_INT_SAMPLER_1D: {       // usampler1D
        break;
    }
    case GL_UNSIGNED_INT_SAMPLER_2D: {       // usampler2D
        break;
    }
    case GL_UNSIGNED_INT_SAMPLER_3D: {       // usampler3D
        break;
    }
    case GL_UNSIGNED_INT_SAMPLER_CUBE: {   // usamplerCube
        break;
    }
    case GL_UNSIGNED_INT_SAMPLER_1D_ARRAY: {   // usampler2DArray
        break;
    }
    case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY: {   // usampler2DArray
        break;
    }
    case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE: {       // usampler2DMS
        break;
    }
    case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY: { 	// usampler2DMSArray
        break;
    }
    case GL_UNSIGNED_INT_SAMPLER_BUFFER: {       // usamplerBuffer
        break;
    }
    case GL_UNSIGNED_INT_SAMPLER_2D_RECT: {    // usampler2DRect
        break;
    }*/
    default: {
        LOG_WARN("Used param '" << name << "' for unsupported uniform type: " << type);
        return false;
    }
    }
    return true;
}

bool readGpuMaterialJson(const nlohmann::json& json_, gpuMaterial* mat) {
    /*
    std::string vert_path;
    std::string geom_path;
    std::string frag_path;
    */
    if (!json_.is_object()) {
        LOG_ERR("gpuMaterial root json is not an object");
        assert(false);
        return false;
    }

    nlohmann::json json = jsonPreprocessExtensions(json_);

    auto it_role = json.find("role");
    if (it_role != json.end()) {
        nlohmann::json& jrole = it_role.value();
        if (jrole.is_string()) {
            auto r = gpuStringToRole(jrole.get<std::string>());
            mat->setRoleOverride(r);
        } else {
            LOG_ERR("gpuMaterial: 'role' must be a string");
        }
    }

    auto it_transparent = json.find("transparent");
    if (it_transparent != json.end()) {
        nlohmann::json& jtransparent = it_transparent.value();
        if (jtransparent.is_boolean() && jtransparent.get<bool>()) {
            mat->setTransparent(true);
        }
    }
    {
        auto it = json.find("vertex");
        if (it != json.end()) {
            const nlohmann::json jpath = it.value();
            std::string path = jpath.get<std::string>();
            mat->setVertexExtension(loadResource<gpuShaderSet>(path));
        }
        /*
        it = json.find("geometry");
        if (it != json.end()) {
            const nlohmann::json jpath = it.value();
            std::string path = jpath.get<std::string>();
            pass->addShaderSet(loadResource<gpuShaderSet>("file://" + path));
            LOG_DBG("geometry: " << path);
        }*/

        it = json.find("fragment");
        if (it != json.end()) {
            const nlohmann::json jpath = it.value();
            std::string path = jpath.get<std::string>();
            mat->setFragmentExtension(loadResource<gpuShaderSet>(path));
        }
    }
    {
        // Flags
        auto j = json.find("depth_test") != json.end() ? json.at("depth_test") : nlohmann::json();
        if (j.is_boolean()) {
            mat->setDepthTest(j.get<bool>());
        }
        j = json.find("stencil_test") != json.end() ? json.at("stencil_test") : nlohmann::json();
        if (j.is_boolean()) {
            mat->setStencilTest(j.get<bool>());
        }
        j = json.find("cull_faces") != json.end() ? json.at("cull_faces") : nlohmann::json();
        if (j.is_boolean()) {
            mat->setBackfaceCulling(j.get<bool>());
        }
        j = json.find("depth_write") != json.end() ? json.at("depth_write") : nlohmann::json();
        if (j.is_boolean()) {
            mat->setBackfaceCulling(j.get<bool>());
        }
        j = json.find("blend_mode") != json.end() ? json.at("blend_mode") : nlohmann::json();
        if (j.is_number_integer()) {
            mat->setBlendingMode((GPU_BLEND_MODE)j.get<int>());
        }
    }

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

                std::string path;
                if (jshader.is_object()) {
                    auto it_ref = jshader.find("ref");
                    if (it_ref != jshader.end()) {
                        path = it_ref.value().get<std::string>();
                    }
                } else if(jshader.is_string()) {
                    path = jshader.get<std::string>();
                } else {
                    assert(false);
                    return false;
                }

                pass->addShaderSet(loadResource<gpuShaderSet>("file://" + path));
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

    auto it_params = json.find("params");
    if (it_params != json.end()) {
        const nlohmann::json& jparams = it_params.value();
        if (jparams.is_object()) {
            for (auto it = jparams.begin(); it != jparams.end(); ++it) {
                const std::string& key = it.key();
                const nlohmann::json& jparam = it.value();

                if (jparam.is_number()) {
                    float f = jparam.get<float>();
                    mat->setParam(key, GL_FLOAT, &f);
                } else if (jparam.is_array()) {
                    if (jparam.size() == 2) {
                        gfxm::vec2 vec2(jparam[0].get<float>(), jparam[1].get<float>());
                        mat->setParam(key, GL_FLOAT_VEC2, &vec2);
                    } else if (jparam.size() == 3) {
                        gfxm::vec3 vec3(jparam[0].get<float>(), jparam[1].get<float>(), jparam[2].get<float>());
                        mat->setParam(key, GL_FLOAT_VEC3, &vec3);
                    } else if (jparam.size() == 4) {
                        gfxm::vec4 vec4(jparam[0].get<float>(), jparam[1].get<float>(), jparam[2].get<float>(), jparam[3].get<float>());
                        mat->setParam(key, GL_FLOAT_VEC4, &vec4);
                    } else {
                        LOG_ERR("'" << key << "' parameter array length not supported: " << jparam.size());
                        continue;
                    }
                } else {
                    LOG_ERR("'" << key << "' parameter format not supported");
                    continue;
                }
            }
        } else {
            LOG_ERR("'params' must be an object");
            assert(false);
        }
    }
    // TODO: Restore default params
    /*
    if(mat->passCount() > 0) {
        auto it_params = json.find("params");
        while (it_params != json.end()) {
            nlohmann::json jparams = it_params.value();
            if (!jparams.is_object()) {
                LOG_ERR("'params' must be an object");
                assert(false);
                break;
            }

            for (auto it = jparams.begin(); it != jparams.end(); ++it) {
                const std::string& key = it.key();
                const nlohmann::json& jparam = it.value();

                int idx = -1;
                gpuShaderProgram* prog = 0;
                for (int i = 0; i < mat->passCount(); ++i) {
                    gpuMaterialPass* pass = mat->getPass(i);
                    prog = pass->getShaderProgram();
                    idx = prog->getUniformIndex(key);
                    if (idx >= 0) {
                        break;
                    }
                }
                if (idx < 0) {
                    LOG_WARN("Param '" << key << "' is not used in any material pass");
                    continue;
                }

                const UNIFORM_INFO& inf = prog->getUniformInfo(idx);
                readParamFromJson(mat, key, inf.type, jparam);
            }
            break;
        }
    }*/

    mat->compile();
    return true;
}
bool writeGpuMaterialJson(nlohmann::json& j, gpuMaterial* mat) {
    j = nlohmann::json::object();

    auto role_override = mat->getRoleOverride();
    if (role_override.has_value()) {
        j["role"] = gpuRoleToString(role_override.value());
    }

    auto vert_ext = mat->getVertexExtensionRef();
    if (vert_ext) {
        j["vertex"] = vert_ext.getResourceId();
    }
    auto frag_ext = mat->getFragmentExtensionRef();
    if (frag_ext) {
        j["fragment"] = frag_ext.getResourceId();
    }

    if (mat->getTransparent().has_value()) {
        j["transparent"] = mat->getTransparent().value();
    }
    if (mat->getDepthTest().has_value()) {
        j["depth_test"] = mat->getDepthTest().value();
    }
    if (mat->getDepthWrite().has_value()) {
        j["depth_write"] = mat->getDepthWrite().value();
    }
    if (mat->getStencilTest().has_value()) {
        j["stencil_test"] = mat->getStencilTest().value();
    }
    if (mat->getBackfaceCulling().has_value()) {
        j["cull_faces"] = mat->getBackfaceCulling().value();
    }
    if (mat->getBlendingMode().has_value()) {
        j["blend_mode"] = int(mat->getBlendingMode().value());
    }

    auto& jpasses     = j["passes"];
    auto& jsamplers   = j["samplers"];

    for (int i = 0; i < mat->passCount(); ++i) {
        auto pass = mat->getPass(i);
        nlohmann::json jpass;
        
        // TODO: Update serialization
        //*(char*)0 = 1;
        /*
        auto& hshader = pass->getShaderProgramHandle();
        type_get<HSHARED<gpuShaderProgram>>().serialize_json(jpass["shader"], &hshader);
        */
        jpass["depth_test"] = pass->depth_test != 0 ? true : false;
        jpass["stencil_test"] = pass->stencil_test != 0 ? true : false;
        jpass["cull_faces"] = pass->cull_faces != 0 ? true : false;
        jpass["depth_write"] = pass->depth_write != 0 ? true : false;
        jpass["blend_mode"] = (int)pass->blend_mode;

        jpasses[pass->getPath()] = jpass;
    }

    for (int i = 0; i < mat->samplerCount(); ++i) {
        auto& hsampler = mat->getSampler(i);
        std::string sampler_name = mat->getSamplerName(i);
        nlohmann::json jh;
        type_get<HSHARED<gpuTexture2d>>().serialize_json(jh, &hsampler);
        jsamplers[sampler_name] = jh;
    }

    return true;
}