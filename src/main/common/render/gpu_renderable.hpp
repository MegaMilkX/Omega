#pragma once

#include "gpu_mesh.hpp"
#include "gpu_mesh_desc.hpp"
#include "gpu_material.hpp"
#include "render_id.hpp"

class gpuRenderable {
    gpuRenderMaterial* material = 0;
    const gpuMeshDesc* mesh_desc = 0;

public:
    const gpuMeshDescBinding* desc_binding = 0;
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
        desc_binding = material->getMeshDescBinding(mesh_desc);
    }

    void bindMesh(int binding_id) {
        auto& binding = desc_binding->binding_array[binding_id];
        for (auto& a : binding.binding->attribs) {
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
        if (binding.binding->index_buffer) {
            binding.binding->index_buffer->bindIndexArray();
        }
    }
    void bindUniformBuffers() {
        for (int i = 0; i < uniform_buffers.size(); ++i) {
            auto& ub = uniform_buffers[i];
            GLint gl_id = ub->gpu_buf.getId();
            glBindBufferBase(GL_UNIFORM_BUFFER, ub->getDesc()->id, gl_id);
        }
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