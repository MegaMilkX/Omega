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

void csgMakeTangentsBitangents(
    const gfxm::vec3* vertices, const gfxm::vec3* normals,
    const gfxm::vec2* uvs, int vertex_count,
    const uint32_t* indices, int index_count,
    std::vector<gfxm::vec3>& out_tan,
    std::vector<gfxm::vec3>& out_bitan
) {
    assert(index_count % 3 == 0);
    out_tan.resize(vertex_count);
    out_bitan.resize(vertex_count);
    for (int i = 0; i < index_count; i += 3) {
        uint32_t a = indices[i];
        uint32_t b = indices[i + 1];
        uint32_t c = indices[i + 2];

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

        out_tan[a] += sdir;
        out_tan[b] += sdir;
        out_tan[c] += sdir;
        out_bitan[a] += tdir;
        out_bitan[b] += tdir;
        out_bitan[c] += tdir;
    }
    for (int i = 0; i < vertex_count; ++i) {
        out_tan[i] = gfxm::normalize(out_tan[i]);
        out_bitan[i] = gfxm::normalize(out_bitan[i]);
    }
}

void csgTriangulateShape(csgBrushShape* shape, std::vector<std::unique_ptr<csgMeshData>>& out_meshes) {
    std::map<csgMaterial*, csgMeshData*> by_material;

    for (int i = 0; i < shape->faces.size(); ++i) {
        csgMaterial* mat = shape->material;
        if (shape->faces[i]->material) {
            mat = shape->faces[i]->material;
        }

        csgMeshData* mesh = 0;
        auto it_mesh = by_material.find(mat);
        if (it_mesh == by_material.end()) {
            mesh = new csgMeshData;
            mesh->material = mat;
            by_material.insert(std::make_pair(mat, mesh));
        } else {
            mesh = it_mesh->second;
        }
        int base_index = mesh->vertices.size();

        for (int j = 0; j < shape->faces[i]->fragments.size(); ++j) {
            auto& frag = shape->faces[i]->fragments[j];
            if (frag.back_volume == frag.front_volume) {
                continue;
            }
            float normal_mul = 1.f;
            bool inverse_winding = false;
            if (frag.back_volume == CSG_VOLUME_EMPTY) {
                normal_mul = -1.f;
                inverse_winding = true;
            }
            for (int k = 0; k < frag.vertices.size(); ++k) {
                mesh->vertices.push_back(frag.vertices[k].position);
                mesh->normals.push_back(normal_mul * frag.vertices[k].normal);
                mesh->colors.push_back(shape->rgba);
                mesh->uvs.push_back(frag.vertices[k].uv);
            }

            std::vector<uint32_t> indices_frag;
            csgTriangulateFragment(&frag, base_index, indices_frag);
            if (inverse_winding) {
                std::reverse(indices_frag.begin(), indices_frag.end());
            }
            mesh->indices.insert(mesh->indices.end(), indices_frag.begin(), indices_frag.end());

            csgMakeTangentsBitangents(
                mesh->vertices.data(), mesh->normals.data(),
                mesh->uvs.data(), mesh->vertices.size(),
                mesh->indices.data(), mesh->indices.size(),
                mesh->tangents, mesh->bitangents
            );

            base_index = mesh->vertices.size();
        }
    }

    out_meshes.clear();
    for (auto& kv : by_material) {
        if (kv.second->vertices.empty()) {
            continue;
        }
        kv.second->uvs_lightmap.resize(kv.second->vertices.size());
        out_meshes.push_back(std::unique_ptr<csgMeshData>(kv.second));
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
                mesh.normals.push_back(normal_mul * frag.vertices[k].normal);
                mesh.colors.push_back(shape->rgba);
                mesh.uvs.push_back(frag.vertices[k].uv);
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
        csgMakeTangentsBitangents(
            mesh.vertices.data(), mesh.normals.data(),
            mesh.uvs.data(), mesh.vertices.size(),
            mesh.indices.data(), mesh.indices.size(),
            mesh.tangents, mesh.bitangents
        );
    }

    for (auto kv = mesh_data.begin(); kv != mesh_data.end();) {
        if (kv->second.vertices.empty()) {
            kv = mesh_data.erase(kv);
        } else {
            ++kv;
        }
    }
}

