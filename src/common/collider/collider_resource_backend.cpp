#include "collider_resource_backend.hpp"

#include "resource_manager/resource_entry.hpp"

#include "sphere_collider.hpp"
#include "box_collider.hpp"
#include "triangle_mesh_collider.hpp"
#include "convex_mesh_collider.hpp"


#include <stdint.h>

constexpr uint32_t SHP_TAG = 'S' | ('H' << 8) | ('P' << 16) | ('\0' << 24);
constexpr uint32_t SHP_VERSION = 1;

#pragma pack(push, 1)
struct SHP_HEAD {
    uint32_t tag = 0;
    uint32_t version = 0;
};
#pragma pack(pop)



ColliderResourceBackend::ColliderResourceBackend() {
    registerFactory<SphereCollider>([]()->Resource* {
        return new SphereCollider;
    });
    registerFactory<BoxCollider>([]()->Resource* {
        return new BoxCollider;
    });
    registerFactory<TriangleMeshCollider>([]()->Resource* {
        return new TriangleMeshCollider;
    });
    registerFactory<ConvexMeshCollider>([]()->Resource* {
        return new ConvexMeshCollider;
    });
}
ColliderResourceBackend::~ColliderResourceBackend() {

}

ResourceEntry* ColliderResourceBackend::findEntry(const std::string& resource_id) {
    auto it = entries.find(resource_id);
    if (it == entries.end()) {
        return nullptr;
    }
    return it->second.get();
}
ResourceEntry* ColliderResourceBackend::createEntry(const std::string& resource_id) {
    auto it = entries.find(resource_id);
    if (it != entries.end()) {
        assert(false);
        return nullptr;
    }
    it = entries.insert(
        std::make_pair(
            resource_id,
            std::unique_ptr<ResourceEntry>(new TResourceEntry<Collider>())
        )
    ).first;
    return it->second.get();
}
eResourceLoadResult ColliderResourceBackend::load(ResourceEntry* entry) {
    auto& in = *entry->reader.get();
    auto view = in.try_slurp();
    if (!view) {
        return eResourceLoadResult::Failed;
    }

    SHP_HEAD head = {};
    in.read(&head);
    if (head.tag != SHP_TAG) {
        LOG_ERR("Not a shape file");
        assert(false);
        return eResourceLoadResult::Failed;
    }

    // TODO
    //entry->data = ?
    //entry->exact_type = rtti::type_get<ColliderType>();
    return eResourceLoadResult::Failed;
}
void ColliderResourceBackend::release(Resource* ptr) {
    delete static_cast<Collider*>(ptr);
}
void ColliderResourceBackend::collectGarbage() {
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