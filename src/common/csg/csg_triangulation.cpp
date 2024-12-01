#include "csg_triangulation.hpp"
#include "csg_common.hpp"
#include "csg_brush_shape.hpp"


void csgTriangulateFragment(const csgFragment* frag, int base_index, std::vector<uint32_t>& out_indices) {
    int vertex_count = frag->vertices.size();
    std::vector<uint32_t> indices;
    indices.resize(vertex_count);
    for (int i = 0; i < vertex_count; ++i) {
        indices[i] = base_index + i;
    }
    for (;;) {
        int n = indices.size();
        if (n < 3) {
            break;
        }
        if (n == 3) {
            out_indices.push_back(indices[0]);
            out_indices.push_back(indices[1]);
            out_indices.push_back(indices[2]);
            break;
        }
        out_indices.push_back(indices[0]);
        out_indices.push_back(indices[1]);
        out_indices.push_back(indices[2]);

        out_indices.push_back(indices[2]);
        out_indices.push_back(indices[3]);
        out_indices.push_back(indices[4 % n]);
        indices.erase(indices.begin() + 3);
        indices.erase(indices.begin() + 1);
    }
}


void csgMakeShapeTriangles(
    csgBrushShape* shape,
    std::unordered_map<csgMaterial*, csgMeshData>& mesh_data
) {
    for (int i = 0; i < shape->faces.size(); ++i) {
        csgMaterial* mat = shape->material;
        if (shape->faces[i]->material) {
            mat = shape->faces[i]->material;
        }
        auto& mesh = mesh_data[mat];
        mesh.material = mat;

        int base_index = mesh.vertices.size();
        gfxm::vec3 face_origin;
        gfxm::mat3 orient = csgMakeFaceOrientationMatrix(shape->faces[i].get(), face_origin);

        for (int j = 0; j < shape->faces[i]->fragments.size(); ++j) {
            auto& frag = shape->faces[i]->fragments[j];
            if (frag.back_volume == frag.front_volume) {
                continue;
            }
            float normal_mul = 1.f;
            bool inverse_winding = false;
            if (frag.front_volume == CSG_VOLUME_EMPTY) {

            } else if(frag.back_volume == CSG_VOLUME_EMPTY) {
                normal_mul = -1.f;
                inverse_winding = true;
            }
            for (int k = 0; k < frag.vertices.size(); ++k) {
                mesh.vertices.push_back(frag.vertices[k].position);
                //mesh.normals.push_back(normal_mul * frag.face->N);
                mesh.normals.push_back(normal_mul * frag.vertices[k].normal);
                mesh.colors.push_back(shape->rgba);
                mesh.uvs.push_back(frag.vertices[k].uv);

                /*
                gfxm::vec2& uv_scale = shape->faces[i].uv_scale;
                gfxm::vec2& uv_offset = shape->faces[i].uv_offset;
                gfxm::vec2 uv = csgProjectVertexXY(orient, face_origin, frag.vertices[k].position);
                uv.x += uv_offset.x;
                uv.y += uv_offset.y;
                uv.x *= 1.f / uv_scale.x;
                uv.y *= 1.f / uv_scale.y;
                mesh.uvs.push_back(uv);*/
            }
            std::vector<uint32_t> indices_frag;
            csgTriangulateFragment(&frag, base_index, indices_frag);
            if (inverse_winding) {
                std::reverse(indices_frag.begin(), indices_frag.end());
            }
            mesh.indices.insert(mesh.indices.end(), indices_frag.begin(), indices_frag.end());
            base_index = mesh.vertices.size();
        }
    }

    for (auto& kv : mesh_data) {
        auto& mesh = kv.second;
        assert(mesh.indices.size() % 3 == 0);
        mesh.tangents.resize(mesh.vertices.size());
        mesh.bitangents.resize(mesh.vertices.size());
        for (int l = 0; l < mesh.indices.size(); l += 3) {
            uint32_t a = mesh.indices[l];
            uint32_t b = mesh.indices[l + 1];
            uint32_t c = mesh.indices[l + 2];

            gfxm::vec3 Va = mesh.vertices[a];
            gfxm::vec3 Vb = mesh.vertices[b];
            gfxm::vec3 Vc = mesh.vertices[c];
            gfxm::vec3 Na = mesh.normals[a];
            gfxm::vec3 Nb = mesh.normals[b];
            gfxm::vec3 Nc = mesh.normals[c];
            gfxm::vec2 UVa = mesh.uvs[a];
            gfxm::vec2 UVb = mesh.uvs[b];
            gfxm::vec2 UVc = mesh.uvs[c];

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

            mesh.tangents[a] += sdir;
            mesh.tangents[b] += sdir;
            mesh.tangents[c] += sdir;
            mesh.bitangents[a] += tdir;
            mesh.bitangents[b] += tdir;
            mesh.bitangents[c] += tdir;
        }
        for (int k = 0; k < mesh.vertices.size(); ++k) {
            mesh.tangents[k] = gfxm::normalize(mesh.tangents[k]);
            mesh.bitangents[k] = gfxm::normalize(mesh.bitangents[k]);
        }
    }
}

