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
