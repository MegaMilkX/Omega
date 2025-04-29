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

    struct SamplerOverride {
        short material_local_technique_id;
        short pass_id;
        int slot;
        GLuint texture_id;
    };

public:
    std::vector<gpuUniformBuffer*> uniform_buffers;
    std::map<std::string, HSHARED<gpuTexture2d>> sampler_overrides;
    
    // Compiled data
    std::shared_ptr<gpuMeshMaterialBinding> desc_binding;
    std::vector<bool> pass_states;
    std::vector<SamplerOverride> compiled_sampler_overrides;

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

    void enableMaterialTechnique(const char* path, bool value);

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

    void addSamplerOverride(const char* name, HSHARED<gpuTexture2d> tex) {
        sampler_overrides[name] = tex;
    }

    void compile() {
        if (!instancing_desc) {
            desc_binding.reset(new gpuMeshMaterialBinding);
            gpuMakeMeshMaterialBinding(desc_binding.get(), material, mesh_desc, 0);
        } else {
            desc_binding.reset(new gpuMeshMaterialBinding);
            gpuMakeMeshMaterialBinding(desc_binding.get(), material, mesh_desc, instancing_desc);
        }
        pass_states.resize(desc_binding->binding_array.size());
        std::fill(pass_states.begin(), pass_states.end(), true);

        /*
        compiled_sampler_overrides.clear();
        assert(sampler_overrides.size() <= 32);
        for (auto& kv : sampler_overrides) {
            if (!kv.second.isValid()) {
                LOG_ERR("Renderable sampler override " << kv.first << " texture handle is invalid");
                continue;
            }

            for (int i = 0; i < material->techniqueCount(); ++i) {
                auto tech = material->getTechniqueByLocalId(i);
                for (int j = 0; j < tech->passCount(); ++j) {
                    auto pass = tech->getPass(j);
                    const std::string sampler_name = kv.first;
                    auto prog = pass->getShader();
                    if (!prog) {
                        continue;
                    }
                    int slot = prog->getDefaultSamplerSlot(sampler_name.c_str());
                    if (slot == -1) {
                        continue;
                    }
                    SamplerOverride override{};
                    override.material_local_technique_id = i;
                    override.pass_id = j;
                    override.slot = slot;
                    override.texture_id = kv.second->getId();
                    compiled_sampler_overrides.push_back(override);
                }
            }
        }*/
    }

    void bindUniformBuffers() {
        for (int i = 0; i < uniform_buffers.size(); ++i) {
            auto& ub = uniform_buffers[i];
            GLint gl_id = ub->gpu_buf.getId();
            glBindBufferBase(GL_UNIFORM_BUFFER, ub->getDesc()->id, gl_id);
        }
    }

    void bindSamplerOverrides(int material_local_tech_id, int pass_id) {
        for (int i = 0; i < compiled_sampler_overrides.size(); ++i) {
            auto& ovr = compiled_sampler_overrides[i];
            if (ovr.material_local_technique_id != material_local_tech_id || ovr.pass_id != pass_id) {
                continue;
            }
            glActiveTexture(GL_TEXTURE0 + ovr.slot);
            glBindTexture(GL_TEXTURE_2D, ovr.texture_id);
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