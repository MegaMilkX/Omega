#include "csg_core.hpp"

#include "math/intersection.hpp"
#include "csg_common.hpp"


bool csgIntersectLineFace(const gfxm::vec3& e0, const gfxm::vec3& e1, csgFace* face, gfxm::vec3& out) {
    assert(face->vertexCount() >= 3);

    //const auto& p0 = face->vertices[0].position;
    //const auto& p1 = face->vertices[1].position;
    //const auto& p2 = face->vertices[2].position;
    const auto& p0 = face->getWorldVertexPos(0);
    const auto& p1 = face->getWorldVertexPos(1);
    const auto& p2 = face->getWorldVertexPos(2);
    gfxm::vec3 N = face->N;

    gfxm::vec3 line_plane_intersection;
    gfxm::vec3 edge_V = (e1 - e0);
    float dot_test = (gfxm::dot(N, edge_V));
    float t;
    if (fabsf(dot_test) > FLT_EPSILON) {
        t = gfxm::dot(N, (p0 - e0) / dot_test);
        line_plane_intersection = e0 + edge_V * t;
        if (t < .0f || t > 1.0f) {
            return false;
        }
    } else {
        return false;
    }

    for (int i = 1; i < face->vertexCount(); ++i) {
        const auto& p0 = face->getWorldVertexPos(i - 1);
        const auto& p1 = face->getWorldVertexPos(i);
        gfxm::vec3 c = gfxm::cross(line_plane_intersection - p0, p1 - p0);
        bool inside = gfxm::dot(c, N) <= 0;
        if (!inside) {
            return false;
        }
    }
    {
        const auto& p0 = face->getWorldVertexPos(face->vertexCount() - 1);
        const auto& p1 = face->getWorldVertexPos(0);
        gfxm::vec3 c = gfxm::cross(line_plane_intersection - p0, p1 - p0);
        bool inside = gfxm::dot(c, N) <= 0;
        if (!inside) {
            return false;
        }
    }

    out = line_plane_intersection;

    return true;
}
bool csgIntersectFaceFace(csgFace* face_a, csgFace* face_b, std::vector<csgVertex>& out_vertices) {
    gfxm::vec3 p;
    std::vector<csgVertex> new_vertices;
    for (int k = 1; k < face_b->vertexCount(); ++k) {
        auto& p0 = face_b->getWorldVertexPos(k - 1);
        auto& p1 = face_b->getWorldVertexPos(k);
        if (!csgIntersectLineFace(p0, p1, face_a, p)) {
            continue;
        }
        csgVertex v;
        v.position = p;
        v.faces.insert(face_a);
        v.faces.insert(face_b);
        out_vertices.push_back(v);
    }
    auto& p0 = face_b->getWorldVertexPos(face_b->vertexCount() - 1);
    auto& p1 = face_b->getWorldVertexPos(0);
    if (csgIntersectLineFace(p0, p1, face_a, p)) {
        csgVertex v;
        v.position = p;
        v.faces.insert(face_a);
        v.faces.insert(face_b);
        out_vertices.push_back(v);
    }

    return true;
}

void csgPrepareCut(csgBrushShape* shape, const gfxm::vec3& cut_plane_N, float cut_plane_D, csgShapeCutData& out) {
    out.clear();
    out.shape = shape;
    out.cut_plane_N = cut_plane_N;
    out.cut_plane_D = cut_plane_D;

    for (int i = 0; i < shape->control_points.size(); ++i) {
        auto cp = shape->control_points[i].get();
        cp->tmp_relation = csgCheckVertexRelation(shape->world_space_vertices[cp->index], cut_plane_N, cut_plane_D);
        if (cp->tmp_relation == CSG_RELATION_FRONT) {
            out.front_control_points.push_back(cp);
        } else if(cp->tmp_relation == CSG_RELATION_BACK) {
            out.back_control_points.push_back(cp);
        } else {
            out.aligned_control_points.push_back(cp);
        }
    }

    for (int i = 0; i < shape->faces.size(); ++i) {
        int intersection_count = 0;
        csgFace* face = shape->faces[i].get();

        static_assert(CSG_RELATION_OUTSIDE == 0 && CSG_RELATION_INSIDE == 1 && CSG_RELATION_ALIGNED == 2, "Relation enum - unexpected values");
        int count[3];
        count[CSG_RELATION_INSIDE] = 0;
        count[CSG_RELATION_ALIGNED] = 0;
        count[CSG_RELATION_OUTSIDE] = 0;
        for (int k = 0; k < face->vertexCount(); ++k) {
            auto cp = face->control_points[k];
            count[cp->tmp_relation] += 1;
        }

        if (count[CSG_RELATION_INSIDE] == 0) {
            out.discarded_faces.push_back(i);
            continue;
        } else if(count[CSG_RELATION_INSIDE] < face->control_points.size()) {
            auto& face_cut_data = out.cut_faces[i];
            face_cut_data.face_id = i;
        } else {
            continue;
        }

        gfxm::vec3 p;
        for (int k = 0; k < face->vertexCount(); ++k) {
            int j = (k + 1) % face->vertexCount();
            auto& p0 = face->getWorldVertexPos(k);
            auto& p1 = face->getWorldVertexPos(j);
            if (!gfxm::intersect_ray_plane_point(p0, p1, cut_plane_N, cut_plane_D, p)) {
                continue;
            }
            out.preview_lines.push_back(p);

            ++intersection_count;
            if (intersection_count == 2) {
                break;
            }
        }
    }
}
void csgPerformCut(csgShapeCutData& data) {
    csgBrushShape* shape = data.shape;
    assert(shape);

    // TODO
    std::vector<csgVertex*> *discarded_control_points = &data.front_control_points;

    shape->faces.push_back(std::unique_ptr<csgFace>(new csgFace));
    csgFace* new_face = shape->faces.back().get();

    gfxm::mat4 inv_transform = gfxm::inverse(shape->transform);

    new_face->shape = shape;
    new_face->N = data.cut_plane_N;
    new_face->D = data.cut_plane_D;
    new_face->lclN =  inv_transform * gfxm::vec4(data.cut_plane_N, .0f);
    gfxm::vec3 lclP = inv_transform * gfxm::vec4(data.cut_plane_N * data.cut_plane_D, 1.f);
    new_face->lclD = gfxm::dot(new_face->lclN, lclP);
    new_face->material = 0;
    new_face->uv_offset = gfxm::vec2(0,0);
    new_face->uv_scale = gfxm::vec2(1,1);

    // TODO:
    // First find all outside vertices
    // Memorize all faces that use those vertices
    // Check if those faces need to be discarded or cut
    // As a result, faces that are inside do not need to be processed at all

    std::vector<csgFace*> intersected_faces;
    for (auto& kv : data.cut_faces) {
        intersected_faces.push_back(shape->faces[kv.first].get());
    }
    for (int i = 0; i < intersected_faces.size(); ++i) {
        for (int j = i; j < intersected_faces.size(); ++j) {
            auto face_b = intersected_faces[i];
            auto face_c = intersected_faces[j];
            csgVertex v;
            if (!csgMakeVertexLocal(new_face, face_b, face_c, &v)) {
                continue;
            }
            // NOTE: No need to handle duplicates, they are not possible in this case I'm pretty sure

            CSG_RELATION_TYPE rel = CSG_RELATION_BACK;
            for (int k = 0; k < intersected_faces.size(); ++k) {
                auto& face = intersected_faces[k];
                rel = csgCheckVertexRelation(v.position, face->lclN, face->lclD);
                if (rel == CSG_RELATION_FRONT) {
                    break;
                }
            }
            if (rel == CSG_RELATION_FRONT) {
                continue;
            }

            csgVertex* p_vertex = shape->_createControlPoint(v);

            new_face->tmp_control_points.insert(p_vertex);
            face_b->tmp_control_points.insert(p_vertex);
            face_c->tmp_control_points.insert(p_vertex);
        }
    }

    csgFindEdges(shape, new_face);
    csgFixWindingLocal(shape, new_face);

    csgUpdateFaceNormals(new_face);

    for (auto face : intersected_faces) {
        face->tmp_control_points.insert(face->control_points.begin(), face->control_points.end());
        for (auto cp : *discarded_control_points) {
            face->tmp_control_points.erase(cp);
        }
        for (auto cp : data.aligned_control_points) {
            face->tmp_control_points.erase(cp);
        }
        csgFindEdges(shape, face);
        csgFixWindingLocal(shape, face);
        csgUpdateFaceNormals(face);
    }

    std::sort(data.discarded_faces.begin(), data.discarded_faces.end(), [](int a, int b)->bool {
        return a > b;
    });
    for (auto fidx : data.discarded_faces) {
        auto pface = shape->faces[fidx].get();
        for (auto cp : pface->control_points) {
            cp->faces.erase(pface);
        }
        shape->faces.erase(shape->faces.begin() + fidx);
    }

    for (auto cp : *discarded_control_points) {
        cp->faces.clear();
    }
    for (auto cp : data.aligned_control_points) {
        cp->faces.clear();
    }
    for (int i = 0; i < shape->control_points.size(); ++i) {
        if (shape->control_points[i]->faces.empty()) {
            shape->control_points.erase(shape->control_points.begin() + i);
            --i;
            continue;
        }
    }
    for (int i = 0; i < shape->control_points.size(); ++i) {
        shape->control_points[i]->index = i;
    }

    data.shape->invalidate();
}

/*
void csgIntersectShapes(csgBrushShape* shape_a, csgBrushShape* shape_b) {
    //auto face_a = &shape_a->faces[6];
    //auto face_b = &shape_b->faces[4];
    //csgIntersectFaceFace(face_a, face_b);
    //csgIntersectFaceFace(face_b, face_a);

    std::unordered_map<csgFace*, std::vector<csgVertex>> new_vertices_per_face;
    for (int i = 0; i < shape_a->faces.size(); ++i) {
        auto face_a = &shape_a->faces[i];
        for (int j = 0; j < shape_b->faces.size(); ++j) {
            auto face_b = &shape_b->faces[j];
            std::vector<csgVertex> new_vertices;
            csgIntersectFaceFace(face_a, face_b, new_vertices);
            csgIntersectFaceFace(face_b, face_a, new_vertices);

            if (!new_vertices.empty()) {
                assert(new_vertices.size() == 2);
                auto& a = new_vertices_per_face[face_a];
                auto& b = new_vertices_per_face[face_b];
                a.insert(a.end(), new_vertices.begin(), new_vertices.end());
                b.insert(b.end(), new_vertices.begin(), new_vertices.end());
            }
        }
    }
    for (auto& kv : new_vertices_per_face) {
        kv.first->vertices.insert(kv.first->vertices.end(), kv.second.begin(), kv.second.end());
    }
}*/


void csgSplit(csgFragment* frag, csgFace* face, csgFragment* front, csgFragment* back) {
    std::unordered_map<CSG_RELATION_TYPE, csgFragment*> pieces;
    pieces[CSG_RELATION_FRONT] = front;
    pieces[CSG_RELATION_BACK] = back;
    for (auto& kv : pieces) {
        csgFragment* piece = kv.second;
        piece->face = frag->face;
        piece->vertices.clear();
        piece->front_volume = frag->front_volume;
        piece->back_volume = frag->back_volume;
    }

    int vertex_count = frag->vertices.size();
    for (int i = 0; i < vertex_count; ++i) {
        size_t j = (i + 1) % vertex_count;
        csgVertex v0 = frag->vertices[i];
        csgVertex v1 = frag->vertices[j];
        CSG_RELATION_TYPE c0 = csgCheckVertexRelation(&v0, face);
        CSG_RELATION_TYPE c1 = csgCheckVertexRelation(&v1, face);
        if (c0 != c1) {
            csgEdgeTmp edge;
            if (!csgFindEdge(&v0, &v1, &edge)) {
                // TODO: not sure how to handle
                //assert(false);
                //pieces[c0]->vertices.push_back(v0);
                continue;
            }
            csgVertex v;
            if (!csgMakeVertex(edge.faces[0], edge.faces[1], face, &v)) {
                // TODO: not sure how to handle
                //assert(false);
                //pieces[c0]->vertices.push_back(v0);
                continue;
            }
            float t = gfxm::dot(gfxm::normalize(v1.position - v0.position), v.position - v0.position) / (v1.position - v0.position).length();
            v.normal = (gfxm::slerp(v0.normal, v1.normal, t));
            v.uv = gfxm::lerp(v0.uv, v1.uv, t);

            if (c0 == CSG_RELATION_ALIGNED) {
                pieces[c1]->vertices.push_back(v);
            } else if(c1 == CSG_RELATION_ALIGNED) {
                pieces[c0]->vertices.push_back(v0);
                pieces[c0]->vertices.push_back(v);
            } else {
                pieces[c0]->vertices.push_back(v0);
                pieces[c0]->vertices.push_back(v);
                pieces[c1]->vertices.push_back(v);
            }
        } else {
            // TODO: not sure how to handle
            if (c0 == CSG_RELATION_ALIGNED) {
                //pieces[CSG_RELATION_BACK]->vertices.push_back(v0);
            } else {
                pieces[c0]->vertices.push_back(v0);
            }
        }
    }
}
std::vector<csgFragment> csgCarve(csgFragment frag, csgBrushShape* shape, int face_idx) {
    std::vector<csgFragment> pieces;

    if (face_idx >= shape->faces.size()) {
        return { frag };
    }

    csgFace* face = shape->faces[face_idx].get();
    CSG_RELATION_TYPE rel = csgCheckFragmentRelation(&frag, face);
    switch (rel) {
    case CSG_RELATION_FRONT:
        frag.rel_type = CSG_RELATION_OUTSIDE;
        return { frag };
    case CSG_RELATION_ALIGNED:
    case CSG_RELATION_REVERSE_ALIGNED:
        frag.rel_type = rel;
    case CSG_RELATION_BACK:
        return csgCarve(std::move(frag), shape, face_idx + 1);
    case CSG_RELATION_SPLIT: {
        csgFragment front;
        csgFragment back;
        front.N = frag.N;
        back.N = frag.N;
        csgSplit(&frag, face, &front, &back);
        back.rel_type = frag.rel_type;
        auto rest = csgCarve(std::move(back), shape, face_idx + 1);

        if (rest.size() == 1 && rest[0].rel_type == CSG_RELATION_OUTSIDE) {
            frag.rel_type = CSG_RELATION_OUTSIDE;
            return { frag };
        }

        front.rel_type = CSG_RELATION_OUTSIDE;
        rest.push_back(std::move(front));
        return rest;
    }
    }
    
    assert(false);
    return {};
}
bool csgCheckShapeOrder(const csgBrushShape* shape_a, const csgBrushShape* shape_b) {
    return shape_a->uid < shape_b->uid;
}
void csgRebuildFragments(csgBrushShape* shape) {
    for (auto& fptr : shape->faces) {
        auto& face = *fptr.get();
        face.fragments.clear();
        face.fragments.emplace_back();
        {
            csgFragment* fragment = &face.fragments.back();
            fragment->face = fptr.get();
            fragment->N = face.N;
            fragment->vertices.resize(face.control_points.size());
            for (int i = 0; i < face.control_points.size(); ++i) {
                fragment->vertices[i] = *face.control_points[i];
                fragment->vertices[i].position = shape->world_space_vertices[face.control_points[i]->index];
                fragment->vertices[i].normal = face.normals[i];
                fragment->vertices[i].uv = face.uvs[i];
            }
            //fragment->vertices = face.vertices;
            fragment->front_volume = CSG_VOLUME_SOLID;
            fragment->back_volume = shape->volume_type;
            fragment->rgba = shape->rgba;
        }

        for(auto other : shape->intersecting_sorted) {
            if (!csgIntersectAabb(face.aabb_world, other->aabb)) {
                continue;
            }

            int frag_count = face.fragments.size();
            for (int frag_idx = frag_count - 1; frag_idx >= 0; --frag_idx) {
                bool is_first = csgCheckShapeOrder(shape, other); // TODO
                csgFragment frag = std::move(face.fragments[frag_idx]);
                face.fragments.erase(face.fragments.begin() + frag_idx);

                frag.rel_type = CSG_RELATION_INSIDE;
                std::vector<csgFragment> frags = csgCarve(std::move(frag), other, 0);
                for (auto& frag : frags) {
                    bool keep_frag = true;
                    switch (frag.rel_type) {
                    case CSG_RELATION_INSIDE:
                        if (is_first) {
                            frag.back_volume = other->volume_type;
                        }
                        frag.front_volume = other->volume_type;
                        break;
                    case CSG_RELATION_ALIGNED:
                        if (is_first) {
                            keep_frag = false;
                        }
                        break;
                    case CSG_RELATION_REVERSE_ALIGNED:
                        if (is_first) {
                            keep_frag = false;
                        } else {
                            frag.front_volume = other->volume_type;
                        }
                        break;
                    }
                    if (keep_frag) {
                        face.fragments.emplace_back(std::move(frag));
                    }
                }
            }
        }
    }
}
