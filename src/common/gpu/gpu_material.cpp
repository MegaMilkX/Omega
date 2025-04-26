#include "gpu/gpu_material.hpp"

#include "gpu/gpu_pipeline.hpp"

#include "gpu/gpu.hpp"

void gpuMaterial::compile() {
    auto pipeline = gpuGetPipeline();

    for (int i = 0; i < passes.size(); ++i) {
        auto mat_pass = passes[i].get();
        auto pip_pass = pipeline->findPass(mat_pass->getPath().c_str());
        if (!pip_pass) {
            LOG_ERR("Pass '" << mat_pass->getPath() << "' required by a material does not exist");
            continue;
        }

        mat_pass->pipeline_idx = pip_pass->getId();

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