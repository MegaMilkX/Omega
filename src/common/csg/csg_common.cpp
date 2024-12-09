#include "csg_common.hpp"
#include "csg_face.hpp"
#include "csg_brush_shape.hpp"


bool csgApproxEqual(float a, float b) {
    return int(roundf(a * 1000)) == int(roundf(b * 1000));
}

bool csgIntersectAabb(const gfxm::aabb& a, const gfxm::aabb& b) {
    return a.from.x < b.to.x &&
        a.from.y < b.to.y &&
        a.from.z < b.to.z &&
        b.from.x < a.to.x &&
        b.from.y < a.to.y &&
        b.from.z < a.to.z;
}
gfxm::vec2 csgProjectVertexXY(const gfxm::mat3& m, const gfxm::vec3& origin, const gfxm::vec3& v) {
    gfxm::vec2 v2d;
    v2d.x = gfxm::dot(m[0], v - origin);
    v2d.y = gfxm::dot(m[1], v - origin);
    return v2d;
}

bool csgCheckVerticesCoplanar(csgFace* face) {
    assert(face->control_points.size() >= 3);
    for (auto cp : face->control_points) {
        if (CSG_RELATION_ALIGNED != csgCheckVertexRelation(face->shape->world_space_vertices[cp->index], face)) {
            return false;
        }
    }
    return true;
}
bool csgCheckFaceConvex(csgFace* face) {
    assert(face->control_points.size() >= 3);
    float sign = .0f;
    for (int i = 0; i < face->control_points.size(); ++i) {
        int a = i;
        int b = (i + 1) % face->control_points.size();
        int c = (i + 2) % face->control_points.size();
        gfxm::vec3 p0 = face->control_points[a]->position;
        gfxm::vec3 p1 = face->control_points[b]->position;
        gfxm::vec3 p2 = face->control_points[c]->position;
        gfxm::vec3 C = gfxm::cross(p2 - p1, p1 - p0);
        
        float d = gfxm::dot(face->N, C);/*
        if (fabsf(d) <= FLT_EPSILON) {
            // Colinear edges are not allowed too
            return false;
        }*/
        float s = d > .0f ? 1.f : -1.f;
        if (s * sign < .0f) {
            return false;
        } else {
            sign = s;
        }
    }
    return true;
}

CSG_RELATION_TYPE csgCheckVertexRelation(const gfxm::vec3& v, const gfxm::vec3& planeN, float planeD) {
    //float d = gfxm::dot(gfxm::vec4(face->plane->N, face->plane->D), gfxm::vec4(v->position, 1.f));
    
    gfxm::vec3 planePoint = planeN * planeD;
    gfxm::vec3 pt_dir = v - planePoint;
    float d = gfxm::dot(gfxm::normalize(pt_dir), planeN);

    //float d = gfxm::dot(face->plane->N, v->position) + face->plane->D;

    if (csgApproxEqual(d, .0f)) {
        return CSG_RELATION_ALIGNED;
    } else if (d > .0f) {
        return CSG_RELATION_FRONT;
    } else {
        return CSG_RELATION_BACK;
    }
}
CSG_RELATION_TYPE csgCheckVertexRelation(const gfxm::vec3& v, csgFace* face) {
    //float d = gfxm::dot(gfxm::vec4(face->plane->N, face->plane->D), gfxm::vec4(v->position, 1.f));
    
    gfxm::vec3 planePoint = face->N * face->D;
    gfxm::vec3 pt_dir = v - planePoint;
    float d = gfxm::dot(gfxm::normalize(pt_dir), face->N);

    //float d = gfxm::dot(face->plane->N, v->position) + face->plane->D;

    if (csgApproxEqual(d, .0f)) {
        return CSG_RELATION_ALIGNED;
    } else if (d > .0f) {
        return CSG_RELATION_FRONT;
    } else {
        return CSG_RELATION_BACK;
    }
}
CSG_RELATION_TYPE csgCheckVertexRelation(csgVertex* v, csgFace* face) {
    //float d = gfxm::dot(gfxm::vec4(face->plane->N, face->plane->D), gfxm::vec4(v->position, 1.f));
    
    gfxm::vec3 planePoint = face->N * face->D;
    gfxm::vec3 pt_dir = v->position - planePoint;
    float d = gfxm::dot(gfxm::normalize(pt_dir), face->N);

    //float d = gfxm::dot(face->plane->N, v->position) + face->plane->D;

    if (csgApproxEqual(d, .0f)) {
        return CSG_RELATION_ALIGNED;
    } else if (d > .0f) {
        return CSG_RELATION_FRONT;
    } else {
        return CSG_RELATION_BACK;
    }
}
CSG_RELATION_TYPE csgCheckFragmentRelation(csgFragment* frag, csgFace* face) {
    std::unordered_map<CSG_RELATION_TYPE, int> count;
    count[CSG_RELATION_INSIDE] = 0;
    count[CSG_RELATION_ALIGNED] = 0;
    count[CSG_RELATION_OUTSIDE] = 0;
    for (csgVertex& v : frag->vertices) {
        count[csgCheckVertexRelation(&v, face)] += 1;
    }
    if (count[CSG_RELATION_OUTSIDE] > 0 &&
        count[CSG_RELATION_INSIDE] > 0) {
        return CSG_RELATION_SPLIT;
    }
    else if (count[CSG_RELATION_OUTSIDE] == 0 && count[CSG_RELATION_INSIDE] == 0) {
        float d = gfxm::dot(face->N, frag->face->N);
        if (d < .0f) {
            return CSG_RELATION_REVERSE_ALIGNED;
        }
        else {
            return CSG_RELATION_ALIGNED;
        }
    }
    else if (count[CSG_RELATION_INSIDE] > 0) {
        return CSG_RELATION_INSIDE;
    }
    else {
        return CSG_RELATION_OUTSIDE;
    }
}
CSG_RELATION_TYPE csgCheckVertexShapeRelation(csgVertex* v, csgBrushShape* shape) {
    int n = shape->faces.size();
    CSG_RELATION_TYPE rel = CSG_RELATION_INSIDE;
    for (int i = 0; i < n; ++i) {
        switch (csgCheckVertexRelation(v, shape->faces[i].get())) {
        case CSG_RELATION_FRONT:
            return CSG_RELATION_OUTSIDE;
        case CSG_RELATION_ALIGNED:
            rel = CSG_RELATION_ALIGNED;
        }
    }
    return rel;
}

void csgTransformShape(csgBrushShape* shape, const gfxm::mat4& transform) {
    shape->transform = transform;
    shape->invalidate();
}
void csgUpdateShapeWorldSpace(csgBrushShape* shape) {
    bool aabb_initialized = false;
    shape->world_space_vertices.resize(shape->control_points.size());
    gfxm::mat4 inv_transp = gfxm::transpose(gfxm::inverse(shape->transform));
    for (int i = 0; i < shape->control_points.size(); ++i) {
        auto& lclv = *shape->control_points[i].get();
        gfxm::vec3 world_v = shape->transform * gfxm::vec4(lclv.position, 1.f);
        shape->world_space_vertices[i] = world_v;
        
        if (aabb_initialized) {
            gfxm::expand_aabb(shape->aabb, world_v);
        } else {
            shape->aabb.from = world_v;
            shape->aabb.to = world_v;
            aabb_initialized = true;
        }

        for (auto& fptr : shape->faces) {
            auto& f = *fptr.get();
            f.normals.resize(f.lcl_normals.size());
            for (int j = 0; j < f.lcl_normals.size(); ++j) {
                auto& N = f.lcl_normals[j];
                f.normals[j] = inv_transp * gfxm::vec4(N, .0f);
                f.normals[j] = gfxm::normalize(f.normals[j]);
            }
        }
    }
    if (aabb_initialized) {
        shape->aabb.from -= gfxm::vec3(.001f, .001f, .001f);
        shape->aabb.to += gfxm::vec3(.001f, .001f, .001f);
    }

    for (auto& fptr : shape->faces) {
        auto& face = *fptr.get();
        assert(face.vertexCount());

        face.mid_point = gfxm::vec3(0, 0, 0);
        for (int j = 0; j < face.vertexCount(); ++j) {
            auto& v = face.getWorldVertexPos(j);
            face.mid_point += v;
        }
        face.mid_point /= (float)face.vertexCount();

        face.aabb_world.from = face.getWorldVertexPos(0);
        face.aabb_world.to = face.getWorldVertexPos(0);
        for (int j = 1; j < face.vertexCount(); ++j) {
            gfxm::expand_aabb(face.aabb_world, face.getWorldVertexPos(j));
        }
        face.aabb_world.from -= gfxm::vec3(.001f, .001f, .001f);
        face.aabb_world.to += gfxm::vec3(.001f, .001f, .001f);


        gfxm::mat4 transp_inv = gfxm::transpose(gfxm::inverse(shape->transform));
        face.N = transp_inv * gfxm::vec4(face.lclN, .0f);
        face.N = gfxm::normalize(face.N);
        gfxm::vec3 lclP = face.lclN * face.lclD;
        gfxm::vec3 P = shape->transform * gfxm::vec4(lclP, 1.f);
        face.D = gfxm::dot(face.N, P);

        if (shape->automatic_uv) {
            face.uvs.resize(face.control_points.size());

            gfxm::vec3 face_origin = face.lclN * face.lclD;
            gfxm::mat3 orient = csgMakeFaceLocalOrientationMatrix(&face, face_origin);
            gfxm::vec2& uv_scale = face.uv_scale;
            gfxm::vec2& uv_offset = face.uv_offset;
            for (int i = 0; i < face.control_points.size(); ++i) {
                gfxm::vec3 V = face.control_points[i]->position;
                gfxm::vec2 uv = csgProjectVertexXY(orient, face_origin, V);
                uv.x += uv_offset.x;
                uv.y += uv_offset.y;
                uv.x *= 1.f / uv_scale.x;
                uv.y *= 1.f / uv_scale.y;
                face.uvs[i] = uv;
            }
        }
    }
}

gfxm::mat3 csgMakeFaceLocalOrientationMatrix(csgFace* face, gfxm::vec3& origin) {
    origin = face->lclN * face->lclD;
    gfxm::vec3 up = gfxm::vec3(.0f, 1.f, .0f);
    gfxm::vec3 right = gfxm::vec3(1.f, .0f, .0f);
    gfxm::vec3 back = gfxm::vec3(.0f, .0f, 1.f);
    gfxm::vec3 planeN = face->lclN;
    gfxm::mat3 m;
    if (fabsf(gfxm::dot(up, planeN)) >= (1.f - FLT_EPSILON)) {
        back = gfxm::cross(planeN, right);
        right = gfxm::cross(back, up);
        up = planeN;

        m[0] = right;
        m[1] = back;
        m[2] = up;
    } else {
        right = gfxm::cross(up, planeN);
        up = gfxm::cross(planeN, right);
        back = planeN;

        m[0] = right;
        m[1] = up;
        m[2] = back;
    }

    return m;
}

gfxm::mat3 csgMakeFaceOrientationMatrix(csgFace* face, gfxm::vec3& origin) {
    origin = face->N * face->D;
    gfxm::vec3 up = gfxm::vec3(.0f, 1.f, .0f);
    gfxm::vec3 right = gfxm::vec3(1.f, .0f, .0f);
    gfxm::vec3 back = gfxm::vec3(.0f, .0f, 1.f);
    gfxm::vec3 planeN = face->N;
    gfxm::mat3 m;
    if (fabsf(gfxm::dot(up, planeN)) >= (1.f - FLT_EPSILON)) {
        back = gfxm::cross(planeN, right);
        right = gfxm::cross(back, up);
        up = planeN;

        m[0] = right;
        m[1] = back;
        m[2] = up;
    } else {
        right = gfxm::cross(up, planeN);
        up = gfxm::cross(planeN, right);
        back = planeN;

        m[0] = right;
        m[1] = up;
        m[2] = back;
    }

    return m;
}


void csgUpdateFaceNormals(csgFace* face, bool smooth) {
    auto shape = face->shape;
    face->lcl_normals.resize(face->control_points.size());
    if (smooth) {
        for (int i = 0; i < face->control_points.size(); ++i) {
            auto& v = *face->control_points[i];
            gfxm::vec3 N;
            float fcount = .0f;
            for (auto f : v.faces) {
                if (fabsf(gfxm::dot(face->lclN, f->lclN)) < .707f) {
                    continue;
                }
                N += f->lclN;
                fcount += 1.f;
            }
            //N /= fcount;
            face->lcl_normals[i] = gfxm::normalize(N);
        }
    } else {
        for (int i = 0; i < face->control_points.size(); ++i) {
            face->lcl_normals[i] = face->lclN;
        }
    }

    if (face->uvs.size() != face->control_points.size()) {
        face->uvs.resize(face->control_points.size());

        gfxm::vec3 face_origin = face->lclN * face->lclD;
        gfxm::mat3 orient = csgMakeFaceLocalOrientationMatrix(face, face_origin);
        gfxm::vec2& uv_scale = face->uv_scale;
        gfxm::vec2& uv_offset = face->uv_offset;
        for (int i = 0; i < face->control_points.size(); ++i) {
            gfxm::vec3 V = face->control_points[i]->position;
            gfxm::vec2 uv = csgProjectVertexXY(orient, face_origin, V);
            uv.x += uv_offset.x;
            uv.y += uv_offset.y;
            uv.x *= 1.f / uv_scale.x;
            uv.y *= 1.f / uv_scale.y;
            face->uvs[i] = uv;
        }
    }
}

void csgUpdateShapeNormals(csgBrushShape* shape, bool smooth) {
    for (auto& face : shape->faces) {
        csgUpdateFaceNormals(face.get(), smooth);
    }
}


bool csgMakeVertexLocal(csgFace* FA, csgFace* FB, csgFace* FC, csgVertex* out) {
    std::array<csgFace*, 3> faces = { FA, FB, FC };
    std::sort(faces.begin(), faces.end());
    FA = FA;
    FB = FB;
    FC = FC;

    float d0 = gfxm::dot(FA->lclN, FB->lclN);
    float d1 = gfxm::dot(FA->lclN, FC->lclN);
    float d2 = gfxm::dot(FB->lclN, FC->lclN);
    if (fabsf(d0) >= 1.f
        || fabsf(d1) >= 1.f
        || fabsf(d2) >= 1.f
    ) {
        return false;
    }

    gfxm::vec3 lineN = gfxm::cross(FA->lclN, FB->lclN);
    float det = lineN.length2();
    gfxm::vec3 lineP = (gfxm::cross(FB->lclN, lineN) * FA->lclD + gfxm::cross(lineN, FA->lclN) * FB->lclD) / det;

    // C plane with intersection line intersection
    float denom = gfxm::dot(FC->lclN, lineN);
    if (abs(denom) <= 0.00000001f) {
        return false;
    }
    gfxm::vec3 vec = FC->lclN * FC->lclD - lineP;
    float t = gfxm::dot(vec, FC->lclN) / denom;
    out->position = lineP + lineN * t;
    out->faces.insert(FA);
    out->faces.insert(FB);
    out->faces.insert(FC);
    return true;
}

bool csgMakeVertex(csgFace* FA, csgFace* FB, csgFace* FC, csgVertex* out) {
    std::array<csgFace*, 3> faces = { FA, FB, FC };
    std::sort(faces.begin(), faces.end());
    FA = FA;
    FB = FB;
    FC = FC;
    //csgPlane* A = FA->plane;
    //csgPlane* B = FB->plane;
    //csgPlane* C = FC->plane;

    float d0 = gfxm::dot(FA->N, FB->N);
    float d1 = gfxm::dot(FA->N, FC->N);
    float d2 = gfxm::dot(FB->N, FC->N);
    if (fabsf(d0) >= 1.f
        || fabsf(d1) >= 1.f
        || fabsf(d2) >= 1.f
    ) {
        return false;
    }

    gfxm::vec3 lineN = gfxm::cross(FA->N, FB->N);
    float det = lineN.length2();
    gfxm::vec3 lineP = (gfxm::cross(FB->N, lineN) * FA->D + gfxm::cross(lineN, FA->N) * FB->D) / det;

    // C plane with intersection line intersection
    float denom = gfxm::dot(FC->N, lineN);
    if (abs(denom) <= 0.00000001f) {
        return false;
    }
    gfxm::vec3 vec = FC->N * FC->D - lineP;
    float t = gfxm::dot(vec, FC->N) / denom;
    out->position = lineP + lineN * t;
    out->faces.insert(FA);
    out->faces.insert(FB);
    out->faces.insert(FC);
    return true;
}

bool csgFindEdge(const csgVertex* a, const csgVertex* b, csgEdgeTmp* edge) {
    std::vector<csgFace*> faces;
    auto it = std::set_intersection(
        std::begin(a->faces), std::end(a->faces),
        std::begin(b->faces), std::end(b->faces),
        std::back_inserter(faces)
    );

    if (faces.size() >= 2) {
        if (edge) {
            *edge = csgEdgeTmp{ { faces[0], faces[1] } };
        }
        return true;
    }
    else {
        return false;
    }
}

void csgFindEdges(csgBrushShape* shape, csgFace* face) {
    face->control_points.clear();
    std::set<csgVertex*> unsorted = std::move(face->tmp_control_points);
    auto it = unsorted.begin();
    while (it != unsorted.end()) {
        csgVertex* vert = *it;
        face->control_points.push_back(vert);
        unsorted.erase(it);
        it = std::find_if(unsorted.begin(), unsorted.end(), [shape, &vert](csgVertex* other)->bool {
            return csgFindEdge(vert, other);
        });
    }
}

void csgFixWindingLocal(csgBrushShape* shape, csgFace* face) {
    assert(face->vertexCount() >= 3);
    gfxm::vec3 cross;
    const auto& p0 = face->control_points[0]->position;
    const auto& p1 = face->control_points[1]->position;
    for (int i = 2; i < face->control_points.size(); ++i) {
        const auto& p2 = face->control_points[i]->position;
        cross = gfxm::cross(p1 - p0, p2 - p0);
        if (cross.length2() > FLT_EPSILON) {
            break;
        }
    }
    float d = gfxm::dot(face->lclN, cross);
    if (d < .0f) {
        std::reverse(face->control_points.begin(), face->control_points.end());
    }
    return;
}

void csgFixWinding(csgBrushShape* shape, csgFace* face) {
    // TODO: This only works when creating the shape from planes, since face->N is same as face->lclN at that point
    assert(face->vertexCount() >= 3);
    gfxm::vec3 cross;
    const auto& p0 = face->control_points[0]->position;
    const auto& p1 = face->control_points[1]->position;
    for (int i = 2; i < face->control_points.size(); ++i) {
        const auto& p2 = face->control_points[i]->position;
        cross = gfxm::cross(p1 - p0, p2 - p0);
        if (cross.length2() > FLT_EPSILON) {
            break;
        }
    }
    float d = gfxm::dot(face->N, gfxm::normalize(cross));
    if (d < .0f) {
        std::reverse(face->control_points.begin(), face->control_points.end());
    }
    return;
}


void csgShapeInitFacesFromPlanes_(csgBrushShape* shape) {
    std::unordered_map<std::size_t, int> vertex_hash_to_index;
    // TODO: Optimize
    // Init faces once in local space
    // Then transform resulting vertices, avoiding face recalculations upon transforming
    // Still need to reinit faces when planes are modified tho
    int plane_count = shape->planes.size();
    shape->control_points.clear();
    shape->faces.clear();
    shape->faces.resize(plane_count);
    for (int i = 0; i < plane_count; ++i) {
        shape->faces[i].reset(new csgFace);
        shape->faces[i]->shape = shape;
        shape->faces[i]->N = shape->planes[i].N;
        shape->faces[i]->D = shape->planes[i].D;
        shape->faces[i]->lclN = shape->planes[i].N;
        shape->faces[i]->lclD = shape->planes[i].D;
        shape->faces[i]->material = shape->planes[i].material;
        shape->faces[i]->uv_offset = shape->planes[i].uv_offset;
        shape->faces[i]->uv_scale = shape->planes[i].uv_scale;
    }
    bool aabb_initialized = false;
    
    for (int i = 0; i < plane_count; ++i) {
        for (int j = i + 1; j < plane_count - 1; ++j) {
            for (int k = j + 1; k < plane_count; ++k) {
                auto FA = shape->faces[i].get();
                auto FB = shape->faces[j].get();
                auto FC = shape->faces[k].get();
                csgVertex v;
                if (!csgMakeVertex(FA, FB, FC, &v)) {
                    continue;
                }
                std::size_t hash = std::hash<csgVertex>()(v);
                int vertex_index = -1;
                csgVertex* p_vertex = 0;
                auto it = vertex_hash_to_index.find(hash);
                if (it == vertex_hash_to_index.end()
                    || !csgCompareVertices(*shape->control_points[it->second].get(), v)
                ) {
                    CSG_RELATION_TYPE rel = csgCheckVertexShapeRelation(&v, shape);
                    if (rel == CSG_RELATION_OUTSIDE) {
                        continue;
                    }
                    vertex_index = shape->_controlPointCount();
                    vertex_hash_to_index.insert(std::make_pair(hash, vertex_index));
                    p_vertex = shape->_createControlPoint(v);
                } else {
                    vertex_index = it->second;
                    p_vertex = shape->_getControlPoint(vertex_index);

                    p_vertex->faces.insert(FA);
                    p_vertex->faces.insert(FB);
                    p_vertex->faces.insert(FC);
                }


                if (aabb_initialized) {
                    gfxm::expand_aabb(shape->aabb, v.position);
                } else {
                    shape->aabb.from = v.position;
                    shape->aabb.to = v.position;
                    aabb_initialized = true;
                }

                FA->tmp_control_points.insert(p_vertex);
                FB->tmp_control_points.insert(p_vertex);
                FC->tmp_control_points.insert(p_vertex);
            }
        }
    }
    if (aabb_initialized) {
        shape->aabb.from.x -= 0.001f;
        shape->aabb.from.y -= 0.001f;
        shape->aabb.from.z -= 0.001f;
        shape->aabb.to.x += 0.001f;
        shape->aabb.to.y += 0.001f;
        shape->aabb.to.z += 0.001f;
    }
    shape->world_space_vertices.resize(shape->_controlPointCount());
    for (int i = 0; i < shape->_controlPointCount(); ++i) {
        shape->world_space_vertices[i] = shape->_getControlPoint(i)->position;
    }

    for (int i = 0; i < shape->faces.size(); ++i) {
        auto F = shape->faces[i].get();
        csgFindEdges(shape, F);
        csgFixWinding(shape, F);
    }

    csgUpdateShapeNormals(shape);
}
