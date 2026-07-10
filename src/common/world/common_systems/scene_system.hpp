#pragma once

#include "scene_system.auto.hpp"
#include <any>
#include <set>
#include <unordered_map>
#include "math/gfxm.hpp"
#include "transform_node/transform_node.hpp"
#include "gpu/render_bucket.hpp"
#include "gpu/scene_query_interface.hpp"


class SceneSystem;
[[cppi_class]];
class SceneProxy {
    friend SceneSystem;

    SceneSystem* sys = nullptr;
    int index = 0;
    gfxm::aabb bounding_box;
    float bounding_radius = .0f;
    gfxm::vec3 bounding_sphere_origin;
    void* user_ptr = nullptr;
    HTransform transform_node;
    TransformTicket* transform_ticket = nullptr;
public:
    virtual ~SceneProxy() {}

    virtual void updateBounds() = 0;
    virtual void submit(gpuRenderBucket*) = 0;

    void setTransformNode(HTransform node);
    HTransform getTransformNode() { return transform_node; }

    void setBoundingSphere(float radius, const gfxm::vec3& pos) {
        bounding_radius = radius;
        bounding_sphere_origin = pos;
    }
    void setBoundingBox(const gfxm::aabb& box) {
        bounding_box = box;
    }
    void markDirty();

    const gfxm::vec3& getBoundingSphereOrigin() const { return bounding_sphere_origin; }
    const gfxm::aabb& getBoundingBox() const { return bounding_box; }
    float getBoundingRadius() const { return bounding_radius; }

    void _setUserPtr(void* ptr) { user_ptr = ptr; }
    void* _getUserPtr() { return user_ptr; }
};

struct VisibilityProxyItem {
    SceneProxy* proxy = nullptr;
    //std::unique_ptr<VisProviderProxy> provider_internal;
};

class IVisibilityProvider {
public:
    virtual ~IVisibilityProvider() {}
    virtual void onAddProxy(VisibilityProxyItem*) = 0;
    virtual void onRemoveProxy(VisibilityProxyItem*) = 0;
    virtual void updateProxies(VisibilityProxyItem* items, int count) = 0;
    virtual void collectVisible(const VisibilityQuery& query, gpuRenderBucket* bucket) = 0;
};

[[cppi_class]];
class SceneSystem : public gpuSceneQueryInterface {
    IVisibilityProvider* provider = nullptr;
    std::unordered_map<type, std::any> query_handlers;

    std::vector<VisibilityProxyItem> proxies;
    int dirty_count = 0;
    TransformDirtyList_T<SceneProxy> transform_dirty_list;

    template<typename QUERY_T>
    bool dispatchQuery(const QUERY_T& q) {
        static type t = type_get<QUERY_T>();
        auto it = query_handlers.find(t);
        if (it == query_handlers.end()) {
            return false;
        }
        std::any_cast<std::function<void(const GeometryQuery&)>>(it->second)(q);
        return true;
    }
public:
    SceneSystem() {}
    SceneSystem(SceneSystem&) = delete;
    ~SceneSystem();

    SceneSystem& operator=(SceneSystem&) = delete;

    void addProxy(SceneProxy* prox);
    void removeProxy(SceneProxy* prox);
    void markDirty(SceneProxy*);

    void registerProvider(IVisibilityProvider* prov) {
        provider = prov;
    }
    void unregisterProvider(IVisibilityProvider* prov) {
        if (prov != provider) {
            assert(false);
            return;
        }
        provider = nullptr;
    }

    template<typename QUERY_T>
    void registerQueryHandler(std::function<void(const QUERY_T&)> h);
    void clearQueryHandlers() { query_handlers.clear(); }

    void updateProxies() {
        if (!provider) {
            return;
        }
        if (proxies.size() == 0) {
            return;
        }

        // We don't do updateBounds() in this loop
        // because transform_dirty_list is not the only source of change for proxies
        for (int i = 0; i < transform_dirty_list.dirtyCount(); ++i) {
            auto d = transform_dirty_list.getDirty(i);
            auto prox = static_cast<SceneProxy*>(d->user_ptr);
            prox->markDirty();
        }
        transform_dirty_list.clearDirty();

        for (int i = 0; i < dirty_count; ++i) {
            auto prox = proxies[i].proxy;
            prox->updateBounds();
        }

        provider->updateProxies(&proxies[0], dirty_count);
        dirty_count = 0;
    }

    void queryGeometry(const GeometryQuery& q) override {
        if (dispatchQuery(q)) {
            return;
        }
        // fallback
        for (int i = 0; i < proxies.size(); ++i) {
            auto prox = proxies[i].proxy;
            prox->submit(q.bucket);
        }
    }

    void _replaceTransformNode(SceneProxy* prox, HTransform node);
};

template<typename QUERY_T>
void SceneSystem::registerQueryHandler(std::function<void(const QUERY_T&)> h) {
    query_handlers.insert(std::make_pair( type_get<QUERY_T>(), h ));
}

