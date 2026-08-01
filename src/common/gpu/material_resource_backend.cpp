#include "material_resource_backend.hpp"

#include "json/json.hpp"

#include "gpu/gpu_material.hpp"
#include "gpu/material/pbr_material.hpp"
#include "gpu/material/terrain_material.hpp"
#include "gpu/material/water_material.hpp"


MaterialResourceBackend::MaterialResourceBackend() {
    registerFactory<gpuMaterial>([]()->void* {
        return new gpuMaterial();
    });
    registerFactory<PBRMaterial>([]()->void* {
        return static_cast<gpuMaterial*>(new PBRMaterial);
    });
    registerFactory<TerrainMaterial>([]()->void* {
        return static_cast<gpuMaterial*>(new TerrainMaterial);
    });
    registerFactory<WaterMaterial>([]()->void* {
        return static_cast<gpuMaterial*>(new WaterMaterial);
    });
}
ResourceEntry* MaterialResourceBackend::findEntry(const std::string& resource_id) {
    auto it = entries.find(resource_id);
    if (it == entries.end()) {
        return nullptr;
    }
    return it->second.get();
}

ResourceEntry* MaterialResourceBackend::createEntry(const std::string& resource_id) {
    auto it = entries.find(resource_id);
    if (it != entries.end()) {
        assert(false);
        return nullptr;
    }
    it = entries.insert(
        std::make_pair(
            resource_id,
            std::unique_ptr<ResourceEntry>(new TResourceEntry<gpuMaterial>())
        )
    ).first;
    return it->second.get();
}

void* MaterialResourceBackend::load(ResourceEntry* entry) {
    auto& in = *entry->reader.get();
    auto view = in.try_slurp();
    if (!view) {
        return nullptr;
    }

    std::string str(view.data, view.data + view.size);
    nlohmann::json json_ = nlohmann::json::parse(str);

    if (!json_.is_object()) {
        LOG_ERR("MaterialResourceBackend: json must be an object");
        assert(false);
        return nullptr;
    }

    nlohmann::json json = jsonPreprocessExtensions(json_);

    std::string type_name = json.value("@type", rtti::type_get<PBRMaterial>().get_name());

    rtti::type t = rtti::type_get(type_name.c_str());
    if (!t.is_valid()) {
        LOG_ERR("MaterialResourceBackend: unrecognized type '" << type_name << "'");
        assert(false);
        return nullptr;
    }

    gpuMaterial* mat = t.construct_new<gpuMaterial>();
    if (!mat) {
        LOG_ERR("MaterialResourceBackend: failed to create '" << type_name << "' object");
        assert(false);
        return nullptr;
    }

    mat->fromJson(json);
    return mat;
}

void MaterialResourceBackend::release(void* ptr) {
    delete static_cast<gpuMaterial*>(ptr);
}

void MaterialResourceBackend::collectGarbage() {
    for (auto& kv : entries) {
        auto entry = kv.second.get();
        if (entry->data != nullptr && entry->ref_count == 0) {
            entry->backend->release(entry->data);
            entry->data = nullptr;
            entry->state = eResourceUnloaded;
        }
    }
}

