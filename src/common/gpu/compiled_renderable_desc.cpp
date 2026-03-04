#include "compiled_renderable_desc.hpp"

#include "gpu/gpu_util.hpp"
#include "gpu/gpu.hpp"
#include "gpu/gpu_renderable.hpp"
#include "gpu/gpu_material.hpp"
#include "resource_manager/resource_manager.hpp"


void gpuBindMeshBinding(const gpuMeshShaderBinding* binding) {
    glBindVertexArray(binding->vao);
    //gpuBindMeshBindingDirect(binding);
}
void gpuBindMeshBindingDirect(const gpuMeshShaderBinding* binding) {
    for (auto& a : binding->attribs) {
        if (!a.buffer) {
            LOG_ERR("gpuBindMeshBindingDirect: " << VFMT::getAttribDesc(a.guid)->name << " buffer is null");
            assert(false);
            continue;
        }
        if (a.buffer->getId() == 0) {
            LOG_ERR("gpuBindMeshBindingDirect: invalid " << VFMT::getAttribDesc(a.guid)->name << " buffer id");
            assert(false);
            continue;
        }
        if (a.buffer->getSize() == 0) {
            LOG_WARN("gpuBindMeshBindingDirect: " << VFMT::getAttribDesc(a.guid)->name << " buffer is empty");
            // NOTE: On first renderable compilation buffers can be empty, which is not an error
            // Still, would be nice to catch empty buffers before drawing
            //assert(false);
            //continue;
        }
        GL_CHECK(glEnableVertexAttribArray(a.location));
        GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, a.buffer->getId()));
        GL_CHECK(glVertexAttribPointer(
            a.location, a.count, a.gl_type, a.normalized, a.stride, (void*)a.offset
        ));
        if (a.is_instance_array) {
            glVertexAttribDivisor(a.location, 1);
        } else {
            glVertexAttribDivisor(a.location, 0);
        }
    }
    if (binding->index_buffer) {
        GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, binding->index_buffer->getId()));
    }
}
void gpuDrawMeshBinding(const gpuMeshShaderBinding* b) {
    GLenum mode;
    switch (b->draw_mode) {
    case MESH_DRAW_POINTS: mode = GL_POINTS; break;
    case MESH_DRAW_LINES: mode = GL_LINES; break;
    case MESH_DRAW_LINE_STRIP: mode = GL_LINE_STRIP; break;
    case MESH_DRAW_LINE_LOOP: mode = GL_LINE_LOOP; break;
    case MESH_DRAW_TRIANGLES: mode = GL_TRIANGLES; break;
    case MESH_DRAW_TRIANGLE_STRIP: mode = GL_TRIANGLE_STRIP; break;
    case MESH_DRAW_TRIANGLE_FAN: mode = GL_TRIANGLE_FAN; break;
    default: assert(false);
    };
    if (b->index_buffer) {
        GL_CHECK(glDrawElements(mode, b->index_count, GL_UNSIGNED_INT, 0));
    } else {
        GL_CHECK(glDrawArrays(mode, 0, b->vertex_count));
    }
}
void gpuDrawMeshBindingInstanced(const gpuMeshShaderBinding* binding, int instance_count) {
    GLenum mode;
    switch (binding->draw_mode) {
    case MESH_DRAW_POINTS: mode = GL_POINTS; break;
    case MESH_DRAW_LINES: mode = GL_LINES; break;
    case MESH_DRAW_LINE_STRIP: mode = GL_LINE_STRIP; break;
    case MESH_DRAW_LINE_LOOP: mode = GL_LINE_LOOP; break;
    case MESH_DRAW_TRIANGLES: mode = GL_TRIANGLES; break;
    case MESH_DRAW_TRIANGLE_STRIP: mode = GL_TRIANGLE_STRIP; break;
    case MESH_DRAW_TRIANGLE_FAN: mode = GL_TRIANGLE_FAN; break;
    default: assert(false);
    };
    if (binding->index_buffer) {
        glDrawElementsInstanced(mode, binding->index_count, GL_UNSIGNED_INT, 0, instance_count);
    } else {
        glDrawArraysInstanced(mode, 0, binding->vertex_count, instance_count);
    }
}


gpuMeshShaderBinding* gpuCreateMeshShaderBinding(
    const gpuShaderProgram* prog,
    const gpuMeshDesc* desc,
    const gpuInstancingDesc* inst_desc
) {
    auto ptr = new gpuMeshShaderBinding;
    if (!gpuMakeMeshShaderBinding(ptr, prog, desc, inst_desc)) {
        delete ptr;
        return 0;
    }
    return ptr;
}

void gpuDestroyMeshShaderBinding(gpuMeshShaderBinding* binding) {
    delete binding;
}

bool gpuMakeMeshShaderBinding(
    gpuMeshShaderBinding* out_binding,
    const gpuShaderProgram* prog,
    const gpuMeshDesc* desc,
    const gpuInstancingDesc* inst_desc
) {
    //LOG("gpuMakeMeshShaderBinding() BEGIN");
    out_binding->attribs.clear();

    if (out_binding->vao) {
        glDeleteVertexArrays(1, &out_binding->vao);
    }
    glGenVertexArrays(1, &out_binding->vao);

    out_binding->index_buffer = desc->getIndexBuffer();
    for (auto& it : prog->getAttribTable()) {
        VFMT::GUID attr_guid = it.first;
        int loc = it.second;

        bool is_instance_array = false;
        const gpuBuffer* buffer = 0;
        int stride = 0;
        int offset = 0;
        const VFMT::ATTRIB_DESC* attrDesc = attrDesc = VFMT::getAttribDesc(attr_guid);
        int lcl_attrib_id = desc->getLocalAttribId(attr_guid);
        int lcl_instance_attrib_id = -1;
        if (inst_desc) {
            lcl_instance_attrib_id = inst_desc->getLocalInstanceAttribId(attr_guid);
        }
        if (lcl_attrib_id >= 0) {
            auto& dsc = desc->getLocalAttribDesc(lcl_attrib_id);
            buffer = dsc.buffer;
            stride = dsc.stride;
            offset = dsc.offset;
        } else if (inst_desc && lcl_instance_attrib_id >= 0) {
            auto& dsc = inst_desc->getLocalInstanceAttribDesc(lcl_instance_attrib_id);
            buffer = dsc.buffer;
            stride = dsc.stride;
            offset = dsc.offset;
            is_instance_array = true;
        } else {
            LOG_WARN("gpuMeshDesc or gpuInstancingDesc missing an attribute required by the shader program: " << VFMT::guidToString(attr_guid));
            continue;
        }   

        gpuAttribBinding binding = { 0 };
        binding.guid = attr_guid;
        binding.buffer = buffer;
        binding.location = loc;
        binding.count = attrDesc->count;
        binding.gl_type = attrDesc->gl_type;
        binding.normalized = attrDesc->normalized;
        binding.stride = stride;
        binding.offset = offset;
        binding.is_instance_array = is_instance_array;
        {
            auto desc = VFMT::getAttribDesc(attr_guid);
            //LOG("Program attrib loc " << loc << ": " << attrDesc->name);
        }
        out_binding->attribs.push_back(binding);
    }
    std::sort(out_binding->attribs.begin(), out_binding->attribs.end(), [](const gpuAttribBinding& a, const gpuAttribBinding& b)->bool {
        return a.location < b.location;
    });

    out_binding->index_count = 0;
    if (desc->hasIndexArray()) {
        out_binding->index_count = desc->getIndexCount();
    }
    out_binding->vertex_count = desc->getVertexCount();
    out_binding->draw_mode = desc->draw_mode;

    glBindVertexArray(out_binding->vao);
    gpuBindMeshBindingDirect(out_binding);
    glBindVertexArray(0);

    //LOG("gpuMakeMeshShaderBinding() END");
    return true;
}


#include "gpu/program_lib.hpp"

bool gpuCompileRenderablePasses(
    gpuCompiledRenderableDesc* binding,
    const gpuRenderable* renderable,
    const gpuMaterial* material,
    const gpuMeshDesc* desc,
    const gpuInstancingDesc* inst_desc
) {
    binding->pass_array.clear();

    GPU_INTERMEDIATE_RENDERABLE_CONTEXT ctx;

    GPU_Role role = renderable->role;
    if (material && material->getRoleOverride().has_value()) {
        role = material->getRoleOverride().value();
    }
    gpuGetPipeline()->resolveRenderableRole(role, ctx, material);
    for (int i = 0; i < GPU_EFFECT_COUNT; ++i) {
        auto e_tpl = static_cast<GPU_Effect>(i);
        if ((renderable->effect_flags & (1 << i)) == 0) {
            continue;
        }
        gpuGetPipeline()->resolveRenderableEffect(e_tpl, ctx, material);
    }

    for (int i = 0; i < material->passCount(); ++i) {
        auto mat_pass = material->getPass(i);
        pipe_pass_id_t pip_pass_id = mat_pass->getPipelineIdx();
        auto int_pass = ctx.getOrCreatePass(pip_pass_id);
        if (mat_pass->shaderSetCount()) {
            int_pass->clearBaseShaderSets();
            for (int j = 0; j < mat_pass->shaderSetCount(); ++j) {
                int_pass->addBaseShaderSet(mat_pass->getShaderSet(j));
            }
        }
        int_pass->blend_mode = mat_pass->blend_mode;
        int_pass->draw_flags = 0;
        int_pass->draw_flags |= mat_pass->depth_write ? GPU_DEPTH_WRITE : 0;
        int_pass->draw_flags |= mat_pass->depth_test ? GPU_DEPTH_TEST : 0;
        int_pass->draw_flags |= mat_pass->cull_faces ? GPU_BACKFACE_CULLING : 0;
        int_pass->draw_flags |= mat_pass->stencil_test ? GPU_STENCIL_TEST : 0;
    }

    for (auto it = ctx.pass_map.begin(); it != ctx.pass_map.end(); ++it) {
        pipe_pass_id_t pip_pass_id = it->first;
        GPU_INTERMEDIATE_PASS_DESC* int_pass = &it->second;

        auto& rpd = binding->pass_array.emplace_back();
        rpd.pass = pip_pass_id;
        rpd.material_pass = -1;
        rpd.blend_mode = int_pass->blend_mode;
        rpd.draw_flags = int_pass->draw_flags;
        rpd.state_identity = uint32_t(rpd.draw_flags) | (uint32_t(rpd.blend_mode) << 16);

        for (int j = 0; j < int_pass->extension_shaders.size(); ++j) {
            auto set = int_pass->extension_shaders[j];
            auto compiled_set = set->getCompiled(int_pass->shader_flags);
            for (int l = 0; l < compiled_set->shaders.size(); ++l) {
                auto compiled_shader = compiled_set->shaders[l].get();
                int_pass->extended_by_material |= (1 << compiled_shader->type);
                int_pass->shaders.push_back(compiled_shader);
            }
        }

        shader_flags_t extension_flags = 0x0;
        extension_flags |= (int_pass->extended_by_material & (1 << SHADER_VERTEX)) ? SHADER_FLAG_ENABLE_VERT_EXTENSION : 0;
        extension_flags |= (int_pass->extended_by_material & (1 << SHADER_FRAGMENT)) ? SHADER_FLAG_ENABLE_FRAG_EXTENSION : 0;

        for (int j = 0; j < int_pass->base_shaders.size(); ++j) {
            auto set = int_pass->base_shaders[j];
            auto compiled_set = set->getCompiled(int_pass->shader_flags | extension_flags);
            for (int l = 0; l < compiled_set->shaders.size(); ++l) {
                auto compiled_shader = compiled_set->shaders[l].get();
                bool should_support_extension = (int_pass->extended_by_material & (1 << compiled_shader->type));
                int_pass->shaders.push_back(compiled_shader);
            }
        }

        rpd.prog = gpuGetProgram(int_pass->shaders.data(), int_pass->shaders.size());
        if (!rpd.prog) {
            LOG_ERR("gpuGetProgram failed, shader set list: ");
            if(int_pass->base_shaders.size()) {
                LOG_ERR("Base shaders: ");
                for (int k = 0; k < int_pass->base_shaders.size(); ++k) {
                    LOG_ERR(int_pass->base_shaders[k]->dbgGetName());
                }
            }
            if(int_pass->extension_shaders.size()) {
                LOG_ERR("Extension shaders: ");
                for (int k = 0; k < int_pass->extension_shaders.size(); ++k) {
                    LOG_ERR(int_pass->extension_shaders[k]->dbgGetName());
                }
            }
        }
    }

    for (int i = 0; i < binding->pass_array.size(); ++i) {
        auto& compiled_pass_data = binding->pass_array[i];
        gpuMakeMeshShaderBinding(&compiled_pass_data.binding, compiled_pass_data.prog.get(), desc, inst_desc);
    }

    return true;
}

