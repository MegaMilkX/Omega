#pragma once

#include "gpu_mesh.hpp"
#include "gpu_material.hpp"
#include "render_id.hpp"

class gpuRenderable {
    gpuRenderMaterial* material = 0;
    const gpuMeshDesc* mesh_desc = 0;

public:
    struct AttribBinding {
        const gpuBuffer* buffer;
        int location;
        int count;
        GLenum gl_type;
        bool normalized;
        int stride;
    };

    struct BindingData {
        int technique;
        int pass;
        std::vector<AttribBinding> attribs;
        const gpuBuffer* index_buffer;
    };

    std::vector<BindingData> binding_array;
    
    ktRenderPass* active_pass = 0;

    std::vector<gpuUniformBuffer*> uniform_buffers;
public:
    const gpuRenderMaterial* getMaterial() const {
        return material;
    }
    const gpuMeshDesc* getMeshDesc() const {
        return mesh_desc;
    }

    gpuRenderable& setMaterial(gpuRenderMaterial* material) {
        this->material = material;
        return *this;
    }
    gpuRenderable& setMeshDesc(const gpuMeshDesc* mesh) {
        this->mesh_desc = mesh;
        return *this;
    }
    gpuRenderable& attachUniformBuffer(gpuUniformBuffer* buf) {
        uniform_buffers.push_back(buf);
        return *this;
    }

    void compile() {
        binding_array.clear();
        for (int i = 0; i < material->techniqueCount(); ++i) {
            auto tech = material->getTechniqueByLocalId(i);
            if (!tech) {
                continue;
            }
            for (int j = 0; j < tech->passCount(); ++j) {
                auto pass = tech->getPass(j);

                BindingData binding_data = { 0 };
                binding_data.technique = material->getTechniquePipelineId(i);
                binding_data.pass = j;
                binding_data.index_buffer = mesh_desc->getIndexBuffer();
                for (auto& it : pass->attrib_table) {
                    VFMT::GUID attrib_guid = it.first;
                    int location = it.second;

                    int lcl_attrib_id = mesh_desc->getLocalAttribId(attrib_guid);
                    if (lcl_attrib_id == -1) {
                        LOG_WARN("Mesh attribute required by the material is not present (" << attrib_guid << ")");
                        //assert(false);
                        continue;
                    }
                    const gpuMeshDesc::AttribDesc& lclAttrDesc = mesh_desc->getLocalAttribDesc(lcl_attrib_id);
                    const VFMT::ATTRIB_DESC* attribDesc = VFMT::getAttribDesc(attrib_guid);

                    AttribBinding binding = { 0 };
                    binding.buffer = lclAttrDesc.buffer;
                    binding.location = location;
                    binding.count = attribDesc->count;
                    binding.gl_type = attribDesc->gl_type;
                    binding.normalized = attribDesc->normalized;
                    binding.stride = lclAttrDesc.stride;
                    binding_data.attribs.push_back(binding);
                }
                binding_array.push_back(binding_data);
            }
        }
    }

    void bindMesh(int binding_id) {
        auto& binding = binding_array[binding_id];
        for (auto& a : binding.attribs) {
            if (!a.buffer) {
                assert(false);
                continue;
            }
            a.buffer->bindArray();
            glEnableVertexAttribArray(a.location);
            glVertexAttribPointer(
                a.location, a.count, a.gl_type, a.normalized, a.stride, (void*)0
            );
        }
        if (binding.index_buffer) {
            binding.index_buffer->bindIndexArray();
        }
    }
    void bindTechniquePass(int technique, int pass) {
        active_pass = material->getTechniqueByLocalId(technique)->getPass(pass);
    }
    void bindUniformBuffers() {
        for (int i = 0; i < uniform_buffers.size(); ++i) {
            auto& ub = uniform_buffers[i];
            GLint gl_id = ub->gpu_buf.getId();
            glBindBufferBase(GL_UNIFORM_BUFFER, ub->getDesc()->id, gl_id);
        }
    }

    template<typename UNIFORM>
    void setUniform(typename const UNIFORM::VALUE_T& value) {
        active_pass->setUniform<UNIFORM>(value);
    }

    void drawArrays() {
        mesh_desc->_drawArrays();
    }
    void drawIndexed() {
        mesh_desc->_drawIndexed();
    }
    void drawArraysLine() {
        mesh_desc->_drawArraysLine();
    }
    void drawArraysLineStrip() {
        mesh_desc->_drawArraysLineStrip();
    }
};