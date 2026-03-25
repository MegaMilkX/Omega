#pragma once

#include "scene_system.auto.hpp"
#include <set>
#include "math/gfxm.hpp"
#include "transform_node/transform_node.hpp"
#include "gpu/render_bucket.hpp"


class SceneSystem;
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

struct VisibilityQuery {
    gfxm::frustum fru;
    int query_id;
    VisibilityQuery(const gfxm::mat4& proj, const gfxm::mat4& view, int id)
    : query_id(id) {
        fru = gfxm::make_frustum(proj, view);
    }
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
class SceneSystem {
    IVisibilityProvider* provider = nullptr;
    std::vector<VisibilityProxyItem> proxies;
    int dirty_count = 0;
    TransformDirtyList_T<SceneProxy> transform_dirty_list;
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
    void collectVisible(const VisibilityQuery& query, gpuRenderBucket* bucket) {
        if (!provider) {
            // Just submit everything as a fallback when there's no vis provider
            for (int i = 0; i < proxies.size(); ++i) {
                auto prox = proxies[i].proxy;
                prox->submit(bucket);
            }
            return;
        }
        provider->collectVisible(query, bucket);
    }

    void _replaceTransformNode(SceneProxy* prox, HTransform node);
};

