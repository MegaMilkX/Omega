#include "gpu/gpu_material.hpp"

#include "gpu/gpu_pipeline.hpp"

#include "gpu/gpu.hpp"

void gpuMaterial::compile() {
    auto pipeline = gpuGetPipeline();

    technique_pipeline_ids.resize(techniques_by_name.size());
    techniques_by_pipeline_id.resize(pipeline->techniqueCount());
    memset(techniques_by_pipeline_id.data(), 0, techniques_by_pipeline_id.size() * sizeof(techniques_by_pipeline_id[0]));

    int tech_id = 0;
    for (auto& it : techniques_by_name) {
        const std::string& tech_name = it.first;
        auto& t = it.second;
        t->material_local_tech_id = tech_id;

        gpuPipelineTechnique* pipe_tech = pipeline->findTechnique(tech_name.c_str());
        if (!pipe_tech) {
            LOG_ERR("Technique " << it.first << " required by a material does not exist");
            technique_pipeline_ids[tech_id] = -1;
            continue;
        }
        technique_pipeline_ids[tech_id] = pipe_tech->getId();
        techniques_by_pipeline_id[pipe_tech->getId()] = t.get();
        ++tech_id;

        for (int ii = 0; ii < t->passes.size() && pipe_tech->passCount(); ++ii) {
            auto& p = t->passes[ii];
            auto pipe_pass = pipe_tech->getPass(ii);
            //gpuFrameBuffer* frame_buffer = pipe_pass->getFrameBuffer();

            memset(p->gl_draw_buffers, 0, sizeof(p->gl_draw_buffers));
            GLuint program = p->getShader()->getId();
            glUseProgram(program);

            for (int k = 0; k < p->getShader()->getSamplerCount(); ++k) {
                const std::string& name = p->getShader()->getSamplerName(k);
                auto it = sampler_names.find(name);
                if (it == sampler_names.end()) {
                    RHSHARED<gpuTexture2d> htex = getDefaultTexture(name.c_str());
                    if (htex.isValid()) {
                        addSampler(name.c_str(), htex);
                    }
                }
            }

            // Sampler slots
            p->sampler_set.clear();
            for (auto& kv : sampler_names) {
                const std::string& sampler_name = kv.first;
                int material_texture_idx = kv.second;

                RHSHARED<gpuTexture2d> htex = samplers[material_texture_idx];
                if (!htex.isValid()) {
                    htex = getDefaultTexture(sampler_name.c_str());
                }
                GLuint texture_id = htex->getId();

                int slot = p->getShader()->getDefaultSamplerSlot(sampler_name.c_str());
                if (slot < 0) {
                    continue;
                }

                ShaderSamplerSet::Sampler sampler;
                sampler.source = SHADER_SAMPLER_SOURCE_GPU;
                sampler.type = SHADER_SAMPLER_TEXTURE2D;
                sampler.slot = slot;
                sampler.texture_id = texture_id;
                p->sampler_set.add(sampler);
            }
            // Texture buffer samplers
            for (auto& kv : buffer_sampler_names) {
                auto& sampler_name = kv.first;
                if (!buffer_samplers[kv.second]) {
                    assert(false);
                    continue;
                }
                GLuint texture_id = buffer_samplers[kv.second]->getId();
                int slot = p->getShader()->getDefaultSamplerSlot(sampler_name.c_str());
                if (slot < 0) {
                    continue;
                }
                
                ShaderSamplerSet::Sampler sampler;
                sampler.source = SHADER_SAMPLER_SOURCE_GPU;
                sampler.type = SHADER_SAMPLER_TEXTURE_BUFFER;
                sampler.slot = slot;
                sampler.texture_id = texture_id;
                p->sampler_set.add(sampler);
            }
            // Frame image samplers
            for (auto& it : pass_output_samplers) {
                int slot = p->getShader()->getDefaultSamplerSlot(it.to_string().c_str());
                string_id frame_image_id = it;
                if (slot < 0) {
                    continue;
                }

                const gpuPass::ChannelDesc* pass_ch_desc = pipe_pass->getChannelDesc(it.to_string());

                ShaderSamplerSet::Sampler sampler;
                sampler.source = SHADER_SAMPLER_SOURCE_CHANNEL_IDX;
                sampler.type = SHADER_SAMPLER_TEXTURE2D;
                sampler.slot = slot;
                sampler.channel_idx = ShaderSamplerSet::ChannelBufferIdx{ pass_ch_desc->render_target_channel_idx, pass_ch_desc->lwt_buffer_idx };
                p->sampler_set.add(sampler);
            }

            /*
            // Sampler slots
            p->texture_bindings.clear();
            for (auto& kv : sampler_names) {
                auto& sampler_name = kv.first;
                GLuint texture_id = 0;
                RHSHARED<gpuTexture2d> htex = samplers[kv.second];
                if (!htex.isValid()) {
                    htex = getDefaultTexture(sampler_name.c_str());
                }
                texture_id = htex->getId();

                int slot = p->getShader()->getDefaultSamplerSlot(sampler_name.c_str());
                if (slot < 0) {
                    continue;
                }
                //LOG_WARN(sampler_name << " slot: " << slot);

                p->texture_bindings.push_back(gpuMaterialPass::TextureBinding{
                    texture_id, slot
                });
            }*//*
            for (auto& kv : buffer_sampler_names) {
                auto& sampler_name = kv.first;
                GLuint texture_id = 0;
                if (!buffer_samplers[kv.second]) {
                    assert(false);
                    continue;
                }
                texture_id = buffer_samplers[kv.second]->getId();
                int slot = p->getShader()->getDefaultSamplerSlot(sampler_name.c_str());
                if (slot < 0) {
                    continue;
                }
                //LOG_WARN(sampler_name << " slot: " << slot);
                p->texture_buffer_bindings.push_back(gpuMaterialPass::TextureBinding{
                    texture_id, slot
                });
            }*//*
            for (auto& it : pass_output_samplers) {
                int slot = p->getShader()->getDefaultSamplerSlot(it.to_string().c_str());
                if (slot < 0) {
                    continue;
                }
                //LOG_WARN(it.to_string() << "slot: " << slot);
                p->pass_output_bindings.push_back(gpuMaterialPass::PassOutputBinding{
                    it, slot
                });
            }*/

            // Samplers
            /*
            GLint count = 0;
            glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &count);
            for (int i = 0; i < count; ++i) {
                const GLsizei bufSize = 32;
                GLchar name[bufSize];
                GLsizei name_len;
                GLint size;
                GLenum type;
                glGetActiveUniform(program, (GLuint)i, bufSize, &name_len, &size, &type, name);
                std::string sName(name, name + name_len);
                auto it = sampler_names.find(sName);
                if (type == GL_SAMPLER_2D_ARB && it != sampler_names.end()) {
                    auto tex_id = samplers[it->second];
                    glUniform1i(glGetUniformLocation(program, sName.c_str()), it->second);
                }
            }*/

            // Attributes
            /*
            glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &count);
            for (int i = 0; i < count; ++i) {
                const GLsizei bufSize = 32;
                GLchar name[bufSize];
                GLsizei name_len;
                GLint size;
                GLenum type;
                glGetActiveAttrib(program, (GLuint)i, bufSize, &name_len, &size, &type, name);
                std::string attrib_name(name, name + name_len);
                GLint attr_loc = glGetAttribLocation(program, attrib_name.c_str());

                auto desc = VFMT::getAttribDescWithInputName(attrib_name.c_str());
                if (!desc) {
                    continue;
                }
                p->attrib_table[desc->global_id] = attr_loc;
            }*/

            // Outputs
            {
                //glGetProgramInterfaceiv(program, GL_PROGRAM_OUTPUT, GL_ACTIVE_RESOURCES, &count);
                // NOTE: Must iterate channels in the same order
                // as when the framebuffer for this pass was created
                int fb_attachment_index = 0;
                memset(p->gl_draw_buffers, sizeof(p->gl_draw_buffers), 0);
                for (int i = 0; i < pipe_pass->channelCount(); ++i) {
                    const gpuPass::ChannelDesc* ch_desc = pipe_pass->getChannelDesc(i);
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

                    p->gl_draw_buffers[loc] = GL_COLOR_ATTACHMENT0 + fb_attachment_index;
                    ++fb_attachment_index;
                }
            }

            // Uniform buffers
            {/*
                for (int i = 0; i < pipeline->uniformBufferCount(); ++i) {
                    auto ub = pipeline->getUniformBuffer(i);

                    GLuint block_index = glGetUniformBlockIndex(program, ub->getName());
                    if (block_index == GL_INVALID_INDEX) {
                        continue;
                    }
                    int uniform_count = ub->uniformCount();
                    std::vector<const char*> names;
                    std::vector<GLuint> indices;
                    std::vector<GLint> offsets;
                    names.resize(uniform_count);
                    indices.resize(uniform_count);
                    offsets.resize(uniform_count);
                    for (int j = 0; j < uniform_count; ++j) {
                        const char* name = ub->getUniformName(j);
                        names[j] = name;
                    }
                    glGetUniformIndices(program, uniform_count, names.data(), indices.data());
                    glGetActiveUniformsiv(program, uniform_count, indices.data(), GL_UNIFORM_OFFSET, offsets.data());

                    for (int j = 0; j < uniform_count; ++j) {
                        if (indices[j] == GL_INVALID_INDEX) {
                            LOG_ERR("Uniform buffer '" << ub->getName() << "' member '" << names[j] << "' not found");
                            assert(false);
                            // TODO: Fail material compilation
                        }
                    }
                    for (int j = 0; j < uniform_count; ++j) {
                        if (offsets[j] != ub->getUniformByteOffset(j)) {
                            LOG_ERR("Uniform buffer '" << ub->getName() << "' member '" << names[j] << "' offset mismatch: expected " << ub->getUniformByteOffset(j) << ", got " << offsets[j]);
                            assert(false);
                            // TODO: Fail material compilation
                        }
                    }

                    glUniformBlockBinding(program, block_index, i);
                }*/
            }

            glUseProgram(0);
        }
    }
}


void gpuMaterial::serializeJson(nlohmann::json& j) {
    // TODO;
    //static_assert(false);
}
void gpuMaterial::deserializeJson(nlohmann::json& j) {
    //static_assert(false);
}