#include "gpu_renderable.hpp"

#include "gpu.hpp"
#include "gpu_pipeline.hpp"
#include "gpu/param_block/param_block_mgr.hpp"
#include "util/timer.hpp"


GLenum gpuTypeToGLenum(GPU_TYPE type) {
    switch (type) {
    case GPU_FLOAT: return GL_FLOAT;
    case GPU_INT: return GL_INT;
    case GPU_VEC2: return GL_FLOAT_VEC2;
    case GPU_VEC3: return GL_FLOAT_VEC3;
    case GPU_VEC4: return GL_FLOAT_VEC4;
    case GPU_MAT3: return GL_FLOAT_MAT3;
    case GPU_MAT4: return GL_FLOAT_MAT4;
    default:
        assert(false);
        return 0;
    }
}


gpuRenderable::~gpuRenderable() {
    for (int i = 0; i < owned_buffers.size(); ++i) {
        gpuGetPipeline()->destroyUniformBuffer(owned_buffers[i]);
    }
}

gpuUniformBuffer* gpuRenderable::getOrCreateUniformBuffer(const char* name) {
    for (int i = 0; i < uniform_buffers.size(); ++i) {
        if (uniform_buffers[i]->getDesc()->getName() == std::string(name)) {
            return uniform_buffers[i];
        }
    }

    if (gpuGetPipeline()->isUniformBufferAttached(name)) {
        return 0;
    }
    
    gpuUniformBuffer* buf = gpuGetPipeline()->createUniformBuffer(name);
    owned_buffers.push_back(buf);
    attachUniformBuffer(buf);
    return buf;
}

gpuRenderable* gpuRenderable::setRole(GPU_Role role) {
    this->role = role;
    return this;
}
gpuRenderable* gpuRenderable::enableEffect(GPU_Effect effect) {
    this->effect_flags |= (1 << effect);
    return this;
}

void gpuRenderable::enableMaterialTechnique(const char* path, bool value) {
    auto node = gpuGetPipeline()->findNode(path);
    if (!node) {
        assert(false);
        return;
    }
    int pass_count = node->getPassCount();
    gpuPass* passes[32];
    pass_count = node->getPassList(passes, 32);
    for (int i = 0; i < pass_count; ++i) {
        int pass_pipe_idx = passes[i]->getId();
        for (int j = 0; j < compiled_desc->pass_array.size(); ++j) {
            if (compiled_desc->pass_array[j].pass == pass_pipe_idx) {
                pass_states[j] = value;
            }
        }
        /*
        int pass_mat_idx = material->getPassMaterialIdx(pass_pipe_idx);
        if (pass_mat_idx < 0) {
            continue;
        }
        pass_states[pass_mat_idx] = value;*/
    }
}

int gpuRenderable::getParameterIndex(const char* name) {
    auto it = param_indices.find(name);
    if (it == param_indices.end()) {
        return -1;
    }
    return it->second;
}

void gpuRenderable::setParam(int index, GPU_TYPE type, const void* pvalue) {
    setParam(index, gpuTypeToGLenum(type), pvalue);
}
void gpuRenderable::setParam(int index, GLenum type, const void* pvalue) {
    if (index < 0) {
        assert(false);
        return;
    }
    auto& p = params[index];
    if (p.type == PARAM_UNIFORM) {
        for (int i = 0; i < p.uniform_data_indices.size(); ++i) {
            auto& u = uniform_data[p.uniform_data_indices[i]];
            u.prog->getUniformInfo(u.program_index).auto_upload = true;
            memcpy(u.data, pvalue, glTypeToSize(type));
        }
    } else if (p.type == PARAM_BLOCK_FIELD) {
        if(p.buffer) {
            p.buffer->setValue(p.index, pvalue, glTypeToSize(type));
        }
    } else {
        assert(false);
    }
}
void gpuRenderable::setFloat(int index, float value) {
    setParam(index, GL_FLOAT, &value);
}
void gpuRenderable::setVec2(int index, const gfxm::vec2& value) {
    setParam(index, GL_FLOAT_VEC2, &value);
}
void gpuRenderable::setVec3(int index, const gfxm::vec3& value) {
    setParam(index, GL_FLOAT_VEC3, &value);
}
void gpuRenderable::setVec4(int index, const gfxm::vec4& value) {
    setParam(index, GL_FLOAT_VEC4, &value);
}
void gpuRenderable::setQuat(int index, const gfxm::quat& value) {
    setParam(index, GL_FLOAT_VEC4, &value);
}
void gpuRenderable::setMat3(int index, const gfxm::mat3& value) {
    setParam(index, GL_FLOAT_MAT3, &value);
}
void gpuRenderable::setMat4(int index, const gfxm::mat4& value) {
    setParam(index, GL_FLOAT_MAT4, &value);
}

void gpuRenderable::setParam(const char* name, GPU_TYPE type, const void* pvalue) {
    setParam(getParameterIndex(name), type, pvalue);
}
void gpuRenderable::setFloat(const char* name, float value) {
    setFloat(getParameterIndex(name), value);
}
void gpuRenderable::setVec2(const char* name, const gfxm::vec2& value) {
    setVec2(getParameterIndex(name), value);
}
void gpuRenderable::setVec3(const char* name, const gfxm::vec3& value) {
    setVec3(getParameterIndex(name), value);
}
void gpuRenderable::setVec4(const char* name, const gfxm::vec4& value) {
    setVec4(getParameterIndex(name), value);
}
void gpuRenderable::setQuat(const char* name, const gfxm::quat& value) {
    setQuat(getParameterIndex(name), value);
}
void gpuRenderable::setMat3(const char* name, const gfxm::mat3& value) {
    setMat3(getParameterIndex(name), value);
}
void gpuRenderable::setMat4(const char* name, const gfxm::mat4& value) {
    setMat4(getParameterIndex(name), value);
}


gpuRenderable& gpuRenderable::attachUniformBuffer(gpuUniformBuffer* buf) {
    for (int i = 0; i < uniform_buffers.size(); ++i) {
        if (uniform_buffers[i]->getDesc() == buf->getDesc()) {
            for (int j = 0; j < owned_buffers.size(); ++j) {
                if(owned_buffers[j]->getDesc() == buf->getDesc()) {
                    gpuGetPipeline()->destroyUniformBuffer(owned_buffers[j]);
                    owned_buffers.erase(owned_buffers.begin() + j);
                    break;
                }
            }
            uniform_buffers[i] = buf;
            return *this;
        }
    }
    uniform_buffers.push_back(buf);
    return *this;
}

gpuRenderable& gpuRenderable::attachParamBlock(gpuParamBlock* block) {
    auto t = block->getMgr()->getBlockType();
    param_blocks[t] = block;
    return *this;
}

void gpuRenderable::compile() {
    timer timer_;
    timer_.start();
    //
    compiled_desc.reset(new gpuCompiledRenderableDesc);
    gpuCompileRenderablePasses(compiled_desc.get(), this, material, mesh_desc, instancing_desc);

    //
    auto pipeline = gpuGetPipeline();
    for (int i = 0; i < compiled_desc->pass_array.size(); ++i) {
        auto rdr_pass = &compiled_desc->pass_array[i];
        /*
        auto mat_pass = material->getPass(rdr_pass->material_pass);
        if (mat_pass->getPipelineIdx() < 0) {
            continue;
        }*/
        auto pip_pass = pipeline->getPass(rdr_pass->pass);
        auto prog = rdr_pass->prog.get();

        glUseProgram(prog->getId());

        // Set default textures
        for (int j = 0; j < prog->getSamplerCount(); ++j) {
            const std::string& name = prog->getSamplerName(j);
            int idx = material->getSamplerIdx(name.c_str());
            if (idx < 0) {
                int slot = prog->getDefaultSamplerSlot(name.c_str());
                if (slot < 0) {
                    continue;
                }
                RHSHARED<gpuTexture2d> htex = getDefaultTexture(name.c_str());
                if (htex.isValid()) {
                    ShaderSamplerSet::Sampler sampler;
                    sampler.source = SHADER_SAMPLER_SOURCE_GPU;
                    sampler.type = SHADER_SAMPLER_TEXTURE2D;
                    sampler.slot = slot;
                    sampler.texture_id = htex->getId();
                    rdr_pass->sampler_set.add(sampler);
                }
            }
        }

        // Texture2d samplers
        for (int j = 0; j < material->samplerCount(); ++j) {
            std::string sampler_name = material->getSamplerName(j);
            const auto& htex = material->getSampler(j);

            if (!htex.isValid()) {
                LOG_ERR("Renderable: Sampler " << sampler_name << " is present in material but invalid");
                continue;
            }

            int slot = prog->getDefaultSamplerSlot(sampler_name.c_str());
            if (slot < 0) {
                continue;
            }

            GLuint texture_id = htex->getId();

            ShaderSamplerSet::Sampler sampler;
            sampler.source = SHADER_SAMPLER_SOURCE_GPU;
            sampler.type = SHADER_SAMPLER_TEXTURE2D;
            sampler.slot = slot;
            sampler.texture_id = texture_id;
            rdr_pass->sampler_set.add(sampler);
        }

        // Texture buffer samplers
        for (int j = 0; j < material->bufferSamplerCount(); ++j) {
            std::string sampler_name = material->getBufferSamplerName(j);
            const auto& buf = material->getBufferSampler(j);

            int slot = prog->getDefaultSamplerSlot(sampler_name.c_str());
            if (slot < 0) {
                continue;
            }

            ShaderSamplerSet::Sampler sampler;
            sampler.source = SHADER_SAMPLER_SOURCE_GPU;
            sampler.type = SHADER_SAMPLER_TEXTURE_BUFFER;
            sampler.slot = slot;
            sampler.texture_id = buf->getId();
            rdr_pass->sampler_set.add(sampler);
        }

        // Pipeline channel samplers
        for (int j = 0; j < pip_pass->channelCount(); ++j) {
            const gpuPass::ChannelDesc* ch = pip_pass->getChannelDesc(j);
            if (!ch->reads) {
                continue;
            }

            int slot = prog->getDefaultSamplerSlot(ch->source_local_name.c_str());
            if (slot < 0) {
                continue;
            }

            ShaderSamplerSet::Sampler sampler;
            sampler.source = SHADER_SAMPLER_SOURCE_CHANNEL_IDX;
            sampler.type = SHADER_SAMPLER_TEXTURE2D;
            sampler.slot = slot;
            sampler.channel_idx = ShaderSamplerSet::ChannelBufferIdx{ ch->render_target_channel_idx, ch->lwt_buffer_idx };
            rdr_pass->sampler_set.add(sampler);
        }

        // Pipeline pass textures
        for (int j = 0; j < pip_pass->textureCount(); ++j) {
            auto tex_desc = pip_pass->getTextureDesc(j);
            int slot = prog->getDefaultSamplerSlot(tex_desc->sampler_name.c_str());
            if (slot < 0) {
                continue;
            }
            ShaderSamplerSet::Sampler sampler;
            sampler.source = SHADER_SAMPLER_SOURCE_GPU;
            sampler.type = tex_desc->type;
            sampler.slot = slot;
            sampler.texture_id = tex_desc->texture;
            rdr_pass->sampler_set.add(sampler);
        }

        // Outputs
        {
            int fb_attachment_idx = 0;
            memset(rdr_pass->gl_draw_buffers, 0, sizeof(rdr_pass->gl_draw_buffers));
            for (int j = 0; j < pip_pass->channelCount(); ++j) {
                const gpuPass::ChannelDesc* ch_desc = pip_pass->getChannelDesc(j);
                if (!ch_desc->writes) {
                    continue;
                }

                const std::string& tgt_name = ch_desc->target_local_name;
                std::string out_name = MKSTR("out" << tgt_name);
                GLint loc = glGetFragDataLocation(prog->getId(), out_name.c_str());
                if (loc == -1) {
                    ++fb_attachment_idx;
                    continue;
                }
                if (loc >= platformGeti(PLATFORM_MAX_COLOR_OUTPUTS)) {
                    LOG_ERR("Renderable: Fragment shader output location exceeds limit");
                    assert(false);
                    break;
                }

                rdr_pass->gl_draw_buffers[loc] = GL_COLOR_ATTACHMENT0 + fb_attachment_idx;
                ++fb_attachment_idx;
            }
        }

        glUseProgram(0);

        rdr_pass->sampler_set_identity = rdr_pass->sampler_set.resolveIdentity();
    }

    //
    pass_states.resize(compiled_desc->pass_array.size());
    std::fill(pass_states.begin(), pass_states.end(), true);

    params.clear();
    param_indices.clear();
    // Uniform blocks
    /*
    for (int i = 0; i < compiled_desc->pass_array.size(); ++i) {
        //auto pass = material->getPass(i);
        //auto prog = pass->getShaderProgram();
        auto& prog = compiled_desc->pass_array[i].prog;

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
                                .ub_offset = offset,
                                .index = k
                            }
                        );
                        pparam = &params.back();
                    } else {
                        pparam = &params[it->second];
                    }
                }
            }
        }
    }*/

    // Uniforms
    uniform_data.clear();
    uniform_pass_groups.clear();
    int begin = 0;
    for (int i = 0; i < compiled_desc->pass_array.size()/*material->passCount()*/; ++i) {
        //auto pass = material->getPass(i);
        //auto prog = pass->getShaderProgram();
        auto& prog = compiled_desc->pass_array[i].prog;

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
                    .prog = prog.get()
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

    compiled_sampler_overrides.clear();
    assert(sampler_overrides.size() <= 32);
    for (auto& kv : sampler_overrides) {
        if (!kv.second.isValid()) {
            LOG_ERR("Renderable sampler override " << kv.first << " texture handle is invalid");
            continue;
        }

        //LOG("Sampler override: " << kv.first);

        for (int i = 0; i < compiled_desc->pass_array.size()/*material->passCount()*/; ++i) {
            //auto pass = material->getPass(i);
            //auto prog = pass->getShaderProgram();
            auto& prog = compiled_desc->pass_array[i].prog;

            const std::string sampler_name = kv.first;
            if (!prog) {
                continue;
            }
            int slot = prog->getDefaultSamplerSlot(sampler_name.c_str());
            if (slot == -1) {
                continue;
            }
            SamplerOverride override{};
            override.pass_id = i;
            override.slot = slot;
            override.texture_id = kv.second->getId();
            compiled_sampler_overrides.push_back(override);
        }
    }
    LOG_DBG("Renderable compiled in " << timer_.stop() * 1000.f << "ms");
}


gpuGeometryRenderable::gpuGeometryRenderable(gpuMaterial* mat, const gpuMeshDesc* mesh, const gpuInstancingDesc* instancing, const char* dbg_name)
    : gpuRenderable(mat, mesh, instancing, dbg_name) {
    ubuf_model = gpuGetPipeline()->createUniformBuffer(UNIFORM_BUFFER_MODEL);
    loc_transform = ubuf_model->getDesc()->getUniform(UNIFORM_MODEL_TRANSFORM);
    loc_transform_prev = ubuf_model->getDesc()->getUniform(UNIFORM_MODEL_TRANSFORM_PREV);
    attachUniformBuffer(ubuf_model);
    compile();
}
gpuGeometryRenderable::~gpuGeometryRenderable() {
    gpuGetPipeline()->destroyUniformBuffer(ubuf_model);
}