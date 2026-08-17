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

static uint32_t passResolutionKey(GPU_MESH_DESC_TYPE mdt, GPU_ShadingStyle st) {
    return uint32_t(mdt) | (uint32_t(st) << 16);
}

struct PassResolution {
    const char* pass_name;
    GPU_BLEND_MODE blend_mode; // overridable by material
    uint32_t draw_flags;       // overridable by material
    bool cast_shadows;
};

static std::unordered_map<uint32_t, PassResolution> pass_resolution_table = {
    { 
        passResolutionKey(GPU_MESH_DESC_TYPE::GENERIC, GPU_ShadingStyle::Opaque),
        { "Default", GPU_BLEND_MODE::BLEND, GPU_DEPTH_WRITE | GPU_DEPTH_TEST | GPU_BACKFACE_CULLING, true }
    },
    { 
        passResolutionKey(GPU_MESH_DESC_TYPE::GENERIC, GPU_ShadingStyle::ForwardTranslucent),
        { "HL2/Translucent", GPU_BLEND_MODE::BLEND, GPU_DEPTH_TEST | GPU_BACKFACE_CULLING, true }
    },
    { 
        passResolutionKey(GPU_MESH_DESC_TYPE::GENERIC, GPU_ShadingStyle::VFX),
        { "VFX", GPU_BLEND_MODE::ADD, GPU_BACKFACE_CULLING, false }
    },
    { 
        passResolutionKey(GPU_MESH_DESC_TYPE::GENERIC, GPU_ShadingStyle::WATER),
        { "HL2/Water", GPU_BLEND_MODE::BLEND, GPU_BACKFACE_CULLING, false }
    },
    { 
        passResolutionKey(GPU_MESH_DESC_TYPE::SPRITE, GPU_ShadingStyle::Opaque),
        { "Default", GPU_BLEND_MODE::BLEND, GPU_DEPTH_WRITE | GPU_DEPTH_TEST | GPU_BACKFACE_CULLING, true }
    },
    { 
        passResolutionKey(GPU_MESH_DESC_TYPE::SPRITE, GPU_ShadingStyle::ForwardTranslucent),
        { "HL2/Translucent", GPU_BLEND_MODE::BLEND, GPU_DEPTH_TEST | GPU_BACKFACE_CULLING, false }
    },
    { 
        passResolutionKey(GPU_MESH_DESC_TYPE::SPRITE, GPU_ShadingStyle::VFX),
        { "VFX", GPU_BLEND_MODE::ADD, 0, false }
    },
    { 
        passResolutionKey(GPU_MESH_DESC_TYPE::DECAL, GPU_ShadingStyle::Opaque),
        { "Decals_GBuffer", GPU_BLEND_MODE::BLEND, GPU_BACKFACE_CULLING, false }
    },
    { 
        passResolutionKey(GPU_MESH_DESC_TYPE::DECAL, GPU_ShadingStyle::ForwardTranslucent),
        { "Decals", GPU_BLEND_MODE::BLEND, GPU_BACKFACE_CULLING, false }
    },
    { 
        passResolutionKey(GPU_MESH_DESC_TYPE::DECAL, GPU_ShadingStyle::VFX),
        { "Decals", GPU_BLEND_MODE::ADD, GPU_BACKFACE_CULLING, false }
    },
    { 
        passResolutionKey(GPU_MESH_DESC_TYPE::TEXT, GPU_ShadingStyle::Opaque),
        { "Default", GPU_BLEND_MODE::BLEND, GPU_DEPTH_WRITE | GPU_DEPTH_TEST | GPU_BACKFACE_CULLING, true }
    },
    { 
        passResolutionKey(GPU_MESH_DESC_TYPE::TEXT, GPU_ShadingStyle::ForwardTranslucent),
        { "HL2/Translucent", GPU_BLEND_MODE::BLEND, GPU_DEPTH_TEST | GPU_BACKFACE_CULLING, true }
    },
    { 
        passResolutionKey(GPU_MESH_DESC_TYPE::TEXT, GPU_ShadingStyle::VFX),
        { "VFX", GPU_BLEND_MODE::ADD, GPU_BACKFACE_CULLING, false }
    },
    { 
        passResolutionKey(GPU_MESH_DESC_TYPE::LINE, GPU_ShadingStyle::Opaque),
        { "Overlay", GPU_BLEND_MODE::BLEND, GPU_DEPTH_WRITE | GPU_DEPTH_TEST | GPU_BACKFACE_CULLING, false }
    },
    { 
        passResolutionKey(GPU_MESH_DESC_TYPE::LINE, GPU_ShadingStyle::ForwardTranslucent),
        { "Overlay", GPU_BLEND_MODE::BLEND, GPU_DEPTH_TEST | GPU_BACKFACE_CULLING, false }
    },
    { 
        passResolutionKey(GPU_MESH_DESC_TYPE::LINE, GPU_ShadingStyle::VFX),
        { "Overlay", GPU_BLEND_MODE::ADD, GPU_BACKFACE_CULLING, false }
    },
    { 
        passResolutionKey(GPU_MESH_DESC_TYPE::GIZMO, GPU_ShadingStyle::Opaque),
        { "Overlay", GPU_BLEND_MODE::BLEND, GPU_DEPTH_WRITE | GPU_DEPTH_TEST | GPU_BACKFACE_CULLING, false }
    },
    { 
        passResolutionKey(GPU_MESH_DESC_TYPE::GIZMO, GPU_ShadingStyle::ForwardTranslucent),
        { "Overlay", GPU_BLEND_MODE::BLEND, GPU_DEPTH_TEST | GPU_BACKFACE_CULLING, false }
    },
    { 
        passResolutionKey(GPU_MESH_DESC_TYPE::GIZMO, GPU_ShadingStyle::VFX),
        { "Overlay", GPU_BLEND_MODE::ADD, GPU_BACKFACE_CULLING, false }
    },
};

static PassResolution error_pass_resolution = {
    "Error", GPU_BLEND_MODE::BLEND, 0, false
};

static void resolveMaterialParams(GPU_INTERMEDIATE_PASS_DESC* pass, const gpuMaterial* mat, GPU_BLEND_MODE in_blending, draw_flags_t in_draw_flags) {
    GPU_BLEND_MODE blending = in_blending;
    draw_flags_t draw_flags = in_draw_flags;

    if(mat) {
        blending = mat->getBlendingMode();

        draw_flags = mat->getDepthTest() ? (draw_flags | GPU_DEPTH_TEST) : (draw_flags & ~GPU_DEPTH_TEST);
        draw_flags = mat->getDepthWrite() ? (draw_flags | GPU_DEPTH_WRITE) : (draw_flags & ~GPU_DEPTH_WRITE);
        draw_flags = mat->getStencilTest() ? (draw_flags | GPU_STENCIL_TEST) : (draw_flags & ~GPU_STENCIL_TEST);
        draw_flags = mat->getBackfaceCulling() ? (draw_flags | GPU_BACKFACE_CULLING) : (draw_flags & ~GPU_BACKFACE_CULLING);
    }

    pass->blend_mode = blending;
    pass->draw_flags = draw_flags;
}
void resolveRenderable(GPU_INTERMEDIATE_RENDERABLE_CONTEXT& ctx, const gpuMeshDesc* mesh_desc, const gpuMaterial* mat) {
    GPU_ShadingStyle style = GPU_ShadingStyle::Opaque;
    if (mat) {
        style = mat->getShadingStyle();
    }
    uint32_t key = passResolutionKey(mesh_desc->mesh_type, style);
    const PassResolution* reso = &error_pass_resolution;
    auto it = pass_resolution_table.find(key);
    if (it != pass_resolution_table.end()) {
        reso = &it->second;
    } else {
        reso = &error_pass_resolution;
        mat = nullptr;
    }

    bool cast_shadows = reso->cast_shadows; // TODO: Material should have a say

    // Primary pass
    int pipe_pass_id = gpuGetPipeline()->getPassId(reso->pass_name);
    if(pipe_pass_id >= 0) {
        GPU_INTERMEDIATE_PASS_DESC* pass = ctx.getOrCreatePass(pipe_pass_id);
        if (mat) {
            pass->material_shader_key = mat->getShaderKey();
            if (mat->hasVertexShaders()) {
                pass->material_vertex_shaders = mat->getVertexShaders();
            }
            if (mat->hasFragmentShaders()) {
                pass->material_fragment_shaders = mat->getFragmentShaders();
            }
            resolveMaterialParams(pass, mat, reso->blend_mode, reso->draw_flags);
        }
    }

    // Shadows
    if (cast_shadows) {
        GPU_INTERMEDIATE_PASS_DESC* pass = ctx.getOrCreatePass(gpuGetPipeline()->getPassId("Shadowmap"));
        if(mat) {
            pass->material_shader_key = mat->getShaderKey();
            if (mat->hasVertexShaders()) {
                pass->material_vertex_shaders = mat->getVertexShaders();
            }
            if (mat->hasFragmentShaders()) {
                pass->material_fragment_shaders = mat->getFragmentShaders();
            }
        }
        resolveMaterialParams(pass, mat, GPU_BLEND_MODE::BLEND, GPU_DEPTH_TEST | GPU_DEPTH_WRITE | GPU_BACKFACE_CULLING);
    }

    // Wireframe
    {
        GPU_INTERMEDIATE_PASS_DESC* wire_pass = ctx.getOrCreatePass(gpuGetPipeline()->getPassId("Wireframe"));
        wire_pass->blend_mode = GPU_BLEND_MODE::BLEND;
        wire_pass->draw_flags = GPU_DEPTH_WRITE | GPU_DEPTH_TEST;
        if(mat) {
            wire_pass->material_shader_key = mat->getShaderKey();
            if (mat->hasVertexShaders()) {
                wire_pass->material_vertex_shaders = mat->getVertexShaders();
            }
        }
    }
}


bool gpuCompileRenderablePasses(
    gpuCompiledRenderableDesc* binding,
    const gpuRenderable* renderable,
    const gpuMaterial* material,
    const gpuMeshDesc* mesh_desc,
    const gpuInstancingDesc* inst_desc
) {
    assert(mesh_desc);
    binding->pass_array.clear();

    GPU_INTERMEDIATE_RENDERABLE_CONTEXT ctx;

    GPU_Role role = renderable->role;
    if (material && material->getRoleOverride().has_value()) {
        role = material->getRoleOverride().value();
    }
    
    //gpuGetPipeline()->resolveRenderableRole(role, ctx, material);
    resolveRenderable(ctx, mesh_desc, material);

    for (int i = 0; i < GPU_EFFECT_COUNT; ++i) {
        auto e_tpl = static_cast<GPU_Effect>(i);
        if ((renderable->effect_flags & (1 << i)) == 0) {
            continue;
        }
        gpuGetPipeline()->resolveRenderableEffect(e_tpl, ctx, material);
    }

    // Get base shaders from pipeline passes
    for (auto it = ctx.pass_map.begin(); it != ctx.pass_map.end(); ++it) {
        pipe_pass_id_t pip_pass_id = it->first;
        GPU_INTERMEDIATE_PASS_DESC* int_pass = &it->second;

        int_pass->base_shaders.clear(); // Should be empty, but just in case
        auto pipe_pass = gpuGetPipeline()->getPass(pip_pass_id);
        auto base_shaders = pipe_pass->getBaseShaderSets();
        for (auto base_shader_set : base_shaders) {
            int_pass->base_shaders.push_back(base_shader_set.get());
        }

        mesh_desc->apply(*int_pass);
        if (inst_desc) {
            inst_desc->apply(*int_pass);
        }
        if (!int_pass->to_world_vertex_shaders) {
            int_pass->to_world_vertex_shaders = gpuGetDevice()->getSharedResources()->getToWorldShader();
        }
    }

    if(material) {
        renderable->material_version = material->getVersion();

        // Per pass overrides
        /*
        for (int i = 0; i < material->passCount(); ++i) {
            auto mat_pass = material->getPass(i);
            pipe_pass_id_t pip_pass_id = mat_pass->getPipelineIdx();
            auto int_pass = ctx.getOrCreatePass(pip_pass_id);
            if (mat_pass->shaderSetCount()) {
                int_pass->base_shaders.clear();
                for (int j = 0; j < mat_pass->shaderSetCount(); ++j) {
                    int_pass->base_shaders.push_back(mat_pass->getShaderSet(j));
                }
            }
            int_pass->blend_mode = mat_pass->blend_mode;
            int_pass->draw_flags = 0;
            int_pass->draw_flags |= mat_pass->depth_write ? GPU_DEPTH_WRITE : 0;
            int_pass->draw_flags |= mat_pass->depth_test ? GPU_DEPTH_TEST : 0;
            int_pass->draw_flags |= mat_pass->cull_faces ? GPU_BACKFACE_CULLING : 0;
            int_pass->draw_flags |= mat_pass->stencil_test ? GPU_STENCIL_TEST : 0;
        }*/
    }

    for (auto it = ctx.pass_map.begin(); it != ctx.pass_map.end(); ++it) {
        pipe_pass_id_t pip_pass_id = it->first;
        GPU_INTERMEDIATE_PASS_DESC* int_pass = &it->second;

        auto& rpd = binding->pass_array.emplace_back();
        rpd.pass = pip_pass_id;
        rpd.blend_mode = int_pass->blend_mode;
        rpd.draw_flags = int_pass->draw_flags;
        rpd.state_identity = uint32_t(rpd.draw_flags) | (uint32_t(rpd.blend_mode) << 16);

        PassShaderKey key;
        key.use_material_vertex = int_pass->material_vertex_shaders != 0;
        key.use_material_fragment = int_pass->material_fragment_shaders != 0;
        key.use_attrib_vertex = int_pass->attrib_vertex_shaders != 0;
        key.use_attrib_geometry = int_pass->attrib_geometry_shaders != 0;
        key.use_attrib_fragment = int_pass->attrib_fragment_shaders != 0;

        for (int i = 0; i < int_pass->base_shaders.size(); ++i) {
            gpuCompilePassShaderSet(*int_pass, int_pass->base_shaders[i], key);
        }

        gpuCompileGenericShaderSet(*int_pass, int_pass->attrib_vertex_shaders, nullptr);
        gpuCompileGenericShaderSet(*int_pass, int_pass->attrib_geometry_shaders, nullptr);
        gpuCompileGenericShaderSet(*int_pass, int_pass->attrib_fragment_shaders, nullptr);

        TransformShaderKey transform_key;
        transform_key.mode = renderable->dbg_billboard ? GPU_TransformMode::Billboard : GPU_TransformMode::World;
        gpuCompileTransformShaderSet(*int_pass, int_pass->to_world_vertex_shaders, transform_key);

        gpuCompileGenericShaderSet(*int_pass, int_pass->material_vertex_shaders, int_pass->material_shader_key);
        gpuCompileGenericShaderSet(*int_pass, int_pass->material_fragment_shaders, int_pass->material_shader_key);

        rpd.prog = gpuGetProgram(int_pass->shaders.data(), int_pass->shaders.size());
        if (!rpd.prog) {
            LOG_ERR("gpuGetProgram failed, shader set list: ");
            if(int_pass->base_shaders.size()) {
                LOG_ERR("Base shaders: ");
                for (int k = 0; k < int_pass->base_shaders.size(); ++k) {
                    LOG_ERR(int_pass->base_shaders[k]->dbgGetName());
                }
            }
            if (int_pass->material_vertex_shaders) {
                LOG_ERR("Material vertex shaders: ");
                LOG_ERR(int_pass->material_vertex_shaders->dbgGetName());
            }
            if (int_pass->material_fragment_shaders) {
                LOG_ERR("Material fragment shaders: ");
                LOG_ERR(int_pass->material_fragment_shaders->dbgGetName());
            }
            if (int_pass->attrib_vertex_shaders) {
                LOG_ERR("Attribute vertex shaders: ");
                LOG_ERR(int_pass->attrib_vertex_shaders->dbgGetName());
            }
            if (int_pass->attrib_fragment_shaders) {
                LOG_ERR("Attribute fragment shaders: ");
                LOG_ERR(int_pass->attrib_fragment_shaders->dbgGetName());
            }
            if (int_pass->to_world_vertex_shaders) {
                LOG_ERR("To World vertex shaders: ");
                LOG_ERR(int_pass->to_world_vertex_shaders->dbgGetName());
            }
            return false;
        }

        if (!gpuGetPipeline()->validateProgram(rpd.prog.get())) {
            return false;
        }

        for (auto kv : int_pass->textures) {
            const auto& sampler_name = kv.first;
            ShaderSamplerSet::Sampler sampler = kv.second;
            sampler.slot = rpd.prog->getDefaultSamplerSlot(sampler_name.c_str());
            if (sampler.slot < 0) {
                continue;
            }
            rpd.sampler_set.add(sampler);
        }
    }

    for (int i = 0; i < binding->pass_array.size(); ++i) {
        auto& compiled_pass_data = binding->pass_array[i];
        gpuMakeMeshShaderBinding(&compiled_pass_data.binding, compiled_pass_data.prog.get(), mesh_desc, inst_desc);
    }

    return true;
}

