#pragma once

#include "gpu_mesh.hpp"
#include "gpu_mesh_desc.hpp"
#include "gpu_instancing_desc.hpp"
#include "gpu_material.hpp"
#include "render_id.hpp"

enum GPU_TYPE {
    GPU_FLOAT,
    GPU_VEC2,
    GPU_VEC3,
    GPU_VEC4,
    GPU_MAT3,
    GPU_MAT4
};

class gpuRenderable {
public:
    struct UNIFORM {
        int loc;
        GLenum type;
        int program_index; // Index of the uniform local to its program
        gpuShaderProgram* prog = 0;
        union {
            float float_;
            gfxm::vec2 vec2;
            gfxm::vec3 vec3;
            gfxm::vec4 vec4;
            gfxm::mat3 mat3;
            gfxm::mat4 mat4;
            unsigned char data[36];
        };
    };
    struct UNIFORM_PASS_GROUP {
        int begin;
        int end;
    };
    enum PARAM_TYPE {
        PARAM_UNIFORM,
        PARAM_BLOCK_FIELD
    };
    struct PARAMETER {
        PARAM_TYPE type;
        gpuUniformBuffer* buffer = 0;
        int ub_offset;
        std::vector<int> uniform_data_indices;
    };

    struct SamplerOverride {
        short material_local_technique_id;
        short pass_id;
        int slot;
        GLuint texture_id;
    };

    std::vector<gpuUniformBuffer*> uniform_buffers;
    std::vector<gpuUniformBuffer*> owned_buffers;
    std::map<std::string, HSHARED<gpuTexture2d>> sampler_overrides;

    // Compiled data
    std::shared_ptr<gpuMeshMaterialBinding> desc_binding;
    std::vector<bool> pass_states;
    std::vector<SamplerOverride> compiled_sampler_overrides;

private:
    gpuMaterial* material = 0;
    const gpuMeshDesc* mesh_desc = 0;
    const gpuInstancingDesc* instancing_desc = 0;

    std::vector<UNIFORM> uniform_data;
    std::vector<UNIFORM_PASS_GROUP> uniform_pass_groups;

    std::vector<PARAMETER> params;
    std::map<std::string, int> param_indices;

    gpuUniformBuffer* getOrCreateUniformBuffer(const char* name);

public:
    std::string dbg_name;

    gpuRenderable() {}
    gpuRenderable(gpuMaterial* mat, const gpuMeshDesc* mesh, const gpuInstancingDesc* instancing = 0, const char* dbg_name = "noname")
    : dbg_name(dbg_name) {
        material = mat;
        mesh_desc = mesh;
        instancing_desc = instancing;
        compile();
    }
    virtual ~gpuRenderable();

    void enableMaterialTechnique(const char* path, bool value);

    int getParameterIndex(const char* name);
    void setParam(int index, GPU_TYPE type, const void* pvalue);
    void setParam(int index, GLenum type, const void* pvalue);
    void setFloat(int index, float value);
    void setVec2(int index, const gfxm::vec2& value);
    void setVec3(int index, const gfxm::vec3& value);
    void setVec4(int index, const gfxm::vec4& value);
    void setQuat(int index, const gfxm::quat& value);
    void setMat3(int index, const gfxm::mat3& value);
    void setMat4(int index, const gfxm::mat4& value);
    void setParam(const char* name, GPU_TYPE type, const void* pvalue);
    void setFloat(const char* name, float value);
    void setVec2(const char* name, const gfxm::vec2& value);
    void setVec3(const char* name, const gfxm::vec3& value);
    void setVec4(const char* name, const gfxm::vec4& value);
    void setQuat(const char* name, const gfxm::quat& value);
    void setMat3(const char* name, const gfxm::mat3& value);
    void setMat4(const char* name, const gfxm::mat4& value);

    bool isInstanced() const {
        return mesh_desc && instancing_desc;
    }
    const gpuMaterial* getMaterial() const {
        return material;
    }
    const gpuMeshDesc* getMeshDesc() const {
        return mesh_desc;
    }
    const gpuInstancingDesc* getInstancingDesc() const {
        return instancing_desc;
    }

    gpuRenderable& setMaterial(gpuMaterial* material) {
        this->material = material;
        return *this;
    }
    gpuRenderable& setMeshDesc(const gpuMeshDesc* mesh) {
        this->mesh_desc = mesh;
        return *this;
    }
    gpuRenderable& setInstancingDesc(const gpuInstancingDesc* instancing) {
        this->instancing_desc = instancing;
        return *this;
    }
    gpuRenderable& attachUniformBuffer(gpuUniformBuffer* buf);

    void addSamplerOverride(const char* name, HSHARED<gpuTexture2d> tex) {
        sampler_overrides[name] = tex;
    }

    void compile() {
        //
        if (!instancing_desc) {
            desc_binding.reset(new gpuMeshMaterialBinding);
            gpuMakeMeshMaterialBinding(desc_binding.get(), material, mesh_desc, 0);
        } else {
            desc_binding.reset(new gpuMeshMaterialBinding);
            gpuMakeMeshMaterialBinding(desc_binding.get(), material, mesh_desc, instancing_desc);
        }

        //
        pass_states.resize(desc_binding->binding_array.size());
        std::fill(pass_states.begin(), pass_states.end(), true);

        params.clear();
        param_indices.clear();
        // Uniform blocks
        for (int i = 0; i < material->passCount(); ++i) {
            auto pass = material->getPass(i);
            auto prog = pass->getShaderProgram();

            for (int j = 0; j < prog->uniformBlockCount(); ++j) {
                auto ubdesc = prog->getUniformBlockDesc(j);

                gpuUniformBuffer* buffer = getOrCreateUniformBuffer(ubdesc->getName());
                
                for (int k = 0; k < ubdesc->uniformCount(); ++k) {
                    const char* name = ubdesc->getUniformName(k);
                    int offset = ubdesc->getUniformByteOffset(k);
                    PARAMETER* pparam = 0;
                    {
                        auto it = param_indices.find(name);
                        if (it == param_indices.end()) {
                            param_indices.insert(std::make_pair(std::string(name), params.size()));
                            params.push_back(
                                PARAMETER{
                                    .type = PARAM_BLOCK_FIELD,
                                    .buffer = buffer,
                                    .ub_offset = offset
                                }
                            );
                            pparam = &params.back();
                        } else {
                            pparam = &params[it->second];
                        }
                    }
                }
            }
        }

        // Uniforms

        uniform_data.clear();
        uniform_pass_groups.clear();
        int begin = 0;
        for (int i = 0; i < material->passCount(); ++i) {
            auto pass = material->getPass(i);
            auto prog = pass->getShaderProgram();

            int uniform_count = prog->uniformCount();
            for (int j = 0; j < uniform_count; ++j) {
                UNIFORM_INFO inf = prog->getUniformInfo(j);

                PARAMETER* pparam = 0;
                {
                    auto it = param_indices.find(inf.name);
                    if (it == param_indices.end()) {
                        param_indices.insert(std::make_pair(inf.name, params.size()));
                        params.push_back(
                            PARAMETER{
                                .type = PARAM_UNIFORM
                            }
                        );
                        pparam = &params.back();
                    } else {
                        pparam = &params[it->second];
                    }
                }

                if (!pparam->uniform_data_indices.empty()) {
                    auto master_type = uniform_data[pparam->uniform_data_indices[0]].type;
                    if (inf.type != master_type) {
                        LOG_ERR("Uniform '" << inf.name << "' must have the same type across all passes");
                        assert(false);
                        continue;
                    }
                }
                pparam->uniform_data_indices.push_back(uniform_data.size());

                uniform_data.push_back(
                    UNIFORM{
                        .loc = inf.location,
                        .type = inf.type,
                        .program_index = j,
                        .prog = prog
                    }
                );
                memset(uniform_data.back().data, 0, sizeof(uniform_data.back().data));
            }

            uniform_pass_groups.push_back(
                UNIFORM_PASS_GROUP{
                    .begin = begin,
                    .end = (int)uniform_data.size()
                }
            );
            begin = uniform_data.size();
        }

        for (auto kv : param_indices) {
            const std::string& name = kv.first;
            gpuMaterial::PARAMETER* param = material->getParam(name);
            if (!param) {
                continue;
            }
            setParam(kv.second, param->type, param->data);
        }

        /*
        compiled_sampler_overrides.clear();
        assert(sampler_overrides.size() <= 32);
        for (auto& kv : sampler_overrides) {
            if (!kv.second.isValid()) {
                LOG_ERR("Renderable sampler override " << kv.first << " texture handle is invalid");
                continue;
            }

            for (int i = 0; i < material->techniqueCount(); ++i) {
                auto tech = material->getTechniqueByLocalId(i);
                for (int j = 0; j < tech->passCount(); ++j) {
                    auto pass = tech->getPass(j);
                    const std::string sampler_name = kv.first;
                    auto prog = pass->getShader();
                    if (!prog) {
                        continue;
                    }
                    int slot = prog->getDefaultSamplerSlot(sampler_name.c_str());
                    if (slot == -1) {
                        continue;
                    }
                    SamplerOverride override{};
                    override.material_local_technique_id = i;
                    override.pass_id = j;
                    override.slot = slot;
                    override.texture_id = kv.second->getId();
                    compiled_sampler_overrides.push_back(override);
                }
            }
        }*/
    }

    void bindUniformBuffers() {
        for (int i = 0; i < uniform_buffers.size(); ++i) {
            auto& ub = uniform_buffers[i];
            GLint gl_id = ub->gpu_buf.getId();
            glBindBufferBase(GL_UNIFORM_BUFFER, ub->getDesc()->id, gl_id);
        }
    }
    void uploadUniforms(int material_pass_idx) {
        assert(
            material_pass_idx >= 0
            && material_pass_idx < uniform_pass_groups.size()
        );

        const UNIFORM_PASS_GROUP& group = uniform_pass_groups[material_pass_idx];

        for (int i = group.begin; i < group.end; ++i) {
            auto& u = uniform_data[i];
            if (!u.prog->getUniformInfo(u.program_index).auto_upload) {
                continue;
            }

            switch (u.type) {
            case GL_FLOAT: {   // float
                glUniform1f(u.loc, u.float_);
                break;
            }
            case GL_FLOAT_VEC2: {   // vec2
                glUniform2f(u.loc, u.vec2.x, u.vec2.y);
                break;
            }
            case GL_FLOAT_VEC3: {   // vec3
                glUniform3f(u.loc, u.vec3.x, u.vec3.y, u.vec3.z);
                break;
            }
            case GL_FLOAT_VEC4: {   // vec4
                glUniform4f(u.loc, u.vec4.x, u.vec4.y, u.vec4.z, u.vec4.w);
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
                glUniform1i(u.loc, *(GLint*)u.data);
                break;
            }
            case GL_INT_VEC2: {   // ivec2
                glUniform2i(u.loc, *(GLint*)u.data, *((GLint*)u.data + 1));
                break;
            }
            case GL_INT_VEC3: {   // ivec3
                glUniform3i(u.loc, *(GLint*)u.data, *((GLint*)u.data + 1), *((GLint*)u.data + 2));
                break;
            }
            case GL_INT_VEC4: {   // ivec4
                glUniform4i(u.loc, *(GLint*)u.data, *((GLint*)u.data + 1), *((GLint*)u.data + 2), *((GLint*)u.data + 3));
                break;
            }
            case GL_UNSIGNED_INT: {   // unsigned int
                glUniform1ui(u.loc, *(GLuint*)u.data);
                break;
            }
            case GL_UNSIGNED_INT_VEC2: {   // uvec2
                glUniform2ui(u.loc, *(GLuint*)u.data, *((GLuint*)u.data + 1));
                break;
            }
            case GL_UNSIGNED_INT_VEC3: {   // uvec3
                glUniform3ui(u.loc, *(GLuint*)u.data, *((GLuint*)u.data + 1), *((GLuint*)u.data + 2));
                break;
            }
            case GL_UNSIGNED_INT_VEC4: {   // uvec4
                glUniform4ui(u.loc, *(GLuint*)u.data, *((GLuint*)u.data + 1), *((GLuint*)u.data + 2), *((GLuint*)u.data + 3));
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
                glUniform1fv(u.loc, 4, (GLfloat*)u.data);
                break;
            }
            case GL_FLOAT_MAT3: {   // mat3
                glUniform1fv(u.loc, 9, (GLfloat*)u.data);
                break;
            }
            case GL_FLOAT_MAT4: {   // mat4
                glUniform1fv(u.loc, 16, (GLfloat*)u.data);
                break;
            }
            case GL_FLOAT_MAT2x3: {   // mat2x3
                glUniform1fv(u.loc, 6, (GLfloat*)u.data);
                break;
            }
            case GL_FLOAT_MAT2x4: {   // mat2x4
                glUniform1fv(u.loc, 8, (GLfloat*)u.data);
                break;
            }
            case GL_FLOAT_MAT3x2: {   // mat3x2
                glUniform1fv(u.loc, 6, (GLfloat*)u.data);
                break;
            }
            case GL_FLOAT_MAT3x4: {   // mat3x4
                glUniform1fv(u.loc, 12, (GLfloat*)u.data);
                break;
            }
            case GL_FLOAT_MAT4x2: {   // mat4x2
                glUniform1fv(u.loc, 8, (GLfloat*)u.data);
                break;
            }
            case GL_FLOAT_MAT4x3: {   // mat4x3
                glUniform1fv(u.loc, 12, (GLfloat*)u.data);
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
                LOG_WARN("Unsupported uniform type used: " << u.type);
                assert(false);
            }
            }
        }
    }

    void bindSamplerOverrides(int material_local_tech_id, int pass_id) {
        for (int i = 0; i < compiled_sampler_overrides.size(); ++i) {
            auto& ovr = compiled_sampler_overrides[i];
            if (ovr.material_local_technique_id != material_local_tech_id || ovr.pass_id != pass_id) {
                continue;
            }
            glActiveTexture(GL_TEXTURE0 + ovr.slot);
            glBindTexture(GL_TEXTURE_2D, ovr.texture_id);
        }
    }
};

class gpuGeometryRenderable : public gpuRenderable {
    gpuUniformBuffer* ubuf_model = 0;
    int loc_transform;
public:
    gpuGeometryRenderable(gpuMaterial* mat, const gpuMeshDesc* mesh, const gpuInstancingDesc* instancing = 0, const char* dbg_name = "noname");
    ~gpuGeometryRenderable();

    void setTransform(const gfxm::mat4& t) {
        ubuf_model->setMat4(loc_transform, t);
    }
};