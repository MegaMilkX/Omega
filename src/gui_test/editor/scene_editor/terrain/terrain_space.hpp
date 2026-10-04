#pragma once

#include "math/intersection.hpp"
#include "../scene_space.hpp"
#include "util.hpp"
#include "gpu/device.hpp"
#include "gpu/gpu_pipeline.hpp"
#include "gpu/material/vfx_material.hpp"

#include "cell.hpp"

#include "brush.hpp"
#include "brush_tip.hpp"

#include "modes/sculpt_mode.hpp"
#include "modes/decoration_mode.hpp"

struct TerrainParams {
    int cell_segments_x = 0;
    int cell_segments_z = 0;
    float cell_width = .0f;
    float cell_depth = .0f;
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
constexpr static int TERRAIN_SPACE_VERSION = 2;
class TerrainSceneSpace : public SceneSpace {
    // Terrain data
    constexpr static int CELL_SEGMENTS_X = 200;
    constexpr static int CELL_SEGMENTS_Z = 200;
    constexpr static float CELL_WIDTH = 204.8f;
    constexpr static float CELL_DEPTH = 204.8f;

    //std::vector<TerrainCell> cells;
    std::unordered_map<uint64_t, TerrainCell> cell_table;

    // Preview
    ResourceRef<gpuMaterial> terrain_material = loadResource<gpuMaterial>("materials/terrain");
    std::set<uint64_t> dirty_cells;
    struct CellRemeshScratch {
        std::vector<gfxm::vec3> vertices;
        std::vector<gfxm::vec3> normals;
        std::vector<gfxm::vec3> tangents;
        std::vector<gfxm::vec3> bitangents;
        std::vector<gfxm::vec2> uvs;
        std::vector<COLOR24> colors;
        std::vector<uint32_t> indices;
    } cell_remesh_scratch;

    // 
    std::unique_ptr<TerrainEditMode> edit_mode;
    

    void updateCell(uint64_t cell_key) {
        TerrainCell* cell = getCell(cell_key);
        if (!cell) {
            return;
        }

        cell->y_min = cell->points[0];
        cell->y_max = cell->y_min;
        for (int i = 1; i < cell->points.size(); ++i) {
            cell->y_min = gfxm::_min(cell->y_min, cell->points[i]);
            cell->y_max = gfxm::_max(cell->y_max, cell->points[i]);
        }

        int cx = 0;
        int cz = 0;
        terrainCellCoordsFromKey(cell_key, cx, cz);
        updateCellPreview(cell, cx, cz);
        updateCellDecorations(cell, cx, cz);
    }

    void updateCellDecorations(TerrainCell* cell, int cx, int cz) {
        for (int i = 0; i < cell->decorations.size(); ++i) {
            auto& deco = cell->decorations[i];

            for (int j = 0; j < deco.instances.size(); ++j) {
                auto& inst = deco.instances[j];
                inst.pos.y = sampleHeightWorldBilinear(inst.pos.x, inst.pos.z);
            }
            deco.inst_desc.setArray(deco.instances.data(), deco.instances.size());
        }
    }

    void updateCellPreview(TerrainCell* cell, int cx, int cz) {
        uint64_t cell_key = terrainCellKey(cx, cz);
        const float CELL_X = cx * CELL_WIDTH;
        const float CELL_Z = cz * CELL_DEPTH;

        const auto& points = cell->points;
        auto& mesh = cell->mesh;
        auto& renderable = cell->renderable;

        std::vector<gfxm::vec3>& vertices = cell_remesh_scratch.vertices;
        std::vector<gfxm::vec3>& normals = cell_remesh_scratch.normals;
        std::vector<gfxm::vec3>& tangents = cell_remesh_scratch.tangents;
        std::vector<gfxm::vec3>& bitangents = cell_remesh_scratch.bitangents;
        std::vector<gfxm::vec2>& uvs = cell_remesh_scratch.uvs;
        std::vector<COLOR24>& colors = cell_remesh_scratch.colors;
        std::vector<uint32_t>& indices = cell_remesh_scratch.indices;
        vertices.resize((CELL_SEGMENTS_X + 1) * (CELL_SEGMENTS_Z + 1));
        uvs.resize(vertices.size());
        colors.resize(vertices.size());
        indices.resize(CELL_SEGMENTS_X * CELL_SEGMENTS_Z * 6);
        normals.resize(vertices.size());
        tangents.resize(vertices.size());
        bitangents.resize(vertices.size());

        {
            const float QUAD_WIDTH = CELL_WIDTH / CELL_SEGMENTS_X;
            const float QUAD_DEPTH = CELL_DEPTH / CELL_SEGMENTS_Z;
            for (int y = 0; y <= CELL_SEGMENTS_Z; ++y) {
                for (int x = 0; x <= CELL_SEGMENTS_X; ++x) {
                    float h = sampleHeightQuadSpace(cell_key, x, y);
                    vertices[x + y * (CELL_SEGMENTS_X + 1)] = gfxm::vec3(x * QUAD_WIDTH, h, y * QUAD_DEPTH);
                }
            }

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
            for (int y = 0; y <= CELL_SEGMENTS_Z; ++y) {
                for (int x = 0; x <= CELL_SEGMENTS_X; ++x) {
                    float h = sampleHeightQuadSpace(cell_key, x, y);;

                    //colors[x + y * CELL_SEGMENTS_X] = gfxm::make_rgba32(h, h, h, 1.f);
                    colors[x + y * (CELL_SEGMENTS_X + 1)] = gfxm::make_rgba32(1, 1, 1, 1.f);
                }
            }

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

    float sampleHeightQuadSpace(uint64_t cell_key, int x, int z) {
        int icellx = 0;
        int icellz = 0;
        terrainCellCoordsFromKey(cell_key, icellx, icellz);

        while (x >= CELL_SEGMENTS_X) {
            x -= CELL_SEGMENTS_X;
            icellx += 1;
        }
        while (z >= CELL_SEGMENTS_Z) {
            z -= CELL_SEGMENTS_Z;
            icellz += 1;
        }

        auto cell = getCell(icellx, icellz);
        if (!cell) {
            return .0f;
        }
        x = gfxm::_min(x, CELL_SEGMENTS_X - 1);
        z = gfxm::_min(z, CELL_SEGMENTS_Z - 1);
        return cell->points[x + z * CELL_SEGMENTS_X];
    }
    float sampleHeightWorldPointSpace(int px, int pz) {
        const int icx = floorf(float(px) / CELL_SEGMENTS_X);
        const int icz = floorf(float(pz) / CELL_SEGMENTS_Z);

        auto cell = getCell(icx, icz);
        if (!cell) {
            return .0f;
        }
        const int lpx = px - icx * CELL_SEGMENTS_X;
        const int lpz = pz - icz * CELL_SEGMENTS_Z;
        return cell->points[lpx + lpz * CELL_SEGMENTS_X];
    }
    
    float sampleHeightWorldBilinear(float x, float z) {
        const float QUAD_WIDTH = CELL_WIDTH / CELL_SEGMENTS_X;
        const float QUAD_DEPTH = CELL_DEPTH / CELL_SEGMENTS_Z;

        const float xf = x / QUAD_WIDTH;
        const float zf = z / QUAD_DEPTH;
        const float x0 = floorf(xf);
        const float x1 = ceilf(xf);
        const float z0 = floorf(zf);
        const float z1 = ceilf(zf);
        const float xfract = gfxm::fract(xf);
        const float zfract = gfxm::fract(zf);

        float h0 = sampleHeightWorldPointSpace(x0, z0);
        float h1 = sampleHeightWorldPointSpace(x1, z0);
        float h2 = sampleHeightWorldPointSpace(x1, z1);
        float h3 = sampleHeightWorldPointSpace(x0, z1);

        float l0 = gfxm::lerp(h0, h1, xfract);
        float l1 = gfxm::lerp(h3, h2, xfract);
        return gfxm::lerp(l0, l1, zfract);
    }

    bool hitTestCell(TerrainCell* cell, int icx, int icz, const gfxm::vec3& A, const gfxm::vec3& B, gfxm::vec3& out_pt, float& out_dist) {
        const float QUAD_WIDTH = CELL_WIDTH / CELL_SEGMENTS_X;
        const float QUAD_DEPTH = CELL_DEPTH / CELL_SEGMENTS_Z;
        uint64_t cell_key = terrainCellKey(icx, icz);
        const float CELL_X = icx * CELL_WIDTH;
        const float CELL_Z = icz * CELL_DEPTH;

        const float lminx = gfxm::_min(A.x, B.x) - CELL_X;
        const float lmaxx = gfxm::_max(A.x, B.x) - CELL_X;
        const float lminz = gfxm::_min(A.z, B.z) - CELL_Z;
        const float lmaxz = gfxm::_max(A.z, B.z) - CELL_Z;
        const gfxm::vec3 lclA = A - gfxm::vec3(CELL_X, .0f, CELL_Z);
        const gfxm::vec3 lclB = B - gfxm::vec3(CELL_X, .0f, CELL_Z);

        gfxm::ivec2 iqmin(
            gfxm::iclamp(int(floorf(lminx / QUAD_WIDTH)) - 1, 0, CELL_SEGMENTS_X - 1),
            gfxm::iclamp(int(floorf(lminz / QUAD_DEPTH)) - 1, 0, CELL_SEGMENTS_Z - 1)
        );
        gfxm::ivec2 iqmax(
            gfxm::iclamp(int(floorf(lmaxx / QUAD_WIDTH)) + 1, 0, CELL_SEGMENTS_X - 1),
            gfxm::iclamp(int(floorf(lmaxz / QUAD_DEPTH)) + 1, 0, CELL_SEGMENTS_Z - 1)
        );

        float min_dist = FLT_MAX;
        bool has_hit = false;
        for (int iqz = iqmin.y; iqz <= iqmax.y; ++iqz) {
            for (int iqx = iqmin.x; iqx <= iqmax.x; ++iqx) {
                float y0 = sampleHeightQuadSpace(cell_key, iqx, iqz);
                float y1 = sampleHeightQuadSpace(cell_key, iqx, iqz + 1);
                float y2 = sampleHeightQuadSpace(cell_key, iqx + 1, iqz + 1);
                float y3 = sampleHeightQuadSpace(cell_key, iqx + 1, iqz);

                gfxm::vec3 p0 = gfxm::vec3(iqx * QUAD_WIDTH, y0, iqz * QUAD_DEPTH);
                gfxm::vec3 p1 = gfxm::vec3(iqx * QUAD_WIDTH, y1, (iqz + 1) * QUAD_DEPTH);
                gfxm::vec3 p2 = gfxm::vec3((iqx + 1) * QUAD_WIDTH, y2, (iqz + 1) * QUAD_DEPTH);
                gfxm::vec3 p3 = gfxm::vec3((iqx + 1) * QUAD_WIDTH, y3, iqz * QUAD_DEPTH);

                gfxm::vec3 pt;
                float dist = .0f;
                if (gfxm::intersect_line_triangle(lclA, lclB, p0, p1, p2, pt, dist)) {
                    if (dist < min_dist) {
                        out_pt = pt;
                        min_dist = dist;
                        has_hit = true;
                    }
                }
                if (gfxm::intersect_line_triangle(lclA, lclB, p2, p3, p0, pt, dist)) {
                    if (dist < min_dist) {
                        out_pt = pt;
                        min_dist = dist;
                        has_hit = true;
                    }
                }
            }
        }

        if(has_hit) {
            out_pt += gfxm::vec3(CELL_X, .0f, CELL_Z);
            out_dist = min_dist;
        }
        return has_hit;
    }

    void onTick(float dt, GUI_TICK_ID id) override {
        if (id != GUI_TICK_CUSTOM) {
            return;
        }
        for(uint64_t cell_key : dirty_cells) {
            updateCell(cell_key);
        }
        dirty_cells.clear();
    }

    void toJson(nlohmann::json& json) const override {
        json["version"] = TERRAIN_SPACE_VERSION;

        nlohmann::json& jcells = json["cells"];
        jcells = nlohmann::json::object();
        for (auto& kv : cell_table) {
            nlohmann::json& jcell = jcells[terrainCellKeyToString(kv.first)];

            TerrainCell* cell = const_cast<TerrainCell*>(&kv.second);
            const unsigned char* buf = (unsigned char*)cell->points.data();
            uint64_t bufsz = cell->points.size() * sizeof(cell->points[0]);

            std::string b64;
            base64_encode(buf, bufsz, b64);
            jcell = b64;
        }
    }
    bool fromJson(const nlohmann::json& json) override {
        int version = json["version"].get<int>();
        LOG("TERRAIN SPACE VERSION: " << version);

        auto it_cells = json.find("cells");
        if (it_cells == json.end()) {
            return false;
        }
        const nlohmann::json& jcells = it_cells.value();

        cell_table.clear();
        for (auto it = jcells.begin(); it != jcells.end(); ++it) {
            const std::string& key = it.key();
            int32_t cx = 0, cz = 0;
            if (!terrainCellKeyFromString(key, cx, cz)) {
                continue;
            }

            const nlohmann::json& jcell = it.value();
            std::string str = jcell.get<std::string>();
            std::vector<char> data;
            base64_decode(str.data(), str.size(), data);

            TerrainCell* cell = ensureCell(cx, cz);
            cell->points.resize(CELL_SEGMENTS_X * CELL_SEGMENTS_Z);
            memcpy(cell->points.data(), data.data(), data.size());
        }

        updatePreview();
        return true;
    }

    template<typename T>
    T* enterMode() {
        if (edit_mode) {
            viewport->removeTool(edit_mode.get());
        }
        auto ptr = new T(this);
        edit_mode.reset(ptr);
        viewport->addTool(edit_mode.get());
        return ptr;
    }
public:
    TerrainSceneSpace()
    : SceneSpace("Terrain") {
        gpuGetPipeline()->enableTechnique("Fog", false);

        initData();
        updatePreview();
        
        subscribe([this](const GuiEvt_KeyDown& e) {
            switch (e.vkey) {
            case 0x32: // 2 key
                enterMode<TerrainSculptMode>();
                return;
            case 0x33: { // 3 key
                /*if (edit_mode) {
                    viewport->removeTool(edit_mode.get());
                }
                edit_mode.reset(nullptr);*/
                enterMode<TerrainDecorationMode>();
                return;
            }
            }
            e.consume = false;
            e.invoke_next();
        });
    }

    void initData() {
        cell_table.clear();

        const int width = 10;
        const int depth = 10;
        for (int cz = 0; cz < depth; ++cz) {
            for (int cx = 0; cx < width; ++cx) {
                TerrainCell* cell = ensureCell(cx - width / 2, cz - depth / 2);
                cell->points.resize(CELL_SEGMENTS_X * CELL_SEGMENTS_Z);
                std::fill(cell->points.begin(), cell->points.end(), .0f);
            }
        }
    }

    void updatePreview() {
        for (auto& kv : cell_table) {
            updateCell(kv.first);
        }
    }

    void queryGeometry(const GeometryQuery& q) {
        if (edit_mode) {
            edit_mode->queryGeometry(q);
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
        int icminx = floorf(fminx / CELL_WIDTH);
        int icmaxx = floorf(fmaxx / CELL_WIDTH);
        int icminz = floorf(fminz / CELL_DEPTH);
        int icmaxz = floorf(fmaxz / CELL_DEPTH);

        for (int icz = icminz; icz <= icmaxz; ++icz) {
            for (int icx = icminx; icx <= icmaxx; ++icx) {                
                auto cell = getCell(icx, icz);
                if (!cell) {
                    continue;
                }
                q.bucket->add(&cell->renderable);

                for (int i = 0; i < cell->decorations.size(); ++i) {
                    auto& deco = cell->decorations[i];
                    for (int j = 0; j < deco.renderables.size(); ++j) {
                        if (deco.inst_desc.getInstanceCount() == 0) {
                            continue;
                        }
                        q.bucket->add(&deco.renderables[j]);
                    }
                }
            }
        }
    }
    void enterUi(SceneEditorContext& ctx) override {
        enterMode<TerrainSculptMode>();
    }
    void exitUi(SceneEditorContext& ctx) override {
        if (edit_mode) {
            viewport->removeTool(edit_mode.get());
        }
    }

    TerrainParams getTerrainParams() const {
        return TerrainParams{
            .cell_segments_x = CELL_SEGMENTS_X,
            .cell_segments_z = CELL_SEGMENTS_Z,
            .cell_width = CELL_WIDTH,
            .cell_depth = CELL_DEPTH
        };
    }

    TerrainCell* getCell(uint64_t key) {
        auto it = cell_table.find(key);
        if (it == cell_table.end()) {
            return nullptr;
        }
        return &it->second;
    }
    TerrainCell* getCell(int x, int z) { return getCell(terrainCellKey(x, z)); }
    TerrainCell* getCellWorldSpace(float x, float z) {
        int cx = floorf(x / CELL_WIDTH);
        int cz = floorf(z / CELL_DEPTH);
        return getCell(cx, cz);
    }
    TerrainCell* ensureCell(int x, int z) {
        uint64_t key = terrainCellKey(x, z);
        TerrainCell& cell = cell_table[key];
        return &cell;
    }

    void markCellDirty(int icx, int icz) {
        dirty_cells.insert(terrainCellKey(icx, icz));
        guiScheduleTick(this, .0f, GUI_TICK_CUSTOM);
    }
    void markCellDirtyWorldSpace(float x, float z) {
        int cx = floorf(x / CELL_WIDTH);
        int cz = floorf(z / CELL_DEPTH);
        markCellDirty(cx, cz);
    }
    
    bool hitTest(gfxm::vec3& out) {
        gfxm::ray r = viewport->makeRayFromMousePos();
        const gfxm::vec3 A = r.origin;
        const gfxm::vec3 B = r.origin + r.direction * r.length;
        const gfxm::vec3 D = B - A;

        int cx = int(floorf(A.x / CELL_WIDTH));
        int cz = int(floorf(A.z / CELL_DEPTH));

        const int stepx = D.x > .0f ? 1 : -1;
        const int stepz = D.z > .0f ? 1 : -1;
        const float tdx = D.x != .0f ? CELL_WIDTH / fabsf(D.x) : FLT_MAX;
        const float tdz = D.z != .0f ? CELL_DEPTH / fabsf(D.z) : FLT_MAX;
        float tmaxx = D.x != .0f ? (((stepx > 0 ? cx + 1 : cx) * CELL_WIDTH) - A.x) / D.x : FLT_MAX;
        float tmaxz = D.z != .0f ? (((stepz > 0 ? cz + 1 : cz) * CELL_WIDTH) - A.z) / D.z : FLT_MAX;

        float min_dist = FLT_MAX;
        bool has_hit = false;
        float t0 = .0f;
        while (true) {
            const float t1 = gfxm::_min(tmaxx, tmaxz);

            TerrainCell* cell = getCell(cx, cz);
            if (cell) {
                float ya = A.y + D.y * t0;
                float yb = A.y + D.y * t1;
                float ymin = gfxm::_min(ya, yb);
                float ymax = gfxm::_max(ya, yb);

                if (ymax >= cell->y_min && ymin <= cell->y_max) {
                    gfxm::vec3 P0 = A + D * t0;
                    gfxm::vec3 P1 = A + D * t1;

                    if (hitTestCell(cell, cx, cz, P0, P1, out, min_dist)) {
                        has_hit = true;
                        break;
                    }
                }
            }
            if (t1 >= 1.f) {
                break;
            }


            if (tmaxx < tmaxz) {
                cx += stepx;
                t0 = tmaxx;
                tmaxx += tdx;
            } else {
                cz += stepz;
                t0 = tmaxz;
                tmaxz += tdz;
            }
        }
        return has_hit;
    }
};