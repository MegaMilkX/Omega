#include "gizmo.hpp"

#include "gpu/render_bucket.hpp"

struct GizmoVertex {
    gfxm::vec3 position;
    uint32_t color;
};

struct GizmoContext {
    std::vector<GizmoVertex> vertices;
    std::vector<uint32_t> indices;
    gpuBuffer buffer;
    gpuBuffer index_buffer;
    gpuMeshDesc mesh_desc;
    RHSHARED<gpuMaterial> material;
    std::unique_ptr<gpuGeometryRenderable> renderable;
};


GizmoContext*   gizmoCreateContext() {
    auto ctx = new GizmoContext;

    ctx->material = resGet<gpuMaterial>("materials/gizmo.mat");
    
    ctx->mesh_desc.setAttribArray(
        VFMT::Position_GUID, &ctx->buffer, sizeof(GizmoVertex), 0
    );
    ctx->mesh_desc.setAttribArray(
        VFMT::ColorRGBA_GUID, &ctx->buffer, sizeof(GizmoVertex), offsetof(GizmoVertex, GizmoVertex::color)
    );
    ctx->mesh_desc.setIndexArray(&ctx->index_buffer);
    ctx->mesh_desc.setDrawMode(MESH_DRAW_MODE::MESH_DRAW_TRIANGLES);


    ctx->renderable.reset(new gpuGeometryRenderable(
        ctx->material.get(), &ctx->mesh_desc
    ));

    return ctx;
}
void            gizmoReleaseContext(GizmoContext* ctx) {
    delete ctx;
}


void gizmoPushDrawCommands(GizmoContext* ctx, gpuRenderBucket* bucket) {
    if (ctx->vertices.empty() || ctx->indices.empty()) {
        return;
    }

    ctx->buffer.setArrayData(&ctx->vertices[0], ctx->vertices.size() * sizeof(ctx->vertices[0]));
    ctx->index_buffer.setArrayData(&ctx->indices[0], ctx->indices.size() * sizeof(ctx->indices[0]));
    ctx->mesh_desc.setIndexCount(ctx->indices.size());

    ctx->renderable.reset(new gpuGeometryRenderable(ctx->material.get(), &ctx->mesh_desc));
    ctx->renderable->setTransform(gfxm::mat4(1.f));

    bucket->add(ctx->renderable.get());
}

void gizmoClearContext(GizmoContext* ctx) {
    ctx->vertices.clear();
    ctx->indices.clear();
}


void gizmoLine(GizmoContext* ctx, const gfxm::vec3& A, const gfxm::vec3& B, float thickness, GIZMO_COLOR color) {
    gfxm::vec3 UP = gfxm::vec3(0, 1, 0);

    if (gfxm::abs(gfxm::dot(UP, gfxm::normalize(B - A))) >= .99f) {
        UP = gfxm::vec3(1, 0, 0);
    }

    gfxm::vec3 Z = gfxm::normalize(gfxm::cross(B - A, UP));
    gfxm::vec3 Y = gfxm::normalize(gfxm::cross(Z, B - A));
    gfxm::vec3 X = gfxm::normalize(B - A);
    
    uint32_t i = ctx->vertices.size();

    const float half_thickness = thickness * .5f;
    GizmoVertex vertices[] = {
        { A + Z * half_thickness, color },
        { A + Y * half_thickness, color },
        { A - Z * half_thickness, color },
        { A - Y * half_thickness, color },
        { B + Z * half_thickness, color },
        { B + Y * half_thickness, color },
        { B - Z * half_thickness, color },
        { B - Y * half_thickness, color },
    };
    
    uint32_t indices[] = {
        i + 0, i + 4, i + 1,
        i + 4, i + 5, i + 1,

        i + 0, i + 3, i + 4,
        i + 3, i + 7, i + 4,

        i + 2, i + 1, i + 5,
        i + 5, i + 6, i + 2,

        i + 3, i + 2, i + 7,
        i + 7, i + 2, i + 6,
    };

    ctx->vertices.insert(ctx->vertices.end(), vertices, vertices + sizeof(vertices) / sizeof(vertices[0]));
    ctx->indices.insert(ctx->indices.end(), indices, indices + sizeof(indices) / sizeof(indices[0]));
}

void gizmoQuad(
    GizmoContext* ctx,
    const gfxm::vec3& A, const gfxm::vec3& B, const gfxm::vec3& C, const gfxm::vec3& D,
    GIZMO_COLOR color
) {
    uint32_t i = ctx->vertices.size();

    GizmoVertex vertices[] = {
        { A, color },
        { B, color },
        { D, color },
        { C, color },
    };

    uint32_t indices[] = {
        i + 0, i + 1, i + 2,
        i + 1, i + 3, i + 2,
    };

    ctx->vertices.insert(ctx->vertices.end(), vertices, vertices + sizeof(vertices) / sizeof(vertices[0]));
    ctx->indices.insert(ctx->indices.end(), indices, indices + sizeof(indices) / sizeof(indices[0]));
}

void gizmoAABB(GizmoContext* ctx, const gfxm::aabb& aabb, const gfxm::mat4& transform, GIZMO_COLOR color) {
    uint32_t i = ctx->vertices.size();

    gfxm::aabb box = aabb;
    if (box.from.x > box.to.x) {
        std::swap(box.from.x, box.to.x);
    }
    if (box.from.y > box.to.y) {
        std::swap(box.from.y, box.to.y);
    }
    if (box.from.z > box.to.z) {
        std::swap(box.from.z, box.to.z);
    }

    GizmoVertex vertices[] = {
        { gfxm::vec3(box.from.x, box.from.y, box.from.z), color },
        { gfxm::vec3(box.from.x, box.from.y, box.to.z), color },
        { gfxm::vec3(box.to.x, box.from.y, box.to.z), color },
        { gfxm::vec3(box.to.x, box.from.y, box.from.z), color },

        { gfxm::vec3(box.from.x, box.to.y, box.from.z), color },
        { gfxm::vec3(box.from.x, box.to.y, box.to.z), color },
        { gfxm::vec3(box.to.x, box.to.y, box.to.z), color },
        { gfxm::vec3(box.to.x, box.to.y, box.from.z), color },
    };
    for (int i = 0; i < sizeof(vertices) / sizeof(vertices[0]); ++i) {
        vertices[i].position = transform * gfxm::vec4(vertices[i].position, 1.f);
    }

    uint32_t indices[] = {
        i + 0, i + 5, i + 4,
        i + 5, i + 0, i + 1,

        i + 2, i + 7, i + 6,
        i + 3, i + 7, i + 2,
        
        i + 0, i + 3, i + 1,
        i + 1, i + 3, i + 2,

        i + 4, i + 5, i + 6,
        i + 6, i + 7, i + 4,
        
        i + 0, i + 4, i + 7,
        i + 7, i + 3, i + 0,

        i + 1, i + 6, i + 5,
        i + 1, i + 2, i + 6,
    };

    ctx->vertices.insert(ctx->vertices.end(), vertices, vertices + sizeof(vertices) / sizeof(vertices[0]));
    ctx->indices.insert(ctx->indices.end(), indices, indices + sizeof(indices) / sizeof(indices[0]));
}

void gizmoCylinder(GizmoContext* ctx, float radius, float height, int nsegments, const gfxm::mat4& transform, uint32_t color) {
    assert(nsegments >= 3);
    uint32_t base_index = ctx->vertices.size();

    const int N_VERTICES = nsegments * 2 + 2;
    std::vector<GizmoVertex> vertices(N_VERTICES);
    std::vector<uint32_t> indices(nsegments * 2 * 3 + nsegments * 2 * 3);

    for (int i = 0; i < nsegments; ++i) {
        float a = i / float(nsegments) * gfxm::pi * 2.f;
        float x = cosf(a) * radius;
        float z = sinf(a) * radius;
        vertices[i].position = gfxm::vec3(x, height < .0f ? height : .0f, z);
        vertices[i].color = color;
        vertices[i + nsegments].position = gfxm::vec3(x, height < .0f ? .0f : height, z);
        vertices[i + nsegments].color = color;
    }
    vertices[N_VERTICES - 2].position = gfxm::vec3(0, height < .0f ? height : .0f, 0);
    vertices[N_VERTICES - 2].color = color;
    vertices[N_VERTICES - 1].position = gfxm::vec3(0, height < .0f ? .0f : height, 0);
    vertices[N_VERTICES - 1].color = color;

    for (int i = 0; i < N_VERTICES; ++i) {
        vertices[i].position = transform * gfxm::vec4(vertices[i].position, 1.f);
    }

    for (int i = 0; i < nsegments; ++i) {
        indices[i * 6 + 0] = base_index + i + 0;
        indices[i * 6 + 1] = base_index + i + 0 + nsegments;
        indices[i * 6 + 2] = base_index + (i + 1) % nsegments;
        indices[i * 6 + 3] = base_index + (i + 1) % nsegments;
        indices[i * 6 + 4] = base_index + i + 0 + nsegments;
        indices[i * 6 + 5] = base_index + (i + 1) % nsegments + nsegments;
    }
    for (int i = 0; i < nsegments; ++i) {
        indices[nsegments * 2 * 3 + i * 3 + 0] = base_index + i + 0;
        indices[nsegments * 2 * 3 + i * 3 + 1] = base_index + (i + 1) % nsegments;
        indices[nsegments * 2 * 3 + i * 3 + 2] = base_index + N_VERTICES - 2;

        indices[nsegments * 2 * 3 + nsegments * 3 + i * 3 + 0] = base_index + i + 0 + nsegments;
        indices[nsegments * 2 * 3 + nsegments * 3 + i * 3 + 1] = base_index + N_VERTICES - 1;
        indices[nsegments * 2 * 3 + nsegments * 3 + i * 3 + 2] = base_index + (i + 1) % nsegments + nsegments;
    }

    ctx->vertices.insert(ctx->vertices.end(), vertices.begin(), vertices.end());
    ctx->indices.insert(ctx->indices.end(), indices.begin(), indices.end());
}

void gizmoCone(GizmoContext* ctx, const gfxm::mat4& transform, float radius, float height, GIZMO_COLOR color) {
    uint32_t i = ctx->vertices.size();

    GizmoVertex vertices[] = {
        { transform * gfxm::vec4(radius, 0, 0, 1), color },
        { transform * gfxm::vec4(0, 0, radius, 1), color },
        { transform * gfxm::vec4(-radius, 0, 0, 1), color },
        { transform * gfxm::vec4(0, 0, -radius, 1), color },
        { transform * gfxm::vec4(0, height, 0, 1), color },
    };
    uint32_t indices[] = {
        i + 0, i + 4, i + 1,
        i + 1, i + 4, i + 2,
        i + 2, i + 4, i + 3,
        i + 3, i + 4, i + 0
    };

    ctx->vertices.insert(ctx->vertices.end(), vertices, vertices + sizeof(vertices) / sizeof(vertices[0]));
    ctx->indices.insert(ctx->indices.end(), indices, indices + sizeof(indices) / sizeof(indices[0]));
}

void gizmoTorus(GizmoContext* ctx, const gfxm::mat4& transform, float radius, float inner_radius, GIZMO_COLOR color) {
    const int segments_a = 24;
    const int segments_b = 4;

    uint32_t base_index = ctx->vertices.size();
    GizmoVertex vertices[segments_a * segments_b];
    uint32_t indices[segments_a * segments_b * 6];
    for (int i = 0; i < segments_a; ++i) {
        float a = i / float(segments_a) * gfxm::pi * 2.f;
        float x = cosf(a) * radius;
        float z = sinf(a) * radius;
        gfxm::vec3 lcl_x_axis = gfxm::normalize(gfxm::vec3(x, .0f, z));

        for (int j = 0; j < segments_b; ++j) {
            float b = j / float(segments_b) * gfxm::pi * 2.f;
            float x_ = cosf(b) * inner_radius;
            float y_ = sinf(b) * inner_radius;

            gfxm::vec3 p = lcl_x_axis * radius + lcl_x_axis * x_ + gfxm::vec3(.0f, 1.f, .0f) * y_;
            p = transform * gfxm::vec4(p, 1.f);
            vertices[i * segments_b + j].position = p;
            vertices[i * segments_b + j].color = color;

            int vidx = i * segments_b + j;

            uint32_t ia = base_index + i * segments_b + j;
            uint32_t ib = base_index + i * segments_b + (j + 1) % segments_b;
            uint32_t ic = base_index + (ia + segments_b) % (segments_a * segments_b);
            uint32_t id = base_index + (ib + segments_b) % (segments_a * segments_b);
            indices[vidx * 6 + 0] = ia;
            indices[vidx * 6 + 1] = ic;
            indices[vidx * 6 + 2] = ib;
            indices[vidx * 6 + 3] = ic;
            indices[vidx * 6 + 4] = id;
            indices[vidx * 6 + 5] = ib;
        }
    }

    ctx->vertices.insert(ctx->vertices.end(), vertices, vertices + sizeof(vertices) / sizeof(vertices[0]));
    ctx->indices.insert(ctx->indices.end(), indices, indices + sizeof(indices) / sizeof(indices[0]));
}

void gizmoTranslate(GizmoContext* ctx, const GIZMO_TRANSFORM_STATE& state) {
    const float SHAFT_LEN = .7f;
    const float CONE_LEN = 1.f - SHAFT_LEN;

    uint32_t col_x = 0xFF6666FF;
    uint32_t col_y = 0xFF66FF66;
    uint32_t col_z = 0xFFFF6666;
    uint32_t col_xp = 0xAA6666FF;
    uint32_t col_yp = 0xAA66FF66;
    uint32_t col_zp = 0xAAFF6666;
    if (state.hovered_axis == 1) {
        col_x = 0xFF00FFFF;
    }
    if (state.hovered_axis == 2) {
        col_y = 0xFF00FFFF;
    }
    if (state.hovered_axis == 3) {
        col_z = 0xFF00FFFF;
    }
    if (state.hovered_axis == 4) {
        col_xp = 0xFF00FFFF;
    }
    if (state.hovered_axis == 5) {
        col_yp = 0xFF00FFFF;
    }
    if (state.hovered_axis == 6) {
        col_zp = 0xFF00FFFF;
    }
    
    const gfxm::mat4& proj = state.projection;
    const gfxm::mat4& view = state.view;
    const gfxm::mat4& model = state.transform;

    // Figure out scale modifier necessary to keep gizmo the same size on screen at any distance
    const float target_size = .2f; // screen ratio
    float scale = 1.f;
    {
        gfxm::vec4 ref4 = model[3];
        ref4 = proj * view * gfxm::vec4(ref4, 1.f);
        scale = target_size * ref4.w;
    }

    gizmoLine(ctx, model[3], model[3] + gfxm::normalize(model[0]) * scale * SHAFT_LEN, .05f * scale, col_x);
    gizmoLine(ctx, model[3], model[3] + gfxm::normalize(model[1]) * scale * SHAFT_LEN, .05f * scale, col_y);
    gizmoLine(ctx, model[3], model[3] + gfxm::normalize(model[2]) * scale * SHAFT_LEN, .05f * scale, col_z);

    gfxm::mat4 m
        = gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(SHAFT_LEN, .0f, .0f) * scale)
        * gfxm::to_mat4(gfxm::angle_axis(gfxm::radian(-90.0f), gfxm::vec3(.0f, .0f, 1.f)));
    gizmoCone(ctx, model * m, .1f * scale, CONE_LEN * scale, col_x);
    m = gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(.0f, SHAFT_LEN, .0f) * scale);
    gizmoCone(ctx, model * m, .1f * scale, CONE_LEN * scale, col_y);
    m = gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(.0f, .0f, SHAFT_LEN) * scale)
        * gfxm::to_mat4(gfxm::angle_axis(gfxm::radian(90.0f), gfxm::vec3(1.f, .0f, .0f)));
    gizmoCone(ctx, model * m, .1f * scale, CONE_LEN * scale, col_z);

    const float QUAD_SIDE = .25f * scale;

    gizmoQuad(
        ctx,
        model * gfxm::vec4(.0f, .0f, .0f, 1.f),
        model * gfxm::vec4(QUAD_SIDE, .0f, .0f, 1.f),
        model * gfxm::vec4(QUAD_SIDE, QUAD_SIDE, .0f, 1.f),
        model * gfxm::vec4(.0f, QUAD_SIDE, .0f, 1.f),
        col_zp
    );
    gizmoQuad(
        ctx,
        model * gfxm::vec4(.0f, .0f, .0f, 1.f),
        model * gfxm::vec4(.0f, .0f, QUAD_SIDE, 1.f),
        model * gfxm::vec4(QUAD_SIDE, .0f, QUAD_SIDE, 1.f),
        model * gfxm::vec4(QUAD_SIDE, .0f, .0f, 1.f),
        col_yp
    );
    gizmoQuad(
        ctx,
        model * gfxm::vec4(.0f, .0f, .0f, 1.f),
        model * gfxm::vec4(.0f, QUAD_SIDE, .0f, 1.f),
        model * gfxm::vec4(.0f, QUAD_SIDE, QUAD_SIDE, 1.f),
        model * gfxm::vec4(.0f, .0f, QUAD_SIDE, 1.f),
        col_xp
    );

}

void gizmoRotate(GizmoContext* ctx, const GIZMO_TRANSFORM_STATE& state) {
    uint32_t col_xr = 0xFF6666FF;
    uint32_t col_yr = 0xFF66FF66;
    uint32_t col_zr = 0xFFFF6666;
    uint32_t col_rr = 0xFFBBBBBB;
    if (state.hovered_axis == 1) {
        col_xr = 0xFF00FFFF;
    }
    if (state.hovered_axis == 2) {
        col_yr = 0xFF00FFFF;
    }
    if (state.hovered_axis == 3) {
        col_zr = 0xFF00FFFF;
    }
    if (state.hovered_axis == 4) {
        col_rr = 0xFF00FFFF;
    }

    const gfxm::mat4& proj = state.projection;
    const gfxm::mat4& view = state.view;
    const gfxm::mat4& model = state.transform;

    // Figure out scale modifier necessary to keep gizmo the same size on screen at any distance
    const float target_size = .2f; // screen ratio
    float scale = 1.f;
    {
        gfxm::vec4 ref4 = model[3];
        ref4 = proj * view * gfxm::vec4(ref4, 1.f);
        scale = target_size * ref4.w;
    }

    gizmoTorus(
        ctx,
        model * gfxm::to_mat4(gfxm::angle_axis(gfxm::radian(-90.0f), gfxm::vec3(.0f, .0f, 1.f))),
        1.f * scale, .025f * scale, col_xr
    );
    gizmoTorus(
        ctx,
        model,
        1.f * scale, .025f * scale, col_yr
    );
    gizmoTorus(
        ctx,
        model * gfxm::to_mat4(gfxm::angle_axis(gfxm::radian(90.0f), gfxm::vec3(1.f, .0f, .0f))),
        1.f * scale, .025f * scale, col_zr
    );

    gfxm::mat4 inv_view = gfxm::inverse(view);
    gfxm::mat3 orient;
    orient[2] = -inv_view[1];
    orient[1] = gfxm::normalize(gfxm::vec3(inv_view[3]) - gfxm::vec3(model[3]));
    orient[0] = gfxm::normalize(gfxm::cross(orient[2], orient[1]));
    orient[2] = gfxm::normalize(gfxm::cross(orient[1], orient[0]));
    gizmoTorus(
        ctx,
        model * gfxm::to_mat4(orient),
        1.1f * scale, .025f * scale, col_rr
    );
}