#pragma once

#include "gpu_mesh.hpp"
#include "gpu_mesh_desc.hpp"
#include "gpu_instancing_desc.hpp"
#include "gpu_material.hpp"
#include "render_id.hpp"

class gpuRenderable {
    gpuMaterial* material = 0;
    const gpuMeshDesc* mesh_desc = 0;
    const gpuInstancingDesc* instancing_desc = 0;

public:
    std::shared_ptr<gpuMeshMaterialBinding> desc_binding;
    std::vector<gpuUniformBuffer*> uniform_buffers;

public:
    std::string dbg_name;

    gpuRenderable() {}
    gpuRenderable(gpuMaterial* mat, const gpuMeshDesc* mesh, const gpuInstancingDesc* instancing = 0, const char* dbg_name = "noname")
    : dbg_name(dbg_name) {
        material = mat;
        mesh_desc = mesh;
        instancing_desc = instancing;
        compile();
    }
    virtual ~gpuRenderable() {}

    bool isInstanced() const {
        return mesh_desc && instancing_desc;
    }
    const gpuMaterial* getMaterial() const {
        return material;
    }
    const gpuMeshDesc* getMeshDesc() const {
        return mesh_desc;
    }
    const gpuInstancingDesc* getInstancingDesc() const {
        return instancing_desc;
    }

    gpuRenderable& setMaterial(gpuMaterial* material) {
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
            desc_binding.reset(new gpuMeshMaterialBinding);
            gpuMakeMeshMaterialBinding(desc_binding.get(), material, mesh_desc, 0);
        } else {
            desc_binding.reset(new gpuMeshMaterialBinding);
            gpuMakeMeshMaterialBinding(desc_binding.get(), material, mesh_desc, instancing_desc);
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

class gpuGeometryRenderable : public gpuRenderable {
    gpuUniformBuffer* ubuf_model = 0;
    int loc_transform;
public:
    gpuGeometryRenderable(gpuMaterial* mat, const gpuMeshDesc* mesh, const gpuInstancingDesc* instancing = 0, const char* dbg_name = "noname");
    ~gpuGeometryRenderable();

    void setTransform(const gfxm::mat4& t) {
        ubuf_model->setMat4(loc_transform, t);
    }
};