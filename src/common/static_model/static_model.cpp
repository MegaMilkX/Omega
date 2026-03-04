#include "static_model.hpp"
#include "static_model_instance.hpp"


HSHARED<StaticModelInstance> StaticModel::createInstance() {
    HSHARED<StaticModelInstance> handle(HANDLE_MGR<StaticModelInstance>::acquire());

    *handle = std::move(StaticModelInstance(this));

    return handle;
}


bool StaticModel::load(byte_reader& reader) {
    auto view = reader.try_slurp();
    if (!view) {
        return false;
    }
    std::string str_json(view.data, view.data + view.size);
    nlohmann::json j = nlohmann::json::parse(str_json);
    if (!j.is_object()) {
        return false;
    }

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