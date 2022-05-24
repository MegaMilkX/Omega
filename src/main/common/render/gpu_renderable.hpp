#pragma once

#include "gpu_mesh.hpp"
#include "gpu_mesh_desc.hpp"
#include "gpu_instancing_desc.hpp"
#include "gpu_material.hpp"
#include "render_id.hpp"

class gpuRenderable {
    gpuRenderMaterial* material = 0;
    const gpuMeshDesc* mesh_desc = 0;
    const gpuInstancingDesc* instancing_desc = 0;

public:
    const gpuMeshDescBinding* desc_binding = 0;
    std::vector<gpuUniformBuffer*> uniform_buffers;

public:
    gpuRenderable() {}
    gpuRenderable(gpuRenderMaterial* mat, const gpuMeshDesc* mesh, const gpuInstancingDesc* instancing = 0) {
        material = mat;
        mesh_desc = mesh;
        instancing_desc = instancing;
        compile();
    }
    bool isInstanced() const {
        return mesh_desc && instancing_desc;
    }
    const gpuRenderMaterial* getMaterial() const {
        return material;
    }
    const gpuMeshDesc* getMeshDesc() const {
        return mesh_desc;
    }
    const gpuInstancingDesc* getInstancingDesc() const {
        return instancing_desc;
    }

    gpuRenderable& setMaterial(gpuRenderMaterial* material) {
        this->material = material;
        return *this;
    }
    gpuRenderable& setMeshDesc(const gpuMeshDesc* mesh) {
        this->mesh_desc = mesh;
        return *this;
    }
    gpuRenderable& setInstancingDesc(const gpuInstancingDesc* instancing) {
        this->instancing_desc = instancing;
        return *this;
    }
    gpuRenderable& attachUniformBuffer(gpuUniformBuffer* buf) {
        uniform_buffers.push_back(buf);
        return *this;
    }

    void compile() {
        if (!instancing_desc) {
            desc_binding = material->getMeshDescBinding(mesh_desc);
        } else {
            desc_binding = material->getMeshDescBinding(mesh_desc, instancing_desc);
        }
    }

    void bindUniformBuffers() {
        for (int i = 0; i < uniform_buffers.size(); ++i) {
            auto& ub = uniform_buffers[i];
            GLint gl_id = ub->gpu_buf.getId();
            glBindBufferBase(GL_UNIFORM_BUFFER, ub->getDesc()->id, gl_id);
        }
    }
};