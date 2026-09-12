#include "material_resource_backend.hpp"

#include "json/json.hpp"

#include "gpu/gpu_material.hpp"
#include "gpu/material/pbr_material.hpp"
#include "gpu/material/vfx_material.hpp"
#include "gpu/material/terrain_material.hpp"
#include "gpu/material/water_material.hpp"


MaterialResourceBackend::MaterialResourceBackend() {
    registerFactory<gpuMaterial>([]()->Resource* {
        return new gpuMaterial();
    });
    registerFactory<PBRMaterial>([]()->Resource* {
        return new PBRMaterial;
    });
    registerFactory<VFXMaterial>([]()->Resource* {
        return new VFXMaterial;
    });
    registerFactory<TerrainMaterial>([]()->Resource* {
        return new TerrainMaterial;
    });
    registerFactory<WaterMaterial>([]()->Resource* {
        return new WaterMaterial;
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

eResourceLoadResult MaterialResourceBackend::load(ResourceEntry* entry) {
    auto& in = *entry->reader.get();
    auto view = in.try_slurp();
    if (!view) {
        return eResourceLoadResult::Failed;
    }

    std::string str(view.data, view.data + view.size);
    nlohmann::json json_ = nlohmann::json::parse(str);

    if (!json_.is_object()) {
        LOG_ERR("MaterialResourceBackend: json must be an object");
        assert(false);
        return eResourceLoadResult::Failed;
    }

    nlohmann::json json = jsonPreprocessExtensions(json_);

    std::string type_name = json.value("@type", rtti::type_get<PBRMaterial>().get_name());

    rtti::type t = rtti::type_get(type_name.c_str());
    if (!t.is_valid()) {
        LOG_ERR("MaterialResourceBackend: unrecognized type '" << type_name << "'");
        assert(false);
        return eResourceLoadResult::Failed;
    }

    gpuMaterial* mat = t.construct_new<gpuMaterial>();
    if (!mat) {
        LOG_ERR("MaterialResourceBackend: failed to create '" << type_name << "' object");
        assert(false);
        return eResourceLoadResult::Failed;
    }

    rtti::PropSnapshot schema;
    mat->makeSnapshot(schema);
    rtti::PropSnapshot snap;
    snap.fromJson(schema, json);
    mat->applySnapshot(snap);

    entry->data = mat;
    entry->exact_type = t;
    return eResourceLoadResult::Done;
}

void MaterialResourceBackend::release(Resource* ptr) {
    delete static_cast<gpuMaterial*>(ptr);
}

void MaterialResourceBackend::collectGarbage() {
    for (auto& kv : entries) {
        auto entry = kv.second.get();
        if (entry->data != nullptr && entry->ref_count == 0) {
            entry->backend->release(entry->data);
            entry->data = nullptr;
            entry->state = eResourceUnloaded;
            int new_version = entry->version + 1;
            entry->version.store(new_version, std::memory_order_release);
            entry->cast_cache.store(0x0, std::memory_order_release);
            entry->cast_mask.store(0x0, std::memory_order_release);
        }
    }
}

