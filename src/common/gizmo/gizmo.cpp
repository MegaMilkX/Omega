#include "gizmo.hpp"

#include "gpu/render_bucket.hpp"

#pragma pack(push, 1)
struct GizmoLineVertex {
    gfxm::vec3 position;
    float thickness;
    uint32_t color;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct GizmoTriVertex {
    gfxm::vec3 position;
    uint32_t color;
};
#pragma pack(pop)

template<typename VERTEX_T>
struct GizmoMesh {
    std::vector<VERTEX_T> vertices;
    std::vector<uint32_t> indices;
    gpuBuffer buffer;
    gpuBuffer index_buffer;
    gpuMeshDesc mesh_desc;
    RHSHARED<gpuMaterial> material;
    std::unique_ptr<gpuGeometryRenderable> renderable;
};

struct GizmoContext {
    GizmoMesh<GizmoLineVertex> lines;
    GizmoMesh<GizmoTriVertex> triangles;
    /*
    std::vector<GizmoVertex> vertices;
    std::vector<uint32_t> indices;
    gpuBuffer buffer;
    gpuBuffer index_buffer;
    gpuMeshDesc mesh_desc;
    RHSHARED<gpuMaterial> material;
    std::unique_ptr<gpuGeometryRenderable> renderable;*/
};


GizmoContext*   gizmoCreateContext() {
    auto ctx = new GizmoContext;

    {
        ctx->lines.material = resGet<gpuMaterial>("core/materials/gizmo_line.mat");    
        ctx->lines.mesh_desc.setAttribArray(
            VFMT::Position_GUID, &ctx->lines.buffer, sizeof(GizmoLineVertex), offsetof(GizmoLineVertex, GizmoLineVertex::position)
        );
        ctx->lines.mesh_desc.setAttribArray(
            VFMT::LineThickness_GUID, &ctx->lines.buffer, sizeof(GizmoLineVertex), offsetof(GizmoLineVertex, GizmoLineVertex::thickness)
        );
        ctx->lines.mesh_desc.setAttribArray(
            VFMT::ColorRGBA_GUID, &ctx->lines.buffer, sizeof(GizmoLineVertex), offsetof(GizmoLineVertex, GizmoLineVertex::color)
        );
        ctx->lines.mesh_desc.setIndexArray(&ctx->lines.index_buffer);
        ctx->lines.mesh_desc.setDrawMode(MESH_DRAW_MODE::MESH_DRAW_LINES);

        ctx->lines.renderable.reset(new gpuGeometryRenderable(
            ctx->lines.material.get(), &ctx->lines.mesh_desc
        ));
    }

    {
        ctx->triangles.material = resGet<gpuMaterial>("core/materials/gizmo_triangle.mat");
        ctx->triangles.mesh_desc.setAttribArray(
            VFMT::Position_GUID, &ctx->triangles.buffer, sizeof(GizmoTriVertex), offsetof(GizmoTriVertex, GizmoTriVertex::position)
        );
        ctx->triangles.mesh_desc.setAttribArray(
            VFMT::ColorRGBA_GUID, &ctx->triangles.buffer, sizeof(GizmoTriVertex), offsetof(GizmoTriVertex, GizmoTriVertex::color)
        );
        ctx->triangles.mesh_desc.setIndexArray(&ctx->triangles.index_buffer);
        ctx->triangles.mesh_desc.setDrawMode(MESH_DRAW_MODE::MESH_DRAW_TRIANGLES);

        ctx->triangles.renderable.reset(new gpuGeometryRenderable(
            ctx->triangles.material.get(), &ctx->triangles.mesh_desc
        ));
    }

    return ctx;
}
void            gizmoReleaseContext(GizmoContext* ctx) {
    delete ctx;
}


void gizmoPushDrawCommands(GizmoContext* ctx, gpuRenderBucket* bucket) {
    if (!ctx->lines.vertices.empty() && !ctx->lines.indices.empty()) {
        ctx->lines.buffer.setArrayData(&ctx->lines.vertices[0], ctx->lines.vertices.size() * sizeof(ctx->lines.vertices[0]));
        ctx->lines.index_buffer.setArrayData(&ctx->lines.indices[0], ctx->lines.indices.size() * sizeof(ctx->lines.indices[0]));
        ctx->lines.mesh_desc.setIndexCount(ctx->lines.indices.size());

        ctx->lines.renderable.reset(new gpuGeometryRenderable(ctx->lines.material.get(), &ctx->lines.mesh_desc));
        ctx->lines.renderable->setTransform(gfxm::mat4(1.f));

        bucket->add(ctx->lines.renderable.get());
    }

    if (!ctx->triangles.vertices.empty() && !ctx->triangles.indices.empty()) {
        ctx->triangles.buffer.setArrayData(&ctx->triangles.vertices[0], ctx->triangles.vertices.size() * sizeof(ctx->triangles.vertices[0]));
        ctx->triangles.index_buffer.setArrayData(&ctx->triangles.indices[0], ctx->triangles.indices.size() * sizeof(ctx->triangles.indices[0]));
        ctx->triangles.mesh_desc.setIndexCount(ctx->triangles.indices.size());

        ctx->triangles.renderable.reset(new gpuGeometryRenderable(ctx->triangles.material.get(), &ctx->triangles.mesh_desc));
        ctx->triangles.renderable->setTransform(gfxm::mat4(1.f));

        bucket->add(ctx->triangles.renderable.get());
    }
}

void gizmoClearContext(GizmoContext* ctx) {
    ctx->lines.vertices.clear();
    ctx->lines.indices.clear();
    ctx->triangles.vertices.clear();
    ctx->triangles.indices.clear();
}


void gizmoLine(GizmoContext* ctx, const gfxm::vec3& A, const gfxm::vec3& B, float thickness, GIZMO_COLOR color) {    
    uint32_t i = ctx->lines.vertices.size();

    GizmoLineVertex vertices[] = {
        { .position = A, .thickness = thickness, .color = color },
        { .position = B, .thickness = thickness, .color = color },
    };
    
    uint32_t indices[] = {
        i + 0, i + 1,
    };

    ctx->lines.vertices.insert(ctx->lines.vertices.end(), vertices, vertices + sizeof(vertices) / sizeof(vertices[0]));
    ctx->lines.indices.insert(ctx->lines.indices.end(), indices, indices + sizeof(indices) / sizeof(indices[0]));
}

void gizmoCircle(GizmoContext* ctx, const gfxm::mat4& transform, float radius, float thickness, GIZMO_COLOR color) {
    const int N_SEGMENTS = 32;

    uint32_t i = ctx->lines.vertices.size();

    GizmoLineVertex vertices[N_SEGMENTS];
    for(int j = 0; j < N_SEGMENTS; ++j) {
        const float a = j / float(N_SEGMENTS) * gfxm::pi * 2.f;
        gfxm::vec3 P = gfxm::vec3(cosf(a), .0f, sinf(a)) * radius;
        P = transform * gfxm::vec4(P, 1.f);
        vertices[j] = { .position = P, .thickness = thickness, .color = color };
    }
    uint32_t indices[N_SEGMENTS * 2];
    for (int j = 0; j < N_SEGMENTS; ++j) {
        indices[j * 2] = i + j;
        indices[j * 2 + 1] = i + (j + 1) % N_SEGMENTS;
    }

    ctx->lines.vertices.insert(ctx->lines.vertices.end(), vertices, vertices + sizeof(vertices) / sizeof(vertices[0]));
    ctx->lines.indices.insert(ctx->lines.indices.end(), indices, indices + sizeof(indices) / sizeof(indices[0]));
}

void gizmoQuad(
    GizmoContext* ctx,
    const gfxm::vec3& A, const gfxm::vec3& B, const gfxm::vec3& C, const gfxm::vec3& D,
    GIZMO_COLOR color
) {
    uint32_t i = ctx->triangles.vertices.size();

    GizmoTriVertex vertices[] = {
        { A, color },
        { B, color },
        { D, color },
        { C, color },
    };

    uint32_t indices[] = {
        i + 0, i + 1, i + 2,
        i + 1, i + 3, i + 2,
    };

    ctx->triangles.vertices.insert(ctx->triangles.vertices.end(), vertices, vertices + sizeof(vertices) / sizeof(vertices[0]));
    ctx->triangles.indices.insert(ctx->triangles.indices.end(), indices, indices + sizeof(indices) / sizeof(indices[0]));
}

void gizmoAABB(GizmoContext* ctx, const gfxm::aabb& aabb, const gfxm::mat4& transform, GIZMO_COLOR color) {
    uint32_t i = ctx->triangles.vertices.size();

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

    GizmoTriVertex vertices[] = {
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

    ctx->triangles.vertices.insert(ctx->triangles.vertices.end(), vertices, vertices + sizeof(vertices) / sizeof(vertices[0]));
    ctx->triangles.indices.insert(ctx->triangles.indices.end(), indices, indices + sizeof(indices) / sizeof(indices[0]));
}

void gizmoCylinder(GizmoContext* ctx, float radius, float height, int nsegments, const gfxm::mat4& transform, uint32_t color) {
    assert(nsegments >= 3);
    uint32_t base_index = ctx->triangles.vertices.size();

    const int N_VERTICES = nsegments * 2 + 2;
    std::vector<GizmoTriVertex> vertices(N_VERTICES);
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

    ctx->triangles.vertices.insert(ctx->triangles.vertices.end(), vertices.begin(), vertices.end());
    ctx->triangles.indices.insert(ctx->triangles.indices.end(), indices.begin(), indices.end());
}

void gizmoCone(GizmoContext* ctx, const gfxm::mat4& transform, float radius, float height, GIZMO_COLOR color) {
    uint32_t i = ctx->triangles.vertices.size();

    GizmoTriVertex vertices[] = {
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

    ctx->triangles.vertices.insert(ctx->triangles.vertices.end(), vertices, vertices + sizeof(vertices) / sizeof(vertices[0]));
    ctx->triangles.indices.insert(ctx->triangles.indices.end(), indices, indices + sizeof(indices) / sizeof(indices[0]));
}

void gizmoTorus(GizmoContext* ctx, const gfxm::mat4& transform, float radius, float inner_radius, GIZMO_COLOR color) {
    const int segments_a = 24;
    const int segments_b = 4;

    uint32_t base_index = ctx->triangles.vertices.size();
    GizmoTriVertex vertices[segments_a * segments_b];
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

    ctx->triangles.vertices.insert(ctx->triangles.vertices.end(), vertices, vertices + sizeof(vertices) / sizeof(vertices[0]));
    ctx->triangles.indices.insert(ctx->triangles.indices.end(), indices, indices + sizeof(indices) / sizeof(indices[0]));
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

    gizmoLine(ctx, model[3], model[3] + gfxm::normalize(model[0]) * scale * SHAFT_LEN, 4, col_x);
    gizmoLine(ctx, model[3], model[3] + gfxm::normalize(model[1]) * scale * SHAFT_LEN, 4, col_y);
    gizmoLine(ctx, model[3], model[3] + gfxm::normalize(model[2]) * scale * SHAFT_LEN, 4, col_z);

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

    const float LINE_THICKNESS = 6;

    gizmoCircle(
        ctx,
        model * gfxm::to_mat4(gfxm::angle_axis(gfxm::radian(-90.0f), gfxm::vec3(.0f, .0f, 1.f))),
        1.f * scale, LINE_THICKNESS, col_xr
    );
    gizmoCircle(
        ctx,
        model,
        1.f * scale, LINE_THICKNESS, col_yr
    );
    gizmoCircle(
        ctx,
        model * gfxm::to_mat4(gfxm::angle_axis(gfxm::radian(90.0f), gfxm::vec3(1.f, .0f, .0f))),
        1.f * scale, LINE_THICKNESS, col_zr
    );

    gfxm::mat4 inv_view = gfxm::inverse(view);
    gfxm::mat3 orient;
    orient[2] = -inv_view[1];
    orient[1] = gfxm::normalize(gfxm::vec3(inv_view[3]) - gfxm::vec3(model[3]));
    orient[0] = gfxm::normalize(gfxm::cross(orient[2], orient[1]));
    orient[2] = gfxm::normalize(gfxm::cross(orient[1], orient[0]));
    gizmoCircle(
        ctx,
        model * gfxm::to_mat4(orient),
        1.1f * scale, LINE_THICKNESS, col_rr
    );
}