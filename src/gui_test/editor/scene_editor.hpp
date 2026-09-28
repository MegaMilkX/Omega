#pragma once

#include "editor_window.hpp"
#include "gui/elements/viewport/gui_viewport.hpp"
#include "gui/elements/viewport/tools/gui_viewport_tool_transform.hpp"
#include "gui_engine/actor_inspector.hpp"

struct SceneEntry {
    ResourceRef<ActorPrefab> prefab;
    std::unique_ptr<Actor> instance;
};

struct SceneEditorContext {

};

class SceneSpace : public gpuSceneQueryInterface, public GuiViewportToolBase {
public:
    SceneSpace(const char* name)
        : GuiViewportToolBase(name) {}
    virtual ~SceneSpace() {}

    virtual void enterUi(SceneEditorContext& ctx) = 0;

    virtual void toJson(nlohmann::json& json) const = 0;
    virtual bool fromJson(const nlohmann::json& json) = 0;
};


struct TerrainCell {
    std::vector<float> points;
    float y_min = .0f;
    float y_max = .0f;
    // Preview
    gpuMesh mesh;
    gpuRenderable renderable;
    gpuTransformBlock* transform_block = nullptr;

    TerrainCell() = default;
    TerrainCell(const TerrainCell&) = delete;
    TerrainCell(TerrainCell&&) noexcept = default;
    TerrainCell& operator=(const TerrainCell&) = delete;
    TerrainCell& operator=(TerrainCell&&) noexcept = default;
};

class TerrainSampler {
    TerrainCell* cells = nullptr;
    int terrain_width, terrain_depth, cell_segments_x, cell_segments_z;
public:
    TerrainSampler() = default;
    TerrainSampler(TerrainCell* cells, int terrain_width, int terrain_depth, int cell_segments_x, int cell_segments_z)
        : cells(cells)
        , terrain_width(terrain_width)
        , terrain_depth(terrain_depth)
        , cell_segments_x(cell_segments_x)
        , cell_segments_z(cell_segments_z)
    {}

    float sampleQuadSpace(int icell, int x, int z) {
        int icellx = icell % terrain_width;
        int icellz = icell / terrain_width;

        while (x >= cell_segments_x && icellx < terrain_width - 1) {
            x -= cell_segments_x;
            icellx += 1;
        }
        while (z >= cell_segments_z && icellz < terrain_depth - 1) {
            z -= cell_segments_z;
            icellz += 1;
        }

        x = gfxm::_min(x, cell_segments_x - 1);
        z = gfxm::_min(z, cell_segments_z - 1);
        return cells[icellx + icellz * terrain_width].points[x + z * cell_segments_x];
    }
};

struct TerrainBrushContext {
    float* region;
    uint8_t* mask;
    int width;
    int height;
    float radius;
    float strength;
};

class BrushTip {
public:
    virtual ~BrushTip() {}
    virtual void rasterize(uint8_t* out, int width, int height, const gfxm::vec2& center_fract, float angle) = 0;
};
class RadialBrushTip : public BrushTip {
public:
    void rasterize(uint8_t* out, int width, int height, const gfxm::vec2& center_fract, float angle) override {
        for (int y = 0; y < height; ++y) {
            float fy = float(y - center_fract.y) / (height - 1) * 2.f - 1.f;
            for (int x = 0; x < width; ++x) {
                float fx = float(x - center_fract.x) / (width - 1) * 2.f - 1.f;
                float dist2 = fx * fx + fy * fy;
                float dist = gfxm::sqrt(dist2);
                float f = gfxm::_max(.0f, 1.f - dist);
                out[x + y * width] = uint8_t(f * 255.f);
            }
        }
    }
};

class TerrainBrush {
public:
    virtual ~TerrainBrush() {}
    virtual void apply(TerrainBrushContext&) = 0;
};

class DrawTerrainBrush : public TerrainBrush {
public:
    void apply(TerrainBrushContext& ctx) override {        
        for (int i = 0; i < ctx.width * ctx.height; ++i) {
            ctx.region[i] += ctx.strength * (ctx.mask[i] / 255.f);
        }
    }
};

class SmoothTerrainBrush : public TerrainBrush {
    std::vector<float> src;
public:
    void apply(TerrainBrushContext& ctx) override {
        const int w = ctx.width;
        const int h = ctx.height;
        src.assign(ctx.region, ctx.region + w * h); //c++20
        
        bool invert = ctx.strength < .0f;
        const float k = gfxm::clamp(fabsf(ctx.strength), .0f, 1.f);

        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                const int i = x + y * w;
                const uint8_t m = ctx.mask[i];
                if(!m) continue;

                float sum = .0f;
                for (int dy = -1; dy <= 1; ++dy) {
                    const int yy = gfxm::iclamp(y + dy, 0, h - 1);
                    for (int dx = -1; dx <= 1; ++dx) {
                        const int xx = gfxm::iclamp(x + dx, 0, w - 1);
                        sum += src[xx + yy * w];
                    }
                }
                const float avg = sum * (1.f / 9.f);
                if(!invert) {
                    ctx.region[i] += (avg - src[i]) * (m / 255.f) * k;
                } else {
                    ctx.region[i] -= (avg - src[i]) * (m / 255.f) * k;
                }
            }
        }
    }
};

class FlattenTerrainBrush : public TerrainBrush {
public:
    void apply(TerrainBrushContext& ctx) override {
        const int w = ctx.width;
        const int h = ctx.height;
        const float k = gfxm::clamp(ctx.strength, .0f, 1.f);

        // Weighted centroid
        float W = .0f;
        float sx = .0f;
        float sz = .0f;
        float sh = .0f;
        for (int z = 0; z < h; ++z) {
            for (int x = 0; x < w; ++x) {
                const int i = x + z * w;
                const float wt = ctx.mask[i] / 255.f;
                W += wt;
                sx += wt * x;
                sz += wt * z;
                sh += wt * ctx.region[i];
            }
        }
        if (W < 1e-4f) {
            return;
        }
        const float mx = sx / W;
        const float mz = sz / W;
        const float mh = sh / W;

        //
        float Sxx = .0f;
        float Szz = .0f;
        float Sxz = .0f;
        float Sxh = .0f;
        float Szh = .0f;
        for (int z = 0; z < h; ++z) {
            for (int x = 0; x < w; ++x) {
                const int i = x + z * w;
                const float wt = ctx.mask[i] / 255.f;
                const float dx = x - mx;
                const float dz = z - mz;
                const float dh = ctx.region[i] - mh;
                Sxx += wt * dx * dx;
                Szz += wt * dz * dz;
                Sxz += wt * dx * dz;
                Sxh += wt * dx * dh;
                Szh += wt * dz * dh;
            }
        }

        //
        float a = .0f;
        float b = .0f;
        const float det = Sxx * Szz - Sxz * Sxz;
        if (det > 1e-6f * Sxx * Szz) {
            a = (Sxh * Szz - Szh * Sxz) / det;
            b = (Szh * Sxx - Sxh * Sxz) / det;
        }

        // Pull toward the plane
        for (int z = 0; z < h; ++z) {
            for (int x = 0; x < w; ++x) {
                const int i = x + z * w;
                const float plane = mh + a * (x - mx) + b * (z - mz);
                ctx.region[i] += (plane - ctx.region[i]) * (ctx.mask[i] / 255.f) * k;
            }
        }
    }
};

inline bool clipSegmentXZ(
    const gfxm::vec3& A, const gfxm::vec3& B, const gfxm::aabb& box,
    gfxm::vec3& outA, gfxm::vec3& outB
) {
    float t0 = .0f;
    float t1 = 1.f;

    const float dx = B.x - A.x;
    const float dz = B.z - A.z;

    auto fn_clip_axis = [&](float p, float q) ->bool {
        if (p == .0f) {
            if (q < .0f) {
                return false;
            }
        } else {
            const float r = q / p;
            if (p < .0f) {
                if (r > t1) {
                    return false;
                }
                if (r > t0) {
                    t0 = r;
                }
            } else {
                if (r < t0) {
                    return false;
                }
                if (r < t1) {
                    t1 = r;
                }
            }
        }
        return true;
    };

    if (!fn_clip_axis(-dx, A.x - box.from.x)) {
        return false;
    }
    if (!fn_clip_axis( dx, box.to.x - A.x)) {
        return false;
    }
    if (!fn_clip_axis(-dz, A.z - box.from.z)) {
        return false;
    }
    if (!fn_clip_axis( dz, box.to.z - A.z)) {
        return false;
    }
    
    if(t0 > t1) return false;

    outA = A + (B - A) * t0;
    outB = A + (B - A) * t1;
    return true;
}

#pragma pack(push, 1)
struct COLOR24 {
    uint8_t R;
    uint8_t G;
    uint8_t B;
    COLOR24& operator=(const uint32_t& other) {
        R = other & 0xFF;
        G = (other & 0xFF00) >> 8;
        B = (other & 0xFF0000) >> 16;
        return *this;
    }
};
#pragma pack(pop)
constexpr static int TERRAIN_SPACE_VERSION = 1;
class TerrainSceneSpace : public SceneSpace {
    // TESTING
    std::unique_ptr<gpuDecalRenderable> renderable2;
    ResourceRef<gpuMaterial> material2;

    // Terrain data
    constexpr static int WIDTH = 10;
    constexpr static int DEPTH = 10;
    constexpr static int CELL_SEGMENTS_X = 200;
    constexpr static int CELL_SEGMENTS_Z = 200;
    constexpr static float CELL_WIDTH = 204.8f;
    constexpr static float CELL_DEPTH = 204.8f;

    std::vector<TerrainCell> cells;

    // Preview
    ResourceRef<gpuMaterial> terrain_material = loadResource<gpuMaterial>("materials/terrain");
    std::set<int> dirty_cells;

    //
    std::unique_ptr<TerrainBrush> current_brush;
    std::unique_ptr<TerrainBrush> secondary_brush;
    std::unique_ptr<BrushTip> brush_tip;
    gfxm::vec3 brush_pos;
    bool brush_hit = false;
    float brush_scale = .25f;
    float brush_radius = 10.f;
    constexpr static float MIN_BRUSH_RADIUS = .5f;
    constexpr static float MAX_BRUSH_RADIUS = 200.f;

    void updateCell(int icell) {
        TerrainCell* cell = &cells[icell];
        cell->y_min = cell->points[0];
        cell->y_max = cell->y_min;
        for (int i = 1; i < cell->points.size(); ++i) {
            cell->y_min = gfxm::_min(cell->y_min, cell->points[i]);
            cell->y_max = gfxm::_max(cell->y_max, cell->points[i]);
        }

        updateCellPreview(icell);
    }

    void updateCellPreview(int i) {
        TerrainCell* cell = &cells[i];

        const float CELL_X = (i % WIDTH) * CELL_WIDTH;
        const float CELL_Z = (i / WIDTH) * CELL_DEPTH;

        const auto& points = cell->points;
        auto& mesh = cell->mesh;
        auto& renderable = cell->renderable;

        std::vector<gfxm::vec3> vertices;
        std::vector<gfxm::vec3> normals;
        std::vector<gfxm::vec3> tangents;
        std::vector<gfxm::vec3> bitangents;
        std::vector<gfxm::vec2> uvs;
        std::vector<COLOR24> colors;
        std::vector<uint32_t> indices;
        {
            const float QUAD_WIDTH = CELL_WIDTH / CELL_SEGMENTS_X;
            const float QUAD_DEPTH = CELL_DEPTH / CELL_SEGMENTS_Z;
            vertices.resize((CELL_SEGMENTS_X + 1) * (CELL_SEGMENTS_Z + 1));
            for (int y = 0; y <= CELL_SEGMENTS_Z; ++y) {
                for (int x = 0; x <= CELL_SEGMENTS_X; ++x) {
                    float h = sampleHeightQuadSpace(i, x, y);
                    vertices[x + y * (CELL_SEGMENTS_X + 1)] = gfxm::vec3(x * QUAD_WIDTH, h, y * QUAD_DEPTH);
                }
            }

            uvs.resize(vertices.size());
            for (int y = 0; y <= CELL_SEGMENTS_Z; ++y) {
                for (int x = 0; x <= CELL_SEGMENTS_X; ++x) {
                    uvs[x + y * (CELL_SEGMENTS_X + 1)] = gfxm::vec2(x * QUAD_WIDTH * .25f, y * QUAD_DEPTH * .25f);
                }
            }

            gfxm::vec3 gradient[4] = {
                gfxm::vec3(.2f, .3f, .9f),
                gfxm::vec3(.05f, .3f, .12f),
                gfxm::vec3(.2f, .2f, .2f),
                gfxm::vec3(1.f, 1.f, 1.f)
            };
            int grad_count = sizeof(gradient) / sizeof(gradient[0]);
            colors.resize(vertices.size());
            for (int y = 0; y <= CELL_SEGMENTS_Z; ++y) {
                for (int x = 0; x <= CELL_SEGMENTS_X; ++x) {
                    float h = sampleHeightQuadSpace(i, x, y);;

                    //colors[x + y * CELL_SEGMENTS_X] = gfxm::make_rgba32(h, h, h, 1.f);
                    colors[x + y * (CELL_SEGMENTS_X + 1)] = gfxm::make_rgba32(1, 1, 1, 1.f);
                }
            }

            indices.resize(CELL_SEGMENTS_X * CELL_SEGMENTS_Z * 6);
            for (int y = 0; y < CELL_SEGMENTS_Z; ++y) {
                for (int x = 0; x < CELL_SEGMENTS_X; ++x) {
                    int at = 6 * (x + y * CELL_SEGMENTS_X);
                    int i0 = x + y * (CELL_SEGMENTS_X + 1);
                    int i1 = x + 1 + y * (CELL_SEGMENTS_X + 1);
                    int i2 = x + 1 + (y + 1) * (CELL_SEGMENTS_X + 1);
                    int i3 = x + (y + 1) * (CELL_SEGMENTS_X + 1);
                    indices[at + 0] = i0;
                    indices[at + 2] = i1;
                    indices[at + 1] = i2;
                    indices[at + 3] = i2;
                    indices[at + 5] = i3;
                    indices[at + 4] = i0;
                }
            }

            normals.resize(vertices.size());
            tangents.resize(vertices.size());
            bitangents.resize(vertices.size());
            for (int i = 0; i < indices.size() / 3; ++i) {
                int a = indices[i * 3];
                int b = indices[i * 3 + 1];
                int c = indices[i * 3 + 2];

                gfxm::vec3 N = gfxm::normalize(gfxm::cross(vertices[b] - vertices[a], vertices[c] - vertices[a]));
                normals[a] = gfxm::normalize(normals[a] + N);
                normals[b] = gfxm::normalize(normals[b] + N);
                normals[c] = gfxm::normalize(normals[c] + N);
            }

            {
                const int index_count = indices.size();
                const int vertex_count = vertices.size();
                for (int l = 0; l < index_count; l += 3) {
                    uint32_t a = indices[l];
                    uint32_t b = indices[l + 1];
                    uint32_t c = indices[l + 2];

                    gfxm::vec3 Va = vertices[a];
                    gfxm::vec3 Vb = vertices[b];
                    gfxm::vec3 Vc = vertices[c];
                    gfxm::vec3 Na = normals[a];
                    gfxm::vec3 Nb = normals[b];
                    gfxm::vec3 Nc = normals[c];
                    gfxm::vec2 UVa = uvs[a];
                    gfxm::vec2 UVb = uvs[b];
                    gfxm::vec2 UVc = uvs[c];

                    float x1 = Vb.x - Va.x;
                    float x2 = Vc.x - Va.x;
                    float y1 = Vb.y - Va.y;
                    float y2 = Vc.y - Va.y;
                    float z1 = Vb.z - Va.z;
                    float z2 = Vc.z - Va.z;

                    float s1 = UVb.x - UVa.x;
                    float s2 = UVc.x - UVa.x;
                    float t1 = UVb.y - UVa.y;
                    float t2 = UVc.y - UVa.y;

                    float r = 1.f / (s1 * t2 - s2 * t1);
                    gfxm::vec3 sdir(
                        (t2 * x1 - t1 * x2) * r,
                        (t2 * y1 - t1 * y2) * r,
                        (t2 * z1 - t1 * z2) * r
                    );
                    gfxm::vec3 tdir(
                        (s1 * x2 - s2 * x1) * r,
                        (s1 * y2 - s2 * y1) * r,
                        (s1 * z2 - s2 * z1) * r
                    );

                    tangents[a] += sdir;
                    tangents[b] += sdir;
                    tangents[c] += sdir;
                    bitangents[a] += tdir;
                    bitangents[b] += tdir;
                    bitangents[c] += tdir;
                }
                for (int k = 0; k < vertex_count; ++k) {
                    tangents[k] = gfxm::normalize(tangents[k]);
                    bitangents[k] = gfxm::normalize(bitangents[k]);
                }
            }
        }

        Mesh3d mesh3d;
        mesh3d.setAttribArray(VFMT::Position_GUID, vertices.data(), vertices.size() * sizeof(vertices[0]));
        mesh3d.setAttribArray(VFMT::Normal_GUID, normals.data(), normals.size() * sizeof(normals[0]));
        mesh3d.setAttribArray(VFMT::Tangent_GUID, tangents.data(), tangents.size() * sizeof(tangents[0]));
        mesh3d.setAttribArray(VFMT::Bitangent_GUID, bitangents.data(), bitangents.size() * sizeof(bitangents[0]));
        mesh3d.setAttribArray(VFMT::UV_GUID, uvs.data(), uvs.size() * sizeof(uvs[0]));
        mesh3d.setAttribArray(VFMT::ColorRGB_GUID, colors.data(), colors.size() * sizeof(colors[0]));
        mesh3d.setIndexArray(indices.data(), indices.size() * sizeof(indices[0]));
        mesh.setData(&mesh3d);
        mesh.setType(GPU_MESH_DESC_TYPE::GENERIC);

        if (cell->transform_block) {
            gpuGetDevice()->destroyParamBlock(cell->transform_block);
        }
        auto transform_block = gpuGetDevice()->createParamBlock<gpuTransformBlock>();
        cell->transform_block = transform_block;

        transform_block->setTransform(gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(CELL_X, 0, CELL_Z)), false);
        renderable.attachParamBlock(transform_block);
        renderable.setMaterial(terrain_material.get());
        renderable.setMeshDesc(mesh.getMeshDesc());
        renderable.compile();
    }

    float sampleHeightQuadSpace(int icell, int x, int z) {
        int icellx = icell % WIDTH;
        int icellz = icell / WIDTH;

        while (x >= CELL_SEGMENTS_X && icellx < WIDTH - 1) {
            x -= CELL_SEGMENTS_X;
            icellx += 1;
        }
        while (z >= CELL_SEGMENTS_Z && icellz < DEPTH - 1) {
            z -= CELL_SEGMENTS_Z;
            icellz += 1;
        }

        x = gfxm::_min(x, CELL_SEGMENTS_X - 1);
        z = gfxm::_min(z, CELL_SEGMENTS_Z - 1);
        return cells[icellx + icellz * WIDTH].points[x + z * CELL_SEGMENTS_X];
    }

    bool hitTest(gfxm::vec3& out) {
        gfxm::ray r = viewport->makeRayFromMousePos();
        const gfxm::vec3 A = r.origin;
        const gfxm::vec3 B = r.origin + r.direction * r.length;

        gfxm::aabb ray_box(
            gfxm::_min(A.x, B.x),
            gfxm::_min(A.y, B.y),
            gfxm::_min(A.z, B.z),
            gfxm::_max(A.x, B.x),
            gfxm::_max(A.y, B.y),
            gfxm::_max(A.z, B.z)
        );

        gfxm::ivec2 icmin(
            gfxm::iclamp(int(ray_box.from.x / CELL_WIDTH), 0, WIDTH - 1),
            gfxm::iclamp(int(ray_box.from.z / CELL_DEPTH), 0, DEPTH - 1)
        );
        gfxm::ivec2 icmax(
            gfxm::iclamp(int(ray_box.to.x / CELL_WIDTH), 0, WIDTH - 1),
            gfxm::iclamp(int(ray_box.to.z / CELL_DEPTH), 0, DEPTH - 1)
        );
        
        std::set<int> potential_cells;
        for (int cz = icmin.y; cz <= icmax.y; ++cz) {
            for (int cx = icmin.x; cx <= icmax.x; ++cx) {
                int icell = cx + cz * WIDTH;
                TerrainCell* cell = &cells[icell];
                
                // TODO:
                const float CELL_X = (icell % WIDTH) * CELL_WIDTH;
                const float CELL_Z = (icell / WIDTH) * CELL_DEPTH;
                gfxm::aabb cell_box(
                    gfxm::vec3(CELL_X, cell->y_min, CELL_Z),
                    gfxm::vec3(CELL_X + CELL_WIDTH, cell->y_max, CELL_Z + CELL_DEPTH)
                );

                gfxm::vec3 cA, cB;
                if (!clipSegmentXZ(A, B, cell_box, cA, cB)) {
                    continue;
                }
                float ymin = gfxm::_min(cA.y, cB.y);
                float ymax = gfxm::_max(cA.y, cB.y);

                if (ymin < cell->y_min && ymax < cell->y_min) {
                    continue;
                }
                if (ymin > cell->y_max && ymax > cell->y_max) {
                    continue;
                }

                potential_cells.insert(icell);
            }
        }

        float min_dist = FLT_MAX;
        bool has_hit = false;
        const float QUAD_WIDTH = CELL_WIDTH / CELL_SEGMENTS_X;
        const float QUAD_DEPTH = CELL_DEPTH / CELL_SEGMENTS_Z;
        for (auto icell : potential_cells) {
            TerrainCell* cell = &cells[icell];
            const int icx = icell % WIDTH;
            const int icz = icell / WIDTH;
            const float CELL_X = icx * CELL_WIDTH;
            const float CELL_Z = icz * CELL_DEPTH;

            gfxm::aabb lcl_ray_box(
                ray_box.from - gfxm::vec3(CELL_X, .0f, CELL_Z),
                ray_box.to - gfxm::vec3(CELL_X, .0f, CELL_Z)
            );
            gfxm::ivec2 iqmin(
                gfxm::iclamp(int(lcl_ray_box.from.x / QUAD_WIDTH), 0, CELL_SEGMENTS_X - 1),
                gfxm::iclamp(int(lcl_ray_box.from.z / QUAD_DEPTH), 0, CELL_SEGMENTS_Z - 1)
            );
            gfxm::ivec2 iqmax(
                gfxm::iclamp(int(lcl_ray_box.to.x / QUAD_WIDTH), 0, CELL_SEGMENTS_X - 1),
                gfxm::iclamp(int(lcl_ray_box.to.z / QUAD_DEPTH), 0, CELL_SEGMENTS_Z - 1)
            );

            for (int iqz = iqmin.y; iqz <= iqmax.y; ++iqz) {
                for (int iqx = iqmin.x; iqx <= iqmax.x; ++iqx) {
                    float y0 = sampleHeightQuadSpace(icell, iqx, iqz);
                    float y1 = sampleHeightQuadSpace(icell, iqx, iqz + 1);
                    float y2 = sampleHeightQuadSpace(icell, iqx + 1, iqz + 1);
                    float y3 = sampleHeightQuadSpace(icell, iqx + 1, iqz);
                    
                    gfxm::vec3 p0 = gfxm::vec3(iqx * QUAD_WIDTH, y0, iqz * QUAD_DEPTH) + gfxm::vec3(CELL_X, .0f, CELL_Z);
                    gfxm::vec3 p1 = gfxm::vec3(iqx * QUAD_WIDTH, y1, (iqz + 1) * QUAD_DEPTH) + gfxm::vec3(CELL_X, .0f, CELL_Z);
                    gfxm::vec3 p2 = gfxm::vec3((iqx + 1) * QUAD_WIDTH, y2, (iqz + 1) * QUAD_DEPTH) + gfxm::vec3(CELL_X, .0f, CELL_Z);
                    gfxm::vec3 p3 = gfxm::vec3((iqx + 1) * QUAD_WIDTH, y3, iqz * QUAD_DEPTH) + gfxm::vec3(CELL_X, .0f, CELL_Z);

                    gfxm::vec3 pt;
                    float dist = .0f;
                    if (gfxm::intersect_line_triangle(A, B, p0, p1, p2, pt, dist)) {
                        if (dist < min_dist) {
                            out = pt;
                            min_dist = dist;
                            has_hit = true;
                            continue;
                        }
                    }
                    if (gfxm::intersect_line_triangle(A, B, p2, p3, p0, pt, dist)) {
                        if (dist < min_dist) {
                            out = pt;
                            min_dist = dist;
                            has_hit = true;
                            continue;
                        }
                    }
                }
            }
        }

        return has_hit;
    }

    void applyBrush(float ptx, float ptz) {
        float MULTIPLIER = 1.f;
        if (guiIsModifierKeyPressed(GUI_KEY_CONTROL)) {
            MULTIPLIER = -1.f;
        }

        const float BRUSH_STRENGTH = .2f;

        const float QUAD_WIDTH = CELL_WIDTH / CELL_SEGMENTS_X;
        const float QUAD_DEPTH = CELL_DEPTH / CELL_SEGMENTS_Z;

        gfxm::ivec2 icmin(
            gfxm::iclamp((ptx - brush_radius) / CELL_WIDTH, 0, WIDTH - 1),
            gfxm::iclamp((ptz - brush_radius) / CELL_DEPTH, 0, DEPTH - 1)
        );
        gfxm::ivec2 icmax(
            gfxm::iclamp((ptx + brush_radius) / CELL_WIDTH, 0, WIDTH - 1),
            gfxm::iclamp((ptz + brush_radius) / CELL_DEPTH, 0, DEPTH - 1)
        );
        
        // Paint
        gfxm::vec2 fmin(ptx - brush_radius, ptz - brush_radius);
        gfxm::vec2 fmax(ptx + brush_radius, ptz + brush_radius);
        gfxm::vec2 qmin(ceilf(fmin.x / QUAD_WIDTH), ceilf(fmin.y / QUAD_DEPTH));
        gfxm::vec2 qmax(floorf(fmax.x / QUAD_WIDTH), floorf(fmax.y / QUAD_DEPTH));
        const int region_width = qmax.x + 1 - qmin.x;
        const int region_depth = qmax.y + 1 - qmin.y;
        //LOG_DBG("REGION: " << region_width << ", " << region_depth);

        std::vector<float> region(region_width * region_depth);
        // Copy existing heights into the brush region
        for (int icz = icmin.y; icz <= icmax.y; ++icz) {
            for (int icx = icmin.x; icx <= icmax.x; ++icx) {
                const int icell = icx + icz * WIDTH;

                // Region bounds in terms of cell-local points
                const int rminx = qmin.x - CELL_SEGMENTS_X * icx;
                const int rminz = qmin.y - CELL_SEGMENTS_Z * icz;
                const int rmaxx = qmax.x - CELL_SEGMENTS_X * icx;
                const int rmaxz = qmax.y - CELL_SEGMENTS_Z * icz;
                // How much the region overflows on each side
                const int rminx_of = gfxm::_max(0, -rminx);
                const int rminz_of = gfxm::_max(0, -rminz);
                const int rmaxx_of = gfxm::_max(0, rmaxx + 1 - CELL_SEGMENTS_X);
                const int rmaxz_of = gfxm::_max(0, rmaxz + 1 - CELL_SEGMENTS_Z);

                const int rxl = region_width - rminx_of - rmaxx_of;
                const int rzl = region_depth - rminz_of - rmaxz_of;
                for (int pz = 0; pz < rzl; ++pz) {
                    for (int px = 0; px < rxl; ++px) {
                        const int rpx = rminx_of + px;
                        const int rpz = rminz_of + pz;
                        const int cpx = rminx + rminx_of + px;
                        const int cpz = rminz + rminz_of + pz;
                        region[rpx + rpz * region_width]
                            = cells[icell].points[cpx + cpz * CELL_SEGMENTS_X];
                    }
                }
            }
        }

        std::vector<uint8_t> tip_raster(region_width * region_depth);
        brush_tip->rasterize(
            tip_raster.data(), region_width, region_depth,
            gfxm::fract(gfxm::vec2(ptx / QUAD_WIDTH, ptz / QUAD_DEPTH)), .0f
        );

        TerrainBrushContext ctx {
            .region = region.data(),
            .mask = tip_raster.data(),
            .width = region_width,
            .height = region_depth,
            .radius = brush_radius,
            .strength = BRUSH_STRENGTH * MULTIPLIER,
        };
        if (guiIsModifierKeyPressed(GUI_KEY_SHIFT)) {
            secondary_brush->apply(ctx);
        } else {
            current_brush->apply(ctx);
        }

        for (int icz = icmin.y; icz <= icmax.y; ++icz) {
            for (int icx = icmin.x; icx <= icmax.x; ++icx) {
                const int icell = icx + icz * WIDTH;
                const float CELL_X = (icell % WIDTH) * CELL_WIDTH;
                const float CELL_Z = (icell / WIDTH) * CELL_DEPTH;

                // Region bounds in terms of cell-local points
                const int rminx = qmin.x - CELL_SEGMENTS_X * icx;
                const int rminz = qmin.y - CELL_SEGMENTS_Z * icz;
                const int rmaxx = qmax.x - CELL_SEGMENTS_X * icx;
                const int rmaxz = qmax.y - CELL_SEGMENTS_Z * icz;
                //LOG_DBG("LOCAL: [" << rminx << ", " << rminz << ", " << rmaxx << ", " << rmaxz << "]");
                // How much the region overflows on each side
                const int rminx_of = gfxm::_max(0, -rminx);
                const int rminz_of = gfxm::_max(0, -rminz);
                const int rmaxx_of = gfxm::_max(0, rmaxx + 1 - CELL_SEGMENTS_X);
                const int rmaxz_of = gfxm::_max(0, rmaxz + 1 - CELL_SEGMENTS_Z);
                //LOG_DBG("OVERFLOW: [" << rminx_of << ", " << rminz_of << ", " << rmaxx_of << ", " << rmaxz_of << "]");

                gfxm::vec2 lclpt(ptx - CELL_X, ptz - CELL_Z);
                gfxm::vec2 lclmin(lclpt.x - brush_radius, lclpt.y - brush_radius);
                gfxm::vec2 lclmax(lclpt.x + brush_radius, lclpt.y + brush_radius);
                gfxm::ivec2 dlclmin(
                    gfxm::_max(0, int(lclmin.x / CELL_WIDTH * CELL_SEGMENTS_X)),
                    gfxm::_max(0, int(lclmin.y / CELL_DEPTH * CELL_SEGMENTS_Z))
                );
                gfxm::ivec2 dlclmax(
                    gfxm::_min(CELL_SEGMENTS_X - 1, int(lclmax.x / CELL_WIDTH * CELL_SEGMENTS_X)),
                    gfxm::_min(CELL_SEGMENTS_Z - 1, int(lclmax.y / CELL_DEPTH * CELL_SEGMENTS_Z))
                );

                const int rxl = region_width - rminx_of - rmaxx_of;
                const int rzl = region_depth - rminz_of - rmaxz_of;
                for (int pz = 0; pz < rzl; ++pz) {
                    for (int px = 0; px < rxl; ++px) {
                        const int rpx = rminx_of + px;
                        const int rpz = rminz_of + pz;
                        const int cpx = rminx + rminx_of + px;
                        const int cpz = rminz + rminz_of + pz;
                        cells[icell].points[cpx + cpz * CELL_SEGMENTS_X]
                            = region[rpx + rpz * region_width];
                    }
                }

                dirty_cells.insert(icell);
            }
        }
        
        // Smooth
        /*
        float sum = .0f;
        float weight = .0f;
        for (int icz = icmin.y; icz <= icmax.y; ++icz) {
            for (int icx = icmin.x; icx <= icmax.x; ++icx) {
                const int icell = icx + icz * WIDTH;
                const float CELL_X = (icell % WIDTH) * CELL_WIDTH;
                const float CELL_Z = (icell / WIDTH) * CELL_DEPTH;

                gfxm::vec2 lclpt(ptx - CELL_X, ptz - CELL_Z);
                gfxm::vec2 lclmin(lclpt.x - brush_radius, lclpt.y - brush_radius);
                gfxm::vec2 lclmax(lclpt.x + brush_radius, lclpt.y + brush_radius);
                gfxm::ivec2 dlclmin(
                    gfxm::_max(0, int(lclmin.x / CELL_WIDTH * CELL_SEGMENTS_X)),
                    gfxm::_max(0, int(lclmin.y / CELL_DEPTH * CELL_SEGMENTS_Z))
                );
                gfxm::ivec2 dlclmax(
                    gfxm::_min(CELL_SEGMENTS_X - 1, int(lclmax.x / CELL_WIDTH * CELL_SEGMENTS_X)),
                    gfxm::_min(CELL_SEGMENTS_Z - 1, int(lclmax.y / CELL_DEPTH * CELL_SEGMENTS_Z))
                );

                auto& cell = cells[icell];
                for (int z = dlclmin.y; z <= dlclmax.y; ++z) {
                    for (int x = dlclmin.x; x <= dlclmax.x; ++x) {
                        gfxm::vec2 vf = gfxm::vec2(
                            (x * QUAD_WIDTH - lclmin.x) / (brush_radius) - 1.f,
                            (z * QUAD_DEPTH - lclmin.y) / (brush_radius) - 1.f
                        );
                        float f = gfxm::_max(.0f, 1.f - gfxm::length(vf));

                        int ipt = x + z * CELL_SEGMENTS_X;
                        sum += cell->points[ipt] * f;
                        weight += f;
                    }
                }

                dirty_cells.insert(icell);
            }
        }
        if(weight == .0f) weight = 1.f;
        const float avg = sum / weight;
        
        for (int icz = icmin.y; icz <= icmax.y; ++icz) {
            for (int icx = icmin.x; icx <= icmax.x; ++icx) {
                const int icell = icx + icz * WIDTH;
                const float CELL_X = (icell % WIDTH) * CELL_WIDTH;
                const float CELL_Z = (icell / WIDTH) * CELL_DEPTH;

                gfxm::vec2 lclpt(ptx - CELL_X, ptz - CELL_Z);
                gfxm::vec2 lclmin(lclpt.x - brush_radius, lclpt.y - brush_radius);
                gfxm::vec2 lclmax(lclpt.x + brush_radius, lclpt.y + brush_radius);
                gfxm::ivec2 dlclmin(
                    gfxm::_max(0, int(lclmin.x / CELL_WIDTH * CELL_SEGMENTS_X)),
                    gfxm::_max(0, int(lclmin.y / CELL_DEPTH * CELL_SEGMENTS_Z))
                );
                gfxm::ivec2 dlclmax(
                    gfxm::_min(CELL_SEGMENTS_X - 1, int(lclmax.x / CELL_WIDTH * CELL_SEGMENTS_X)),
                    gfxm::_min(CELL_SEGMENTS_Z - 1, int(lclmax.y / CELL_DEPTH * CELL_SEGMENTS_Z))
                );

                auto& cell = cells[icell];
                for (int z = dlclmin.y; z <= dlclmax.y; ++z) {
                    for (int x = dlclmin.x; x <= dlclmax.x; ++x) {
                        gfxm::vec2 vf = gfxm::vec2(
                            (x * QUAD_WIDTH - lclmin.x) / (brush_radius) - 1.f,
                            (z * QUAD_DEPTH - lclmin.y) / (brush_radius) - 1.f
                        );
                        float f = gfxm::_max(.0f, 1.f - gfxm::length(vf));

                        int ipt = x + z * CELL_SEGMENTS_X;
                        cell->points[ipt] = gfxm::lerp(cell->points[ipt], avg, BRUSH_STRENGTH * f);
                    }
                }

                dirty_cells.insert(icell);
            }
        }*/

        guiScheduleTick(this, 0, GUI_TICK_CUSTOM);
    }

    void onTick(float dt, GUI_TICK_ID id) override {
        if (id != GUI_TICK_CUSTOM) {
            return;
        }
        for(auto icell : dirty_cells) {
            updateCell(icell);
        }
        dirty_cells.clear();

        brush_hit = hitTest(brush_pos);
        gfxm::mat4 t = gfxm::translate(gfxm::mat4(1.f), brush_pos);
        renderable2->setTransform(t);
    }

    void toJson(nlohmann::json& json) const override {
        json["version"] = TERRAIN_SPACE_VERSION;

        nlohmann::json& jcells = json["cells"];
        jcells = nlohmann::json::array();
        for (int i = 0; i < cells.size(); ++i) {
            const unsigned char* buf = (unsigned char*)cells[i].points.data();
            uint64_t bufsz = cells[i].points.size() * sizeof(cells[i].points[0]);

            std::string b64;
            base64_encode(buf, bufsz, b64);
            jcells.push_back(b64);
        }
    }
    bool fromJson(const nlohmann::json& json) override {
        int version = json["version"].get<int>();
        LOG("TERRAIN SPACE VERSION: " << version);

        nlohmann::json jcells = json.value("cells", nlohmann::json::array());
        if (!jcells.is_array()) {
            return false;
        }
        cells.clear();
        cells.reserve(WIDTH * DEPTH);
        for (auto it = jcells.begin(); it != jcells.end(); ++it) {
            nlohmann::json jcell = it->get<nlohmann::json>();
            std::string str = jcell.get<std::string>();
            std::vector<char> data;
            base64_decode(str.data(), str.size(), data);
            TerrainCell* cell = &cells.emplace_back();
            cell->points.resize(data.size() / sizeof(cell->points[0]));
            memcpy(cell->points.data(), data.data(), data.size());
        }
        // Seam fix
        /*for (int icell = 0; icell < cells.size(); ++icell) {
            int icellx = icell % WIDTH;
            int icellz = icell / WIDTH;
            for (int ix = 0; ix < CELL_SEGMENTS_X; ++ix) {
                const int iz = CELL_SEGMENTS_Z - 1;
                const int iz_prev = CELL_SEGMENTS_Z - 2;
                const int iz_next = CELL_SEGMENTS_Z;
                float& y = cells[icell]->points[ix + iz * CELL_SEGMENTS_X];
                float y_prev = sampleHeightQuadSpace(icell, ix, iz_prev);
                float y_next = sampleHeightQuadSpace(icell, ix, iz_next);
                y = (y_prev + y_next) * .5f;
            }
            for (int iz = 0; iz < CELL_SEGMENTS_X; ++iz) {
                const int ix = CELL_SEGMENTS_X - 1;
                const int ix_prev = CELL_SEGMENTS_X - 2;
                const int ix_next = CELL_SEGMENTS_X;
                float& y = cells[icell]->points[ix + iz * CELL_SEGMENTS_X];
                float y_prev = sampleHeightQuadSpace(icell, ix_prev, iz);
                float y_next = sampleHeightQuadSpace(icell, ix_next, iz);
                y = (y_prev + y_next) * .5f;
            }
            const int ix = CELL_SEGMENTS_X - 1;
            const int iz = CELL_SEGMENTS_Z - 1;
            float& y = cells[icell]->points[ix + iz * CELL_SEGMENTS_X];
            float y_prev = sampleHeightQuadSpace(icell, ix - 1, iz - 1);
            float y_next = sampleHeightQuadSpace(icell, ix + 1, iz + 1);
            y = (y_prev + y_next) * .5f;
        }*/

        updatePreview();
        return true;
    }
public:
    TerrainSceneSpace()
    : SceneSpace("Terrain") {
        gpuGetPipeline()->enableTechnique("Fog", false);
        
        subscribe([this](const GuiEvt_KeyDown& e) {
            switch (e.vkey) {
            case 90: // Z key
                if(brush_hit) {
                    viewport->setCameraPivot(brush_pos);
                }
                return;
            }
            e.consume = false;
            e.invoke_next();
        });
        subscribe([this](const GuiEvt_Pull& e) {
            if (e.btn != GUI_MOUSE_LEFT) {
                return;
            }
            if (!brush_hit) {
                return;
            }
            applyBrush(brush_pos.x, brush_pos.z);
        });
        subscribe([this](const GuiEvt_MouseMove& e) {
            e.consume = false;
            // TODO: Scheduled hit test, should probably be a separate tick from dirty cell updates
            guiScheduleTick(this, 0, GUI_TICK_CUSTOM);
        });
        subscribe([this](const GuiEvt_Scroll& e) {
            if (!guiIsModifierKeyPressed(GUI_KEY_CONTROL)) {
                e.consume = false;
                return;
            }
            float modifier = brush_radius;
            int offset = 1;
            if (e.value < .0f) {
                offset = -1;
                modifier = MAX_BRUSH_RADIUS + .5f - brush_radius;
            }
            brush_scale = gfxm::_max(.0f, gfxm::_min(1.f, brush_scale + e.value * .00025f));
            brush_radius = gfxm::lerp(MIN_BRUSH_RADIUS, MAX_BRUSH_RADIUS, brush_scale * brush_scale);
            renderable2->setExtents(gfxm::vec3(brush_radius * 2.f, 100.f, brush_radius * 2.f));
        });

        {
            material2 = loadResource<gpuMaterial>("materials/decals/pentagram");
            renderable2.reset(new gpuDecalRenderable(material2.get(), 0, "MyBrush"));
            renderable2->setExtents(gfxm::vec3(brush_radius * 2.f, 100.f, brush_radius * 2.f));
            renderable2->setTransform(gfxm::mat4(1.f));
        }

        initData();
        updatePreview();

        current_brush.reset(new DrawTerrainBrush);
        secondary_brush.reset(new SmoothTerrainBrush);
        brush_tip.reset(new RadialBrushTip);
    }

    void initData() {
        cells.resize(WIDTH * DEPTH);
        for (int i = 0; i < WIDTH * DEPTH; ++i) {
            auto& cell = cells[i];
            cell.points.resize(CELL_SEGMENTS_X * CELL_SEGMENTS_Z);
            std::fill(cell.points.begin(), cell.points.end(), .0f);
        }
    }

    void updatePreview() {
        for (int i = 0; i < WIDTH * DEPTH; ++i) {
            updateCell(i);
        }
    }

    void queryGeometry(const GeometryQuery& q) {
        if(brush_hit) {
            q.bucket->add(renderable2.get());
        }

        // Frustum vs cells
        const gfxm::frustum& fru = q.query.fru;
        gfxm::vec3 corners[8];
        gfxm::intersect_planes(fru.planes[0], fru.planes[2], fru.planes[4], corners[0]);
        gfxm::intersect_planes(fru.planes[1], fru.planes[2], fru.planes[4], corners[1]);
        gfxm::intersect_planes(fru.planes[0], fru.planes[3], fru.planes[4], corners[2]);
        gfxm::intersect_planes(fru.planes[1], fru.planes[3], fru.planes[4], corners[3]);
        gfxm::intersect_planes(fru.planes[0], fru.planes[2], fru.planes[5], corners[4]);
        gfxm::intersect_planes(fru.planes[1], fru.planes[2], fru.planes[5], corners[5]);
        gfxm::intersect_planes(fru.planes[0], fru.planes[3], fru.planes[5], corners[6]);
        gfxm::intersect_planes(fru.planes[1], fru.planes[3], fru.planes[5], corners[7]);
        float fminx = corners[0].x;
        float fminz = corners[0].z;
        float fmaxx = corners[0].x;
        float fmaxz = corners[0].z;
        for (int i = 1; i < 8; ++i) {
            fminx = gfxm::_min(fminx, corners[i].x);
            fminz = gfxm::_min(fminz, corners[i].z);
            fmaxx = gfxm::_max(fmaxx, corners[i].x);
            fmaxz = gfxm::_max(fmaxz, corners[i].z);
        }
        int icminx = gfxm::iclamp(fminx / CELL_WIDTH, 0, WIDTH - 1);
        int icmaxx = gfxm::iclamp(fmaxx / CELL_WIDTH, 0, WIDTH - 1);
        int icminz = gfxm::iclamp(fminz / CELL_DEPTH, 0, DEPTH - 1);
        int icmaxz = gfxm::iclamp(fmaxz / CELL_DEPTH, 0, DEPTH - 1);

        for (int icz = icminz; icz <= icmaxz; ++icz) {
            for (int icx = icminx; icx <= icmaxx; ++icx) {
                const int icell = icx + icz * WIDTH;
                q.bucket->add(&cells[icell].renderable);
            }
        }
    }
    void enterUi(SceneEditorContext& ctx) override {
        // TODO:
    }
};

constexpr static int SCENE_DATA_VERSION = 2;

struct SceneData : public gpuSceneQueryInterface {
    std::unique_ptr<SceneSpace> scene_space;
    std::vector<SceneEntry> entries;

    SceneData() {
        scene_space.reset(new TerrainSceneSpace);
    }

    void queryGeometry(const GeometryQuery& q) override {
        scene_space->queryGeometry(q);
    }

    void toJson(nlohmann::json& json) const {
        json["version"] = SCENE_DATA_VERSION;

        nlohmann::json& jentries = json["entries"];
        jentries = nlohmann::json::array();
        for (int i = 0; i < entries.size(); ++i) {
            ActorPrefab prefab;
            entries[i].instance->makePrefab(prefab);
            nlohmann::json& jentry = jentries.emplace_back();
            prefab.toJson(jentry);
        }

        nlohmann::json& jspace = json["space"];
        scene_space->toJson(jspace);
    }
    bool fromJson(const nlohmann::json& json) {
        int version = json["version"].get<int>();
        LOG("SCENE VERSION: " << version);

        nlohmann::json jentries = json.value("entries", nlohmann::json::array());
        if (!jentries.is_array()) {
            return false;
        }

        entries.clear();
        for (auto it = jentries.begin(); it != jentries.end(); ++it) {
            nlohmann::json jentry = it->get<nlohmann::json>();
            ActorPrefab prefab;
            if (!prefab.fromJson(jentry)) {
                LOG_ERR("Failed to load actor");
                continue;
            }
            auto& entry = entries.emplace_back();
            entry.instance = std::unique_ptr<Actor>(prefab.instantiate()); // TODO: new is hidden here, kinda uncomfortable
        }

        if (version >= 2) {
            nlohmann::json jspace = json.value("space", nlohmann::json::object());
            scene_space.reset(new TerrainSceneSpace); // TODO: scene space types
            scene_space->fromJson(jspace);
        }

        return true;
    }
};

struct GuiEvt_EntrySelected : public GuiEvent {
    GuiEvt_EntrySelected(SceneEntry* e) : entry(e) {}
    SceneEntry* entry = nullptr;
};

class GuiSceneInspector : public GuiElement {
    SceneData* scene_data = nullptr;

    GuiTreeView* list = nullptr;
    std::vector<GuiTreeItem*> items; // TODO: Should just access items through list
public:
    GuiSceneInspector(SceneData* scn)
    : scene_data(scn) {
        setSize(200, 400);
        list = guiCreate<GuiTreeView>();
        list->setSize(gui::fill(), gui::fill());
        pushBack(list);
        updateView();
    }

    void setSelected(SceneEntry* e) {
        for (int i = 0; i < items.size(); ++i) {
            auto item = items[i];
            if (item->user_ptr != e) {
                continue;
            }
            item->invokeBubble(GuiEvt_Selected{item});
            list->scrollTo(item);
        }
    }

    void updateView() {
        list->clearChildren();
        items.clear();
        
        auto& entries = scene_data->entries;
        for (int i = 0; i < entries.size(); ++i) {
            SceneEntry& entry = entries[i];
            Actor* actor = entry.instance.get();
            GuiTreeItem* item = list->addItem(entry.instance->getName().c_str());
            item->user_ptr = &entry;
            item->subscribe([this](const GuiEvt_Selected& e) {
                e.invoke_next();
                invoke(GuiEvt_EntrySelected(static_cast<SceneEntry*>(e.elem->user_ptr)));
            });
            items.push_back(item);
        }
    }
};

class GuiSceneDocument : public GuiEditorWindow {
    SceneData scene_data;

    // ===========================================
    GuiActorInspector* actor_inspector = nullptr;
    GuiSceneInspector* scene_inspector = nullptr;

    RuntimeWorld world;

    gpuMesh mesh;
    std::unique_ptr<gpuGeometryRenderable> renderable;

    gpuMesh mesh2;
    std::unique_ptr<gpuGeometryRenderable> renderable2;
    ResourceRef<gpuMaterial> material2;

    SceneEntry* selected_entry = nullptr;

    void enableTransformTool() {
        if (!selected_entry) {
            return;
        }
        viewport.removeTool(&tool_transform);
        viewport.addTool(&tool_transform);
        tool_transform.translation = selected_entry->instance->getTranslation();
        tool_transform.rotation = selected_entry->instance->getRotation();
    }

    void selectEntry(SceneEntry* e) {
        selected_entry = e;
        actor_inspector->init(e->instance.get());
        scene_inspector->setSelected(e);
    }

public:
    GuiViewport viewport;
    GuiViewportToolTransform tool_transform;

    GuiSceneDocument(GuiActorInspector* inspector = nullptr)
        : GuiEditorWindow("GenericScene", "scene")
    {
        actor_inspector = guiCreate<GuiActorInspector>();
        guiGetRoot()->getWindowLayer()->pushBack(actor_inspector);
        actor_inspector->subscribe([this](const GuiEvt_PropChanged&) {
            enableTransformTool(); // TODO: actually just update transform data
        });

        scene_inspector = guiCreate<GuiSceneInspector>(&scene_data);
        scene_inspector->subscribe([this](const GuiEvt_EntrySelected& e) {
            selected_entry = e.entry;
            actor_inspector->init(e.entry->instance.get());
            enableTransformTool();
        });
        guiGetRoot()->getWindowLayer()->pushBack(scene_inspector);
        
        tool_transform.subscribe([this](const GuiEvt_GizmoTranslate& e) {
            if(!selected_entry) return;
            selected_entry->instance->translate(e.delta);
        });
        tool_transform.subscribe([this](const GuiEvt_GizmoRotate& e) {
            if(!selected_entry) return;
            selected_entry->instance->rotate(e.delta);
        });

        viewport.subscribe([this](const GuiEvt_RClick&) {
            auto menu = guiCreate<GuiMenuList>();
            guiAddTransientPopup(nullptr, menu, guiGetMousePos());
            auto item_create = menu->addItem("Create Actor...", 0);
            item_create->subscribe([this, menu](const GuiEvt_LClick&) {
                auto& entry = scene_data.entries.emplace_back();
                entry.instance.reset(new Actor);
                entry.instance->setName("Actor");
                world.spawn(entry.instance.get());
                scene_inspector->updateView();
                selectEntry(&entry);
                guiRemoveTransientPopup(menu);
            });
            auto item_focus = menu->addItem("Focus selected", 0);
            item_focus->subscribe([this, menu](const GuiEvt_LClick&) {
                // TODO:
                guiRemoveTransientPopup(menu);
            });
        });

        viewport.getRenderView()->setView(gfxm::mat4(1.f));
        viewport.getRenderView()->setZFar(4000.f);
        viewport.getRenderView()->setZNear(.2f);
        {
            viewport.getRenderView()->addQueryInterface(world.getSystem<SceneSystem>());
            viewport.getRenderView()->addQueryInterface(world.getSystem<scnRenderScene>());
            viewport.getRenderView()->addQueryInterface(&scene_data);
        }

        addChild(&viewport);
        viewport.setOwner(this);

        {
            Mesh3d mesh_ram;
            meshGenerateGrid(&mesh_ram, 160, 160, 160);
            mesh.setData(&mesh_ram);
            mesh.setDrawMode(MESH_DRAW_MODE::MESH_DRAW_LINES);
            mesh.setType(GPU_MESH_DESC_TYPE::LINE);
            renderable.reset(new gpuGeometryRenderable(nullptr, mesh.getMeshDesc(), 0, "Grid"));
            renderable->setTransform(gfxm::mat4(1.f));
        }

        {
            Mesh3d mesh_ram;
            meshGenerateCube(&mesh_ram);
            mesh2.setData(&mesh_ram);
            mesh2.setType(GPU_MESH_DESC_TYPE::GENERIC);
            mesh2.setDrawMode(MESH_DRAW_MODE::MESH_DRAW_TRIANGLES);
            material2 = loadResource<gpuMaterial>("materials/default3");
            renderable2.reset(new gpuGeometryRenderable(material2.get(), mesh2.getMeshDesc(), 0, "MyCube"));
            renderable2->setTransform(gfxm::mat4(1.f));
        }

        guiScheduleTick(this, 1.f / 30.f, GUI_TICK_CUSTOM);

        // 
        viewport.addTool(scene_data.scene_space.get());
    }

    void onTick(float dt, GUI_TICK_ID id) override {
        if (id != GUI_TICK_CUSTOM) {
            return;
        }
        world.update(dt);
        guiScheduleTick(this, 1.f / 30.f, GUI_TICK_CUSTOM);
    }

    void onDraw() override {
        viewport.getRenderView()->getRenderBucket()->add(renderable.get());
        /*
        render_instance.render_view->getRenderBucket()->add(renderable2.get());
        viewport.render_instance->world.getRenderScene()->draw(render_instance.render_view->getRenderBucket());

        static float time = .0f;
        time += .01f;
        renderable2->setVec4("color", gfxm::vec4(gfxm::hsv2rgb(sinf(time * (1.f/7.f)), 1.f, 1.f), 1.f));
        */
        GuiEditorWindow::onDraw();
    }
    bool onSaveCommand(const std::string& path) override {
        LOG_DBG("onSaveCommand");
        nlohmann::json json;
        scene_data.toJson(json);
        std::ofstream f(path);
        f << json.dump(4);
        return true;
    }

    bool onOpenCommand(const std::string& path) override {
        LOG_DBG("onOpenCommand");
        if (scene_data.scene_space) {
            viewport.removeTool(scene_data.scene_space.get());
        }

        std::ifstream f(path, std::ios::binary);
        if (!f.is_open()) {
            LOG_ERR("Failed to open file '" << path << "'");
            return false;
        }
        std::string fstr((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        nlohmann::json json = nlohmann::json::parse(fstr);
        if (!scene_data.fromJson(json)) {
            return false;
        }

        // Update the "preview" state
        for(int i = 0; i < scene_data.entries.size(); ++i) {
            world.spawn(scene_data.entries[i].instance.get());
        }
        // Update ui state
        scene_inspector->updateView();
        //
        viewport.addTool(scene_data.scene_space.get());
        
        return true;
    }
};

