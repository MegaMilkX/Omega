#pragma once

#include "static_model.auto.hpp"

#include <vector>
#include "nlohmann/json.hpp"
#include "gpu/gpu_mesh.hpp"
#include "gpu/gpu_material.hpp"
#include "handle/hshared.hpp"

#include "static_model_instance.hpp"


struct StaticModelPart {
    RHSHARED<gpuMesh> mesh;
    int material_idx = 0;
};

[[cppi_class]];
class StaticModel {
    std::vector<StaticModelPart> parts;
    std::vector<RHSHARED<gpuMaterial>> materials;

public:
    TYPE_ENABLE();

    HSHARED<StaticModelInstance> createInstance();

    void addMesh(const StaticModelPart& part) {
        parts.push_back(part);
    }
    void addMaterial(const RHSHARED<gpuMaterial>& material) {
        materials.push_back(material);
    }

    int meshCount() const {
        return (int)parts.size();
    }
    const gpuMesh* getMesh(int i) const {
        return parts[i].mesh.get();
    }
    int getMaterialIndex(int i) const {
        return parts[i].material_idx;
    }

    int materialCount() const {
        return (int)materials.size();
    }
    gpuMaterial* getMaterial(int i) {
        return materials[i].get();
    }

    [[cppi_decl, serialize_json]]
    void toJson(nlohmann::json& j) {
        auto& jmeshes = j["meshes"];
        jmeshes = nlohmann::json::array();
        for (int i = 0; i < parts.size(); ++i) {
            auto jpart = nlohmann::json::object();
            serializeJson(jpart["mesh"], parts[i].mesh);
            serializeJson(jpart["material_idx"], parts[i].material_idx);
            jmeshes.push_back(jpart);
        }

        auto& jmaterials = j["materials"];
        for (int i = 0; i < materials.size(); ++i) {
            auto jmaterial = nlohmann::json::object();
            serializeJson(jmaterial, materials[i]);
            jmaterials.push_back(jmaterial);
        }
    }
    [[cppi_decl, deserialize_json]]
    bool fromJson(const nlohmann::json& j) {
        {
            auto it = j.find("meshes");
            if (it == j.end()) {
                assert(false);
                return false;
            }

            auto jmeshes = it.value();
            if (!jmeshes.is_array()) {
                assert(false);
                return false;
            }

            for (auto& jmesh : jmeshes) {
                if (!jmesh.is_object()) {
                    assert(false);
                    continue;
                }
                StaticModelPart part;
                deserializeJson(jmesh["mesh"], part.mesh);
                deserializeJson(jmesh["material_idx"], part.material_idx);
                parts.push_back(part);
            }
        }

        {
            auto it = j.find("materials");
            if (it == j.end()) {
                assert(false);
                return false;
            }

            auto jmaterials = it.value();
            if (!jmaterials.is_array()) {
                assert(false);
                return false;
            }

            for (auto& jmaterial : jmaterials) {
                RHSHARED<gpuMaterial> mat;
                deserializeJson(jmaterial, mat);
                materials.push_back(mat);
            }
        }

        return true;
    }
};

