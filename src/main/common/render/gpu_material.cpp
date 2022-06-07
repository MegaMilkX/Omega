#include "common/render/gpu_material.hpp"

#include "common/render/gpu_pipeline.hpp"


void gpuMaterial::compile() {
    technique_pipeline_ids.resize(techniques_by_name.size());
    techniques_by_pipeline_id.resize(pipeline->techniqueCount());
    memset(techniques_by_pipeline_id.data(), 0, techniques_by_pipeline_id.size() * sizeof(techniques_by_pipeline_id[0]));

    int tech_id = 0;
    for (auto& it : techniques_by_name) {
        const std::string& tech_name = it.first;
        auto& t = it.second;
        t->material_local_tech_id = tech_id;

        gpuPipelineTechnique* pipe_tech = pipeline->findTechnique(tech_name.c_str());
        assert(pipe_tech);
        technique_pipeline_ids[tech_id] = pipe_tech->getId();
        techniques_by_pipeline_id[pipe_tech->getId()] = t.get();
        ++tech_id;

        for (int ii = 0; ii < t->passes.size() && pipe_tech->passCount(); ++ii) {
            auto& p = t->passes[ii];
            auto pipe_pass = pipe_tech->getPass(ii);
            gpuFrameBuffer* frame_buffer = pipe_pass->getFrameBuffer();

            memset(p->gl_draw_buffers, 0, sizeof(p->gl_draw_buffers));
            GLuint program = p->getShader()->getId();
            glUseProgram(program);

            // Uniforms
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
            }

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
                for (int i = 0; i < frame_buffer->colorTargetCount(); ++i) {
                    const char* target_name = frame_buffer->getColorTargetName(i);
                    std::string out_name = MKSTR("out" << target_name);
                    GLint loc = glGetFragDataLocation(program, out_name.c_str());
                    if (loc == -1) {
                        continue;
                    }
                    if (loc >= GPU_FRAME_BUFFER_MAX_DRAW_COLOR_BUFFERS) {
                        LOG_ERR("Fragment shader output location exceeds limit");
                        assert(false);
                    }

                    p->gl_draw_buffers[loc] = GL_COLOR_ATTACHMENT0 + i;
                }
            }

            // Uniform buffers
            {
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
                            // TODO: Fail material compilation
                        }
                    }
                    for (int j = 0; j < uniform_count; ++j) {
                        if (offsets[j] != ub->getUniformByteOffset(j)) {
                            LOG_ERR("Uniform buffer '" << ub->getName() << "' member '" << names[j] << "' offset mismatch: expected " << ub->getUniformByteOffset(j) << ", got " << offsets[j]);
                            // TODO: Fail material compilation
                        }
                    }

                    glUniformBlockBinding(program, block_index, i);
                }
            }

            glUseProgram(0);
        }
    }
}