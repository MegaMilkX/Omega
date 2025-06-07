#include "gpu_mesh_desc.hpp"

#include "gpu_shader_program.hpp"
#include "gpu_material.hpp"
#include "gpu_instancing_desc.hpp"
#include "gpu_util.hpp"


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


gpuMeshMaterialBinding* gpuCreateMeshMaterialBinding(
    const gpuMaterial* material,
    const gpuMeshDesc* desc,
    const gpuInstancingDesc* inst_desc
) {
    auto ptr = new gpuMeshMaterialBinding;
    if (!gpuMakeMeshMaterialBinding(ptr, material, desc, inst_desc)) {
        delete ptr;
        return 0;
    }
    return ptr;

}

void gpuDestroyMeshMaterialBinding(gpuMeshMaterialBinding* binding) {
    delete binding;
}

bool gpuMakeMeshMaterialBinding(
    gpuMeshMaterialBinding* binding,
    const gpuMaterial* material,
    const gpuMeshDesc* desc,
    const gpuInstancingDesc* inst_desc
) {
    binding->binding_array.clear();

    for (int j = 0; j < material->passCount(); ++j) {
        auto pass = material->getPass(j);
        auto prog = pass->getShader();
        gpuMeshMaterialBinding::BindingData bd;
        bd.pass = pass->getPipelineIdx(); // Must be index of the pipeline pass in linear storage
        bd.material_pass = j;
        gpuMakeMeshShaderBinding(&bd.binding, prog, desc, inst_desc);
        binding->binding_array.push_back(bd);
    }
    return true;
}
