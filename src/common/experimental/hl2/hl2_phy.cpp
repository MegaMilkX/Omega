#include "hl2_phy.hpp"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <iomanip>
#include <format>
#include "log/log.hpp"
#include "math/gfxm.hpp"
#include "valve_data/valve_data.hpp"
#include "valve_data/parser/parse.hpp"

#define FREAD(BUFFER, SIZE, COUNT, FILE) \
if (fread(BUFFER, SIZE, COUNT, FILE) != COUNT) { \
    assert(false); \
    return false; \
}
#define FREAD_MSG(BUFFER, SIZE, COUNT, FILE, COMMENT) \
if (fread(BUFFER, SIZE, COUNT, FILE) != COUNT) { \
    LOG_ERR("'" << COMMENT << "': " << "Failed to FREAD"); \
    assert(false); \
    return false; \
}

static void logBytes(void* bytes_, int count) {
    uint8_t* bytes = (uint8_t*)bytes_;
    LOG("bytelog");
    assert(count % 4 == 0);
    for (int j = 0; j < count; j += 4) {
        const uint16_t a8 = *(uint8_t*)&bytes[j];
        const uint16_t b8 = *(uint8_t*)&bytes[j + 1];
        const uint16_t c8 = *(uint8_t*)&bytes[j + 2];
        const uint16_t d8 = *(uint8_t*)&bytes[j + 3];
        const float f32 = *(float*)&bytes[j];
        const int32_t i32 = *(int32_t*)&bytes[j];
        const int16_t i16_0 = *(int16_t*)&bytes[j];
        const int16_t i16_1 = *(int16_t*)&bytes[j + 2];
        const int8_t i8_0 = *(int8_t*)&bytes[j];
        const int8_t i8_1 = *(int8_t*)&bytes[j + 1];
        const int8_t i8_2 = *(int8_t*)&bytes[j + 2];
        const int8_t i8_3 = *(int8_t*)&bytes[j + 3];

        LOG("\t\t" << j << ": " 
            << std::setw(2) << std::hex << std::setfill('0')
            << a8 << " "
            << std::setw(2) << std::hex << std::setfill('0')
            << b8 << " "
            << std::setw(2) << std::hex << std::setfill('0')
            << c8 << " "
            << std::setw(2) << std::hex << std::setfill('0')
            << d8
            << std::dec
            << " | "
            << std::format("{:.3f}", f32)
            << " | "
            << i32
            << " | "
            << i16_0 << ", " << i16_1
            << " | "
            << int(i8_0) << ", " << int(i8_1) << ", " << int(i8_2) << ", " << int(i8_3)
        );
    }
}

bool hl2LoadPHY(const char* path, PHYFile& phy) {
    std::string phypath = MKSTR("experimental/hl2/" << path << ".phy");
    FILE* f = fopen(phypath.c_str(), "rb");
    if (!f) {
        LOG_ERR("Failed to OPEN file '" << phypath << "'");
        return false;
    }

    fseek(f, 0, SEEK_END);
    size_t fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    std::vector<uint8_t> bytes(fsize);

    FREAD(bytes.data(), fsize, 1, f);
    fclose(f);

    phy.head = *(phyheader_t*)&bytes[0];
    LOG("PHY: solidCount: " << phy.head.solidCount);
    LOG("PHY: filesize: " << fsize);

    int offs = 16;
    for (int i = 0; i < phy.head.solidCount; ++i) {
        int32_t size = *(int32_t*)&bytes[offs];
        offs += 4;

        int offs_ = offs;
        int solids_end = offs_ + size;
        const compactsurfaceheader_t& sh = *(compactsurfaceheader_t*)&bytes[offs_];

        constexpr int32_t VPHYID = (int32_t('V') | int32_t('P') << 8 | int32_t('H') << 16 | int32_t('Y') << 24);
        bool is_vphy = false;
        if(sh.vphysicsID == VPHYID) {
            offs_ += sizeof(compactsurfaceheader_t);
            is_vphy = true;
        }

        if (is_vphy) {
            LOG("PHY surface: VPHY");
        } else {
            LOG("PHY surface: Legacy");
        }

        const legacysurfaceheader_t& lfh = *(legacysurfaceheader_t*)&bytes[offs_];
        LOG("Legacy header: ");
        LOG("\tmass_center: " << lfh.mass_center[0] << " " << lfh.mass_center[1] << " " << lfh.mass_center[2]);
        LOG("\trotation_inertia: " << lfh.rotation_inertia[0] << " " << lfh.rotation_inertia[1] << " " << lfh.rotation_inertia[2]);
        LOG("\tmax_deviation: " << lfh.max_deviation);
        LOG("\toffset_ledgetree_root: " << lfh.offset_ledgetree_root);

        phy.inertia_tensor = gfxm::mat3(
            gfxm::vec3(lfh.rotation_inertia[0], .0f, .0f),
            gfxm::vec3(.0f, lfh.rotation_inertia[1], .0f),
            gfxm::vec3(.0f, .0f, lfh.rotation_inertia[2])
        );

        gfxm::vec3 mass_center(
            lfh.mass_center[0],
            lfh.mass_center[1],
            lfh.mass_center[2]
        );
        float scale_to_bsp = 39.3701f;   // inches per meter
        const float scale_to_meters = 1.f / 41.f;
        mass_center = mass_center * scale_to_bsp * scale_to_meters;
        phy.mass_center = -mass_center;

        const phynode_t* nodes = (phynode_t*)&bytes[offs_ + lfh.offset_ledgetree_root];
        int node_offset = offs_ + lfh.offset_ledgetree_root;

        offs_ += sizeof(legacysurfaceheader_t);

        /*
        LOG("Surface: ");
        LOG("\tsize: " << size);
        LOG("\tmodelType: " << sh.modelType);
        LOG("\tversion: " << sh.version);
        LOG("\tsurfaceSize: " << sh.surfaceSize);
        */

        // Root mesh
        {
            const phynode_t& n = nodes[0];
            
            if (n.convexindex == 0) {
                continue;
            }

            const convexsolidheader_t* solid_raw = (convexsolidheader_t*)(((uint8_t*)&n) + n.convexindex);

            phyvertex_t* verts = (phyvertex_t*)(((uint8_t*)solid_raw) + solid_raw->vertices_offset);
            triangledata_t* triangles = (triangledata_t*)(((uint8_t*)solid_raw) + 16);

            int max_vindex = -1;
            std::vector<gfxm::vec3> vertices;
            std::vector<int> indices(solid_raw->triangles_count * 3);

            for(int j = 0; j < solid_raw->triangles_count; ++j) {
                const triangledata_t* tri = &triangles[j];

                const phyvertex_t& v0 = verts[tri->vertex1_index];
                const phyvertex_t& v1 = verts[tri->vertex2_index];
                const phyvertex_t& v2 = verts[tri->vertex3_index];
                max_vindex = std::max(max_vindex, (int)tri->vertex1_index);
                max_vindex = std::max(max_vindex, (int)tri->vertex2_index);
                max_vindex = std::max(max_vindex, (int)tri->vertex3_index);
                /*
                indices[j * 3] = tri->vertex1_index;
                indices[j * 3 + 1] = tri->vertex2_index;
                indices[j * 3 + 2] = tri->vertex3_index;
                */

                vertices.push_back(v0.pos);
                vertices.push_back(v1.pos);
                vertices.push_back(v2.pos);
                indices[j * 3] = j * 3;
                indices[j * 3 + 1] = j * 3 + 1;
                indices[j * 3 + 2] = j * 3 + 2;
            }

            for (int j = 0; j < vertices.size(); ++j) {
                gfxm::vec3& V = vertices[j];
                float scale_to_bsp = 39.3701f;   // inches per meter
                const float scale_to_meters = 1.f / 41.f;
                V = V * scale_to_bsp * scale_to_meters;
                V.y = -V.y;
                V.z = -V.z;
            }

            phy.root_mesh.reset(new CollisionConvexMesh);
            CollisionConvexMesh* out_mesh = phy.root_mesh.get();
            out_mesh->setData(vertices.data(), vertices.size(), indices.data(), indices.size());
        }

        std::stack<const phynode_t*> node_stack;
        std::vector<const phynode_t*> leaf_nodes;
        const phynode_t* n = &nodes[0];
        while (n || !node_stack.empty()) {
            if (!n) {
                if (!node_stack.empty()) {
                    n = node_stack.top();
                    node_stack.pop();
                } else {
                    break;
                }
            }
            if (n->rightnodeindex == 0) {
                leaf_nodes.push_back(n);
                n = 0;
            } else {
                node_stack.push(n + 1);
                node_stack.push((const phynode_t*)(((uint8_t*)&n->rightnodeindex) + n->rightnodeindex));
                n = 0;
            }
        }

        for (int k = 0; k < leaf_nodes.size(); ++k) {
            const auto& n = *leaf_nodes[k];

            if (n.convexindex == 0) {
                continue;
            }

            const convexsolidheader_t* solid_raw = (convexsolidheader_t*)(((uint8_t*)&n) + n.convexindex);

            phyvertex_t* verts = (phyvertex_t*)(((uint8_t*)solid_raw) + solid_raw->vertices_offset);
            triangledata_t* triangles = (triangledata_t*)(((uint8_t*)solid_raw) + 16);

            int max_vindex = -1;
            std::vector<gfxm::vec3> vertices;
            std::vector<int> indices(solid_raw->triangles_count * 3);

            for(int j = 0; j < solid_raw->triangles_count; ++j) {
                const triangledata_t* tri = &triangles[j];

                const phyvertex_t& v0 = verts[tri->vertex1_index];
                const phyvertex_t& v1 = verts[tri->vertex2_index];
                const phyvertex_t& v2 = verts[tri->vertex3_index];
                max_vindex = std::max(max_vindex, (int)tri->vertex1_index);
                max_vindex = std::max(max_vindex, (int)tri->vertex2_index);
                max_vindex = std::max(max_vindex, (int)tri->vertex3_index);
                /*
                indices[j * 3] = tri->vertex1_index;
                indices[j * 3 + 1] = tri->vertex2_index;
                indices[j * 3 + 2] = tri->vertex3_index;
                */

                vertices.push_back(v0.pos);
                vertices.push_back(v1.pos);
                vertices.push_back(v2.pos);
                indices[j * 3] = j * 3;
                indices[j * 3 + 1] = j * 3 + 1;
                indices[j * 3 + 2] = j * 3 + 2;
            }

            for (int j = 0; j < vertices.size(); ++j) {
                gfxm::vec3& V = vertices[j];
                float scale_to_bsp = 39.3701f;   // inches per meter
                const float scale_to_meters = 1.f / 41.f;
                V = V * scale_to_bsp * scale_to_meters;
                V.y = -V.y;
                V.z = -V.z;
            }
            /*
            int vertex_count = max_vindex + 1;
            LOG("vertex_count: " << vertex_count);
            std::vector<gfxm::vec3> vertices(vertex_count);
            for (int j = 0; j < vertex_count; ++j) {
                gfxm::vec3 v = verts[j].pos;

                const float scale = 1.f / 41.f;

                gfxm::vec3 C;
                C.x = n.center.x;
                C.y = n.center.y;
                C.z = n.center.z;
                gfxm::vec3 MC;
                MC.x = mass_center.x;
                MC.y = mass_center.y;
                MC.z = mass_center.z;

                v.y = -v.y;
                v.z = -v.z;

                //C = gfxm::to_mat4(gfxm::angle_axis(gfxm::radian(180.f), gfxm::vec3(1, 0, 0))) * gfxm::vec4(C, 1.f);
                //MC = gfxm::to_mat4(gfxm::angle_axis(gfxm::radian(-90.f), gfxm::vec3(1, 0, 0))) * gfxm::vec4(MC, 1.f);
                vertices[j] = v;// + MC - C;
            }*/

            CollisionConvexMesh* out_mesh = phy.meshes.emplace_back(new CollisionConvexMesh).get();
            out_mesh->setData(vertices.data(), vertices.size(), indices.data(), indices.size());
        }

        offs += size;
    }

    std::string str(&bytes[offs], bytes.data() + bytes.size());
    LOG(str);
    valve_data vd;
    if (valve::parse_phy(vd, str.data(), str.size())) {
        auto it = vd.find("editparams");
        if (it != vd.end()) {
            float totalmass = it.value().find("totalmass").value().to_float_tmp();
            if(totalmass == .0f) { totalmass = 1.f; }
            phy.total_mass = totalmass;
        }
    }

    return true;
}