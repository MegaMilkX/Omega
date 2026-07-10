#pragma once

#include "scene_query_interface.auto.hpp"
#include "math/gfxm.hpp"
#include "gpu/render_bucket.hpp"


struct VisibilityQuery {
    gfxm::vec3 view_pos;
    gfxm::frustum fru;
    int query_id;
    VisibilityQuery()
    : query_id(0) {}
    VisibilityQuery(const gfxm::mat4& proj, const gfxm::mat4& view, int id)
    : query_id(id) {
        view_pos = gfxm::inverse(view)[3];
        fru = gfxm::make_frustum(proj, view);
    }
};

struct GeometryQuery {
    const VisibilityQuery query;
    mutable gpuRenderBucket* bucket = nullptr;

    GeometryQuery() {}
    GeometryQuery(const VisibilityQuery& q, gpuRenderBucket* bucket)
    : bucket(bucket), query(q) {}
};

[[cppi_class, no_reflect]]; // TODO: no_reflect does not work in this context for some reason
class gpuSceneQueryInterface {
public:
    virtual void queryGeometry(const GeometryQuery& q) = 0;
    // TODO: virtual void queryLight(const LightQuery& q) = 0;
    // TODO: virtual void queryEnvironment(const EnvironmentQuery& q) {}
};

