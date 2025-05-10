#include "gpu/gpu_material.hpp"

#include "gpu/gpu_pipeline.hpp"

#include "gpu/gpu.hpp"

int glTypeToSize(GLenum type) {
    switch (type) {
    case GL_FLOAT:
    case GL_INT:
    case GL_UNSIGNED_INT:
        return 4;
    case GL_FLOAT_VEC2:
    case GL_INT_VEC2:
    case GL_UNSIGNED_INT_VEC2:
        return 8;
    case GL_FLOAT_VEC3:
    case GL_INT_VEC3:
    case GL_UNSIGNED_INT_VEC3:
        return 12;
    case GL_FLOAT_VEC4:
    case GL_INT_VEC4:
    case GL_UNSIGNED_INT_VEC4:
        return 16;
    case GL_DOUBLE:
        return 8;
    case GL_DOUBLE_VEC2:
        return 16;
    case GL_DOUBLE_VEC3:
        return 24;
    case GL_DOUBLE_VEC4:
        return 32;
     /*
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
    case GL_FLOAT_MAT2:
        return 16;
    case GL_FLOAT_MAT3:
        return 36;
    case GL_FLOAT_MAT4:
        return 64;
    case GL_FLOAT_MAT2x3:
        return 24;
    case GL_FLOAT_MAT2x4:
        return 32;
    case GL_FLOAT_MAT3x2:
        return 24;
    case GL_FLOAT_MAT3x4:
        return 48;
    case GL_FLOAT_MAT4x2:
        return 32;
    case GL_FLOAT_MAT4x3:
        return 48;
        /*
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
        LOG_ERR("glTypeToSize: unknown type: " << type);
        assert(false);
    }
    }
    return 0;
}

void gpuMaterial::setParam(const std::string& name, GLenum type, const void* data) {
    PARAMETER param = {
        .type = type,
    };
    memcpy(param.data, data, glTypeToSize(type));
    params[name] = param;
}
void gpuMaterial::setParamFloat(const std::string& name, float value) {
    setParam(name, GL_FLOAT, &value);
}
void gpuMaterial::setParamVec2(const std::string& name, const gfxm::vec2& v) {
    setParam(name, GL_FLOAT_VEC2, &v);
}
void gpuMaterial::setParamVec3(const std::string& name, const gfxm::vec3& v) {
    setParam(name, GL_FLOAT_VEC3, &v);
}
void gpuMaterial::setParamVec4(const std::string& name, const gfxm::vec4& v) {
    setParam(name, GL_FLOAT_VEC4, &v);
}
void gpuMaterial::setParamInt(const std::string& name, int value) {
    setParam(name, GL_INT, &value);
}
void gpuMaterial::setParamVec2i(const std::string& name, const gfxm::ivec2& v) {
    setParam(name, GL_INT_VEC2, &v);
}
void gpuMaterial::setParamVec3i(const std::string& name, const gfxm::ivec3& v) {
    setParam(name, GL_INT_VEC3, &v);

}
void gpuMaterial::setParamVec4i(const std::string& name, const gfxm::ivec4& v) {
    setParam(name, GL_INT_VEC4, &v);
}
void gpuMaterial::setParamMat2(const std::string& name, float* pvalue) {
    setParam(name, GL_FLOAT_MAT2, pvalue);
}
void gpuMaterial::setParamMat3(const std::string& name, float* pvalue) {
    setParam(name, GL_FLOAT_MAT3, pvalue);
}
void gpuMaterial::setParamMat4(const std::string& name, float* pvalue) {
    setParam(name, GL_FLOAT_MAT4, pvalue);
}
void gpuMaterial::setParamMat2x3(const std::string& name, float* pvalue) {
    setParam(name, GL_FLOAT_MAT2x3, pvalue);
}
void gpuMaterial::setParamMat2x4(const std::string& name, float* pvalue) {
    setParam(name, GL_FLOAT_MAT2x4, pvalue);
}
void gpuMaterial::setParamMat3x2(const std::string& name, float* pvalue) {
    setParam(name, GL_FLOAT_MAT3x2, pvalue);
}
void gpuMaterial::setParamMat3x4(const std::string& name, float* pvalue) {
    setParam(name, GL_FLOAT_MAT3x4, pvalue);
}
void gpuMaterial::setParamMat4x2(const std::string& name, float* pvalue) {
    setParam(name, GL_FLOAT_MAT4x2, pvalue);
}
void gpuMaterial::setParamMat4x3(const std::string& name, float* pvalue) {
    setParam(name, GL_FLOAT_MAT4x3, pvalue);
}

gpuMaterial::PARAMETER* gpuMaterial::getParam(const std::string& name) {
    auto it = params.find(name);
    if (it == params.end()) {
        return nullptr;
    }
    return &it->second;
}

void gpuMaterial::compile() {
    auto pipeline = gpuGetPipeline();

    pipe_pass_to_mat_pass.resize(pipeline->passCount());
    std::fill(pipe_pass_to_mat_pass.begin(), pipe_pass_to_mat_pass.end(), -1);

    for (int i = 0; i < passes.size(); ++i) {
        auto mat_pass = passes[i].get();
        auto pip_pass = pipeline->findPass(mat_pass->getPath().c_str());
        if (!pip_pass) {
            LOG_ERR("Pass '" << mat_pass->getPath() << "' required by a material does not exist");
            continue;
        }

        mat_pass->pipeline_idx = pip_pass->getId();
        pipe_pass_to_mat_pass[mat_pass->pipeline_idx] = i;

        memset(mat_pass->gl_draw_buffers, 0, sizeof(mat_pass->gl_draw_buffers));
        const gpuShaderProgram* shader_program = mat_pass->getShader();
        assert(shader_program);
        GLuint program = shader_program->getId();
        glUseProgram(program);

        // Set default textures
        for (int k = 0; k < mat_pass->getShader()->getSamplerCount(); ++k) {
            const std::string& name = mat_pass->getShader()->getSamplerName(k);
            auto it = sampler_names.find(name);
            if (it == sampler_names.end()) {
                RHSHARED<gpuTexture2d> htex = getDefaultTexture(name.c_str());
                if (htex.isValid()) {
                    addSampler(name.c_str(), htex);
                }
            }
        }

        // Sampler slots
        mat_pass->sampler_set.clear();
        for (auto& kv : sampler_names) {
            const std::string& sampler_name = kv.first;
            int material_texture_idx = kv.second;

            RHSHARED<gpuTexture2d> htex = samplers[material_texture_idx];
            if (!htex.isValid()) {
                htex = getDefaultTexture(sampler_name.c_str());
            }
            GLuint texture_id = htex->getId();

            int slot = mat_pass->getShader()->getDefaultSamplerSlot(sampler_name.c_str());
            if (slot < 0) {
                continue;
            }

            ShaderSamplerSet::Sampler sampler;
            sampler.source = SHADER_SAMPLER_SOURCE_GPU;
            sampler.type = SHADER_SAMPLER_TEXTURE2D;
            sampler.slot = slot;
            sampler.texture_id = texture_id;
            mat_pass->sampler_set.add(sampler);
        }

        // Texture buffer samplers
        for (auto& kv : buffer_sampler_names) {
            auto& sampler_name = kv.first;
            if (!buffer_samplers[kv.second]) {
                assert(false);
                continue;
            }
            GLuint texture_id = buffer_samplers[kv.second]->getId();
            int slot = mat_pass->getShader()->getDefaultSamplerSlot(sampler_name.c_str());
            if (slot < 0) {
                continue;
            }

            ShaderSamplerSet::Sampler sampler;
            sampler.source = SHADER_SAMPLER_SOURCE_GPU;
            sampler.type = SHADER_SAMPLER_TEXTURE_BUFFER;
            sampler.slot = slot;
            sampler.texture_id = texture_id;
            mat_pass->sampler_set.add(sampler);
        }
        // Frame image samplers
        for (auto& it : pass_output_samplers) {
            int slot = mat_pass->getShader()->getDefaultSamplerSlot(it.to_string().c_str());
            string_id frame_image_id = it;
            if (slot < 0) {
                continue;
            }

            const gpuPass::ChannelDesc* pass_ch_desc = pip_pass->getChannelDesc(it.to_string());

            ShaderSamplerSet::Sampler sampler;
            sampler.source = SHADER_SAMPLER_SOURCE_CHANNEL_IDX;
            sampler.type = SHADER_SAMPLER_TEXTURE2D;
            sampler.slot = slot;
            sampler.channel_idx = ShaderSamplerSet::ChannelBufferIdx{ pass_ch_desc->render_target_channel_idx, pass_ch_desc->lwt_buffer_idx };
            mat_pass->sampler_set.add(sampler);
        }

        // Outputs
        {
            //glGetProgramInterfaceiv(program, GL_PROGRAM_OUTPUT, GL_ACTIVE_RESOURCES, &count);
            // NOTE: Must iterate channels in the same order
            // as when the framebuffer for this pass was created
            int fb_attachment_index = 0;
            memset(mat_pass->gl_draw_buffers, 0, sizeof(mat_pass->gl_draw_buffers));
            for (int j = 0; j < pip_pass->channelCount(); ++j) {
                const gpuPass::ChannelDesc* ch_desc = pip_pass->getChannelDesc(j);
                if (!ch_desc->writes) {
                    continue;
                }

                const std::string& target_name = ch_desc->target_local_name;
                std::string out_name = MKSTR("out" << target_name);
                GLint loc = glGetFragDataLocation(program, out_name.c_str());
                if (loc == -1) {
                    ++fb_attachment_index;
                    continue;
                }
                if (loc >= GPU_FRAME_BUFFER_MAX_DRAW_COLOR_BUFFERS) {
                    LOG_ERR("Fragment shader output location exceeds limit");
                    assert(false);
                    break;
                }

                mat_pass->gl_draw_buffers[loc] = GL_COLOR_ATTACHMENT0 + fb_attachment_index;
                ++fb_attachment_index;
            }
        }

        glUseProgram(0);
    }
}


void gpuMaterial::serializeJson(nlohmann::json& j) {
    // TODO;
    //static_assert(false);
}
void gpuMaterial::deserializeJson(nlohmann::json& j) {
    //static_assert(false);
}