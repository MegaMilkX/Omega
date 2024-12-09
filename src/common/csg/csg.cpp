#include "csg.hpp"
#include <assert.h>
#include <stdint.h>
#include <unordered_set>

#include "math/intersection.hpp"

#include "gpu/gpu.hpp"
#include "gpu/gpu_util.hpp"
#include "gpu/gpu_buffer.hpp"
#include "gpu/gpu_shader_program.hpp"
#include "gpu/gpu_texture_2d.hpp"

#include "CDT/include/CDT.h"

struct csgData {
    std::unique_ptr<gpuShaderProgram> prog;
    std::unique_ptr<gpuShaderProgram> prog_line;/*
    std::unique_ptr<gpuBuffer> vertex_buffer;
    std::unique_ptr<gpuBuffer> normal_buffer;
    std::unique_ptr<gpuBuffer> color_buffer;
    std::unique_ptr<gpuBuffer> uv_buffer;
    std::unique_ptr<gpuBuffer> index_buffer;
    const gpuMeshShaderBinding* mesh_binding = 0;*/

    struct Mesh {
        csgMaterial* material = 0;
        gpuBuffer vertex_buffer;
        gpuBuffer normal_buffer;
        gpuBuffer color_buffer;
        gpuBuffer uv_buffer;
        gpuBuffer index_buffer;
        std::unique_ptr<gpuMeshDesc> mesh_desc;
        gpuMeshShaderBinding mesh_binding;
    };
    std::vector<std::unique_ptr<Mesh>> meshes;

    std::vector<std::unique_ptr<csgMaterial>> materials;

    struct {
        std::unique_ptr<gpuBuffer> vertex_buffer;
        gpuMeshShaderBinding mesh_binding;
        std::vector<gfxm::vec3> points;
    } point;
    struct {
        std::unique_ptr<gpuBuffer> vertex_buffer;
        gpuMeshShaderBinding mesh_binding;
    } lines;
};
static csgData* csg_data = 0;


bool isVertInsidePlane(
    const gfxm::vec3& v,
    const gfxm::vec3& p0,
    const gfxm::vec3& p1,
    const gfxm::vec3& p2
) {
    gfxm::vec3 cross = gfxm::cross(p1 - p0, p2 - p0);
    { // Check if plane is degenerate (zero area)
        float d = cross.length();
        if (d <= FLT_EPSILON) {
            assert(false);
            return false;
        }
    }

    gfxm::vec3 plane_N = gfxm::normalize(cross);
    gfxm::vec3 pt_dir = v - p0;
    float d = gfxm::dot(pt_dir, plane_N);
    if (d < .0f) {
        return true;
    } else {
        return false;
    }
}
bool intersectEdgeFace(
    const gfxm::vec3& e0,
    const gfxm::vec3& e1,
    const gfxm::vec3& p0,
    const gfxm::vec3& p1,
    const gfxm::vec3& p2,
    const gfxm::vec3& p3,
    gfxm::vec3& result
) {
    gfxm::vec3 cross = gfxm::cross(p1 - p0, p2 - p0);
    { // Check if plane is degenerate (zero area)
        float d = cross.length();
        if (d <= FLT_EPSILON) {
            assert(false);
            return false;
        }
    }
    
    gfxm::vec3 N = gfxm::normalize(cross);

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

    gfxm::vec3 c0 = gfxm::cross(line_plane_intersection - p0, p1 - p0);
    gfxm::vec3 c1 = gfxm::cross(line_plane_intersection - p1, p2 - p1);
    gfxm::vec3 c2 = gfxm::cross(line_plane_intersection - p2, p3 - p2);
    gfxm::vec3 c3 = gfxm::cross(line_plane_intersection - p3, p0 - p3);
    bool inside = gfxm::dot(c0, N) <= 0 && gfxm::dot(c1, N) <= 0 && gfxm::dot(c2, N) <= 0 && gfxm::dot(c3, N) <= 0;
    if (!inside) {
        return false;
    }

    result = line_plane_intersection;
    /*
    if (dot_test < .0f) {
        rhp.normal = N;
        rhp.distance = gfxm::length(ray.direction) * t;
    } else { // backface
        rhp.normal = -N;
        rhp.distance = gfxm::length(ray.direction) * t;
    }*/
    return true;
}
bool isPointOnLineSegment(
    const gfxm::vec3& P,
    const gfxm::vec3& A,
    const gfxm::vec3& B,
    float& t
) {
    gfxm::vec3 cross = gfxm::cross(B - A, P - A);
    float cross_len = cross.length();
    const float EPSILON = 0.000001f;
    if (cross_len > EPSILON) {
        return false;
    }
    float d = gfxm::dot(B - A, P - A);
    // Change to d < .0f to allow matching with the very end of the line segment
    if (d < FLT_EPSILON) {
        return false;
    }
    // Remove -FLT_EPSILON to allow matching with the very end of the line segment
    if (d > (B - A).length2() - FLT_EPSILON) {
        return false;
    }

    t = gfxm::sqrt(d);
    return true;
}


struct FacePairKey {
    uint64_t shape_a;
    uint64_t shape_b;
    int face_a;
    int face_b;

    FacePairKey() {}
    FacePairKey(uint64_t shape_a, uint64_t shape_b, int face_a, int face_b)
        : shape_a(shape_a),
        shape_b(shape_b),
        face_a(face_a),
        face_b(face_b) {
        if (shape_a > shape_b) {
            std::swap(this->shape_a, this->shape_b);
            std::swap(this->face_a, this->face_b);
        }
    }
    bool operator==(const FacePairKey& other) const {
        return shape_a == other.shape_a
            && shape_b == other.shape_b
            && face_a == other.face_a
            && face_b == other.face_b;
    }
};
template<>
struct std::hash<FacePairKey> {
    std::size_t operator()(const FacePairKey& k) const {
        return std::hash<uint64_t>()(k.shape_a)
            ^ std::hash<uint64_t>()(k.shape_b)
            ^ std::hash<uint64_t>()(k.face_a)
            ^ std::hash<uint64_t>()(k.face_b);
    }
};

struct FaceKey {
    uint64_t shape;
    int face;

    FaceKey() {}
    FaceKey(uint64_t shape, int face)
        : shape(shape), face(face) {}
    bool operator==(const FaceKey& other) const {
        return shape == other.shape
            && face == other.face;
    }
};
template<>
struct std::hash<FaceKey> {
    std::size_t operator()(const FaceKey& k) const {
        return std::hash<uint64_t>()(k.shape)
            ^ std::hash<uint64_t>()(k.face);
    }
};


uint32_t makeColor32(float R, float G, float B, float A) {
    uint32_t rc = std::min(255, int(255 * R));
    uint32_t gc = std::min(255, int(255 * G));
    uint32_t bc = std::min(255, int(255 * B));
    uint32_t ac = std::min(255, int(255 * A));
    uint32_t C = 0;
    C |= rc;
    C |= gc << 8;
    C |= bc << 16;
    C |= ac << 24;
    return C;
}

bool intersectPlanePlane(const csgPlane& A, const csgPlane& B, csgLine& out) {
    float d = gfxm::dot(A.N, B.N);
    if (fabsf(d) >= 1.f - FLT_EPSILON) {
        return false;
    }

    gfxm::vec3 CN = gfxm::cross(A.N, B.N);
    float det = CN.length2();
    gfxm::vec3 P = (gfxm::cross(B.N, CN) * A.D + gfxm::cross(CN, A.N) * B.D) / det;
    out.N = CN;
    out.P = P;
    return true;
}
bool intersectLinePlane(const csgLine& L, const csgPlane& P, gfxm::vec3& out) {
    float denom = gfxm::dot(P.N, L.N);
    if (abs(denom) <= 0.000001f) {
        return false;
    }
    gfxm::vec3 vec = P.N * P.D - L.P;
    float t = gfxm::dot(vec, P.N) / denom;
    out = L.P + L.N * t;
    return true;
}
bool isVertexInsidePlanes(const csgPlane* planes, int count, const gfxm::vec3& v) {
    for(int i = 0; i < count; ++i) {/*
        float d = gfxm::dot(gfxm::vec4(planes[i].N, planes[i].D), gfxm::vec4(v, 1.f));
        if (d > FLT_EPSILON) {
            return false;
        }*/
        
        gfxm::vec3 planePoint = planes[i].N * planes[i].D;
        gfxm::vec3 pt_dir = v - planePoint;
        float d = gfxm::dot(gfxm::normalize(pt_dir), planes[i].N);
        if (d > 0.f) {
            return false;
        }
    }
    return true;
}


void csgShapeInitFacesFromPlanes2(csgBrushShape* shape) {
    auto compareVertices = [](const gfxm::vec3& a, const gfxm::vec3& b)->bool {
        const float EPS = .001f;
        const float INV_EPS = 1.f / EPS;
        const int ix = round(a.x * INV_EPS);
        const int iy = round(a.y * INV_EPS);
        const int iz = round(a.z * INV_EPS);
        const int oix = round(b.x * INV_EPS);
        const int oiy = round(b.y * INV_EPS);
        const int oiz = round(b.z * INV_EPS);
        return ix == oix && iy == oiy && iz == oiz;
    };
    auto makeVertexHash = [](const gfxm::vec3& v)->uint64_t {
        const float EPS = .001f;
        const float INV_EPS = 1.f / EPS;
        uint64_t h = 0x778abe;
        h ^= std::hash<int>()(round((v.x + .5f) * INV_EPS)) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<int>()(round((v.y + .5f) * INV_EPS)) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<int>()(round((v.z + .5f) * INV_EPS)) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    };

    shape->control_points.clear();
    shape->faces.clear();

    struct Plane {
        gfxm::vec3 N;
        float D;
    };
    struct Edge {
        gfxm::vec3 P;
        gfxm::vec3 N;
        float ta = -INFINITY;
        float tb = INFINITY;
        bool discard = false;
    };

    std::vector<Plane> planes;
    planes.resize(shape->planes.size());
    for (int i = 0; i < shape->planes.size(); ++i) {
        auto& p = shape->planes[i];
        planes[i].N = p.N;
        planes[i].D = p.D;
    }
    const int n_planes = planes.size();

    const int n_edges = n_planes * (n_planes - 1) / 2;
    std::vector<Edge> edges(n_edges);
    int i_edge = 0;
    for (int i = 0; i < n_planes; ++i) {
        auto& plane_a = planes[i];
        if (plane_a.N.length() == .0f) {
            continue;
        }
        for (int j = i + 1; j < n_planes; ++j) {
            auto& plane_b = planes[j];
            if (plane_b.N.length() == .0f) {
                continue;
            }
            gfxm::vec3 C = gfxm::cross(plane_a.N, plane_b.N);
            float Clen = C.length();
            if (Clen <= FLT_EPSILON) {
                continue;
            }
            gfxm::vec3 N = C / Clen;
            gfxm::vec3 P = (gfxm::cross(plane_b.N, C) * plane_a.D + gfxm::cross(C, plane_a.N) * plane_b.D) / C.length2();
            edges[i_edge].P = P;
            edges[i_edge].N = N;
            ++i_edge;
        }
    }
    int edge_count = i_edge;
    
    i_edge = 0;
    for (int i = 0; i < n_planes; ++i) {
        Plane& plane = planes[i];
        for (int j = 0; j < edge_count; ++j) {
            Edge& edge = edges[j];
            if (edge.discard) {
                continue;
            }
            float denom = gfxm::dot(plane.N, edge.N);
            if (abs(denom) <= 0.00001f) {
                continue;
            }
            gfxm::vec3 vec = plane.N * plane.D - edge.P;
            float t = gfxm::dot(vec, plane.N) / denom;
            if (denom > .0f) {
                if (edge.tb > t) {
                    edge.tb = gfxm::_min(edge.tb, t);
                }
            } else {
                if (edge.ta < t) {
                    edge.ta = gfxm::_max(edge.ta, t);
                }
            }
        }
    }

    for (int i = 0; i < edge_count; ++i) {
        Edge& edge = edges[i];
        if (edge.ta == -INFINITY || edge.tb == INFINITY || edge.ta >= edge.tb) {
            edge.discard = true;
        }
    }

    // TODO: Find unique vertices
    struct Vert {
        gfxm::vec3 P;
    };
    std::unordered_map<uint64_t, Vert*> vertices;
    for (int i = 0; i < edge_count; ++i) {
        Edge& edge = edges[i];
        if (edge.discard) {
            continue;
        }
        gfxm::vec3 va = edge.P + edge.N * edge.ta;
        gfxm::vec3 vb = edge.P + edge.N * edge.tb;
        uint64_t ha = makeVertexHash(va);
        uint64_t hb = makeVertexHash(vb);
        auto it = vertices.find(ha);
        if (it == vertices.end()) {
            //vertices.insert(std::make_pair(ha, new Vert{ va }));
        }
        it = vertices.find(hb);
        if (it == vertices.end()) {
            //vertices.insert(std::make_pair(hb, new Vert{ vb }));
        }
    }
    // TODO:
    assert(false);
}

void csgMakeTestShape(csgBrushShape* shape, const gfxm::mat4& transform) {
    shape->planes.clear();
    shape->planes.resize(7);
    shape->planes[0] = { gfxm::vec3(-1,  0,  0), 1.f };
    shape->planes[1] = { gfxm::vec3(1,  0,  0), .0f };
    shape->planes[2] = { gfxm::vec3(0, -1,  0), -.0f };
    shape->planes[3] = { gfxm::vec3(0,  1,  0), 1.f };
    shape->planes[4] = { gfxm::vec3(0,  0, -1), 1.f };
    shape->planes[5] = { gfxm::vec3(0,  0,  1), .0f };
    shape->planes[6] = { gfxm::normalize(gfxm::vec3(1,  0, -1)), .2f };
    shape->transform = transform;

    csgTransformShape(shape, transform);
    csgShapeInitFacesFromPlanes_(shape);
    csgUpdateShapeWorldSpace(shape);
}
void csgMakeCube(csgBrushShape* shape, float width, float height, float depth, const gfxm::mat4& transform) {
    shape->planes.clear();
    shape->planes.resize(6);
    shape->planes[0] = { gfxm::vec3(-1,  0,  0), width * .5f };
    shape->planes[1] = { gfxm::vec3( 1,  0,  0), width * .5f };
    shape->planes[2] = { gfxm::vec3( 0, -1,  0), height * .5f };
    shape->planes[3] = { gfxm::vec3( 0,  1,  0), height * .5f };
    shape->planes[4] = { gfxm::vec3( 0,  0, -1), depth * .5f };
    shape->planes[5] = { gfxm::vec3( 0,  0,  1), depth * .5f };
    shape->transform = transform;

    csgTransformShape(shape, transform);
    csgShapeInitFacesFromPlanes_(shape);
    csgUpdateShapeWorldSpace(shape);
}
void csgMakeBox(csgBrushShape* shape, float width, float height, float depth, const gfxm::mat4& transform) {
    const float hw = fabsf(width) * .5f;
    const float hh = fabsf(height) * .5f;
    const float hd = depth * .5f;
    shape->planes.clear();
    shape->planes.resize(6);/*
    shape->planes[0] = { gfxm::vec3(-1,  0,  0), hw };
    shape->planes[1] = { gfxm::vec3( 1,  0,  0), hw };
    shape->planes[2] = { gfxm::vec3( 0, -1,  0), hh };
    shape->planes[3] = { gfxm::vec3( 0,  1,  0), hh };*/
    shape->planes[0] = { gfxm::vec3(-1,  0,  0), width < .0f ? fabsf(width) : .0f };
    shape->planes[1] = { gfxm::vec3(1,  0,  0), width < .0f ? .0f : fabsf(width) };
    shape->planes[2] = { gfxm::vec3(0, -1,  0), height < .0f ? fabsf(height) : .0f };
    shape->planes[3] = { gfxm::vec3(0,  1,  0), height < .0f ? .0f : fabsf(height) };
    shape->planes[4] = { gfxm::vec3( 0,  0, -1), depth < .0f ? fabsf(depth) : .0f };
    shape->planes[5] = { gfxm::vec3( 0,  0,  1), depth < .0f ? .0f : fabsf(depth) };
    shape->transform = transform;

    csgTransformShape(shape, transform);
    csgShapeInitFacesFromPlanes_(shape);
    csgUpdateShapeWorldSpace(shape);
}
void csgMakeCylinder(csgBrushShape* shape, float height, float radius, int segments, const gfxm::mat4& transform) {
    assert(height != .0f);
    assert(segments >= 3);
    assert(radius > 0.001f);

    std::vector<gfxm::vec2> uvs;
    float inv_2pi = 1.f / (gfxm::pi * 2.f);
    float inv_pi = 1.f / gfxm::pi;
    //float diameter = radius * 2.f;
    float approx_pi = segments * sinf(2.f * gfxm::pi / segments);
    float circum_step = 2.f * radius * sinf(gfxm::pi / segments);
    float circumference = segments * circum_step;

    shape->automatic_uv = false;
    shape->planes.clear();
    shape->faces.clear();
    shape->control_points.clear();

    for (int i = 0; i < segments; ++i) {
        float a = i / (float)segments * gfxm::pi * 2.f;
        float x = sinf(a) * radius;
        float z = cosf(a) * radius;
        shape->_createControlPoint(gfxm::vec3(x, height > .0f ? .0f : height, z));
        shape->_createControlPoint(gfxm::vec3(x, height > .0f ? height : .0f, z));
        //uvs.push_back(gfxm::vec2(a * inv_2pi * diameter, .0f));
        //uvs.push_back(gfxm::vec2(a * inv_2pi * diameter, 1.f));
        uvs.push_back(gfxm::vec2(a * inv_2pi * circumference, .0f));
        uvs.push_back(gfxm::vec2(a * inv_2pi * circumference, fabsf(height)));
    }

    shape->faces.resize(segments + 2);
    for (int i = 0; i < segments; ++i) {
        const int a = i * 2;
        const int b = i * 2 + 1;
        const int c = (i * 2 + 2) % shape->_controlPointCount();
        const int d = (i * 2 + 3) % shape->_controlPointCount();
        gfxm::vec2 uv_seam_fix = gfxm::vec2(1.f, .0f) * float((i + 1) / segments);

        auto cpa = shape->_getControlPoint(a);
        auto cpb = shape->_getControlPoint(b);
        auto cpc = shape->_getControlPoint(c);
        auto cpd = shape->_getControlPoint(d);

        shape->faces[i].reset(new csgFace);
        auto& face = *shape->faces[i].get();
        face.shape = shape;
        face.control_points.push_back(cpb);
        face.control_points.push_back(cpa);
        face.control_points.push_back(cpc);
        face.control_points.push_back(cpd);
        face.uvs.push_back(uvs[b]);
        face.uvs.push_back(uvs[a]);
        face.uvs.push_back(uvs[a] + gfxm::vec2(circum_step, .0f));
        face.uvs.push_back(uvs[b] + gfxm::vec2(circum_step, .0f));
        //face.uvs.push_back(uvs[c] + uv_seam_fix);
        //face.uvs.push_back(uvs[d] + uv_seam_fix);

        cpa->faces.insert(&face);
        cpb->faces.insert(&face);
        cpc->faces.insert(&face);
        cpd->faces.insert(&face);
        gfxm::vec3 N = -gfxm::normalize(gfxm::cross(cpb->position - cpc->position, cpb->position - cpa->position));
        float D = gfxm::dot(N, cpa->position);
        face.lclN = N;
        face.lclD = D;
    }

    {
        shape->faces[segments].reset(new csgFace);
        auto& face = *shape->faces[segments].get();
        face.shape = shape;
        for (int i = segments - 1; i >= 0; --i) {
            const int a = i * 2;
            auto cp = shape->_getControlPoint(a);
            face.control_points.push_back(shape->_getControlPoint(a));
            face.uvs.push_back(gfxm::vec2(cp->position.x, cp->position.z));
            cp->faces.insert(&face);
        }
        auto cpa = face.control_points[0];
        auto cpb = face.control_points[1];
        auto cpc = face.control_points[2];
        gfxm::vec3 N = gfxm::normalize(gfxm::cross(cpb->position - cpc->position, cpb->position - cpa->position));
        float D = gfxm::dot(N, cpa->position);
        face.lclN = N;
        face.lclD = D;
    }

    {
        shape->faces[segments + 1].reset(new csgFace);
        auto& face = *shape->faces[segments + 1].get();
        face.shape = shape;
        for (int i = 0; i < segments; ++i) {
            const int a = i * 2 + 1;
            auto cp = shape->_getControlPoint(a);
            face.control_points.push_back(shape->_getControlPoint(a));
            face.uvs.push_back(gfxm::vec2(cp->position.x, cp->position.z));
            cp->faces.insert(&face);
        }
        auto cpa = face.control_points[0];
        auto cpb = face.control_points[1];
        auto cpc = face.control_points[2];
        gfxm::vec3 N = gfxm::normalize(gfxm::cross(cpb->position - cpc->position, cpb->position - cpa->position));
        float D = gfxm::dot(N, cpa->position);
        face.lclN = N;
        face.lclD = D;
    }

    shape->transform = transform;

    csgTransformShape(shape, transform);
    csgUpdateShapeWorldSpace(shape);
    csgUpdateShapeNormals(shape);
    /*
    shape->planes.clear();
    shape->planes.resize(segments + 2);
    shape->planes[0] = { gfxm::vec3( 0, -1,  0), .0f };
    for (int i = 0; i < segments; ++i) {
        float step = gfxm::pi * 2 / (float)segments;
        float a = step * (float)i;
        gfxm::vec3 n = gfxm::vec3(cosf(a), .0f, sinf(a));
        shape->planes[i + 1] = { gfxm::normalize(n), radius };
    }
    shape->planes[segments + 1] = { gfxm::vec3(0,  1,  0), height };
    shape->transform = transform;

    csgTransformShape(shape, transform);
    csgShapeInitFacesFromPlanes_(shape);
    csgUpdateShapeWorldSpace(shape);*/
}
void csgMakeSphere(csgBrushShape* shape, int segments, float radius, const gfxm::mat4& transform) {
    assert(radius > 0.001f);

    shape->automatic_uv = false;

    segments = std::max(4, segments - (segments % 2));

    std::vector<gfxm::vec2> uvs;

    {
        shape->_createControlPoint(gfxm::vec3(0, radius, 0));
        uvs.push_back(gfxm::vec2(.5f, 1.0f));
    }

    float inv_2pi = 1.f / (gfxm::pi * 2.f);
    float inv_pi = 1.f / gfxm::pi;
    for (int i = 0; i < segments - 1; ++i) {
        for (int j = 0; j < segments; ++j) {
            float lat = (i + 1) / (float)segments * gfxm::pi * 2.f;
            float lon = j / (float)segments * gfxm::pi * 2.f;
            float z = radius * cosf(lon) * sinf(lat);
            float y = radius * cosf(lat);
            float x = radius * sinf(lon) * sinf(lat);
            gfxm::vec3 P = gfxm::vec3(x, y, z);
            gfxm::vec3 N = gfxm::normalize(P);

            shape->_createControlPoint(P);
            uvs.push_back(gfxm::vec2(lon * inv_2pi, 1.f - lat * inv_pi));
        }
    }
    
    {
        shape->_createControlPoint(gfxm::vec3(0, -radius, 0));
        uvs.push_back(gfxm::vec2(.5f, .0f));
    }

    int levels = segments / 2 - 2;
    shape->faces.resize(segments * levels + segments * 2);

    // Cap
    for (int i = 0; i < segments; ++i) {
        int a = 0;
        int b = i % (segments) + 1;
        int c = (i + 1) % (segments) + 1;

        gfxm::vec2 uv_seam_fix = gfxm::vec2(1.f, .0f) * float((i + 1) / segments);

        shape->faces[i].reset(new csgFace);
        auto& face = *shape->faces[i].get();
        face.shape = shape;
        face.control_points.push_back(shape->_getControlPoint(a));
        face.control_points.push_back(shape->_getControlPoint(b));
        face.control_points.push_back(shape->_getControlPoint(c));
        face.uvs.push_back(uvs[a]);
        face.uvs.push_back(uvs[b]);
        face.uvs.push_back(uvs[c] + uv_seam_fix);

        auto& p0 = *shape->_getControlPoint(a);
        auto& p1 = *shape->_getControlPoint(b);
        auto& p2 = *shape->_getControlPoint(c);
        p0.faces.insert(&face);
        p1.faces.insert(&face);
        p2.faces.insert(&face);
        gfxm::vec3 N = gfxm::normalize(gfxm::cross(p1.position - p2.position, p1.position - p0.position));
        float D = gfxm::dot(N, p0.position);
        face.lclN = N;
        face.lclD = D;
    }
    
    for (int lv = 0; lv < levels; ++lv) {
        for (int i = 0; i < segments; ++i) {
            int a = (i + 1) % segments + lv * segments + 1;
            int b = i + lv * segments + 1;
            int c = b + segments;
            int d = a + segments;

            gfxm::vec2 uv_seam_fix = gfxm::vec2(1.f, .0f) * float((i + 1) / segments);

            shape->faces[i + lv * segments + segments].reset(new csgFace);
            csgFace& face = *shape->faces[i + lv * segments + segments].get();
            face.shape = shape;
            face.control_points.push_back(shape->_getControlPoint(a));
            face.control_points.push_back(shape->_getControlPoint(b));
            face.control_points.push_back(shape->_getControlPoint(c));
            face.control_points.push_back(shape->_getControlPoint(d));
            face.uvs.push_back(uvs[a] + uv_seam_fix);
            face.uvs.push_back(uvs[b]);
            face.uvs.push_back(uvs[c]);
            face.uvs.push_back(uvs[d] + uv_seam_fix);

            auto& p0 = *shape->_getControlPoint(a);
            auto& p1 = *shape->_getControlPoint(b);
            auto& p2 = *shape->_getControlPoint(c);
            auto& p3 = *shape->_getControlPoint(d);
            p0.faces.insert(&face);
            p1.faces.insert(&face);
            p2.faces.insert(&face);
            p3.faces.insert(&face);
            gfxm::vec3 N = gfxm::normalize(gfxm::cross(p1.position - p2.position, p1.position - p0.position));
            float D = gfxm::dot(N, p0.position);
            face.lclN = N;
            face.lclD = D;
        }
    }

    // Cap
    for (int i = 0; i < segments; ++i) {
        int a = shape->_controlPointCount() - 1;
        int b = (i + 1) % (segments)+1 + levels * segments;
        int c = i % (segments)+1 + levels * segments;

        gfxm::vec2 uv_seam_fix = gfxm::vec2(1.f, .0f) * float((i + 1) / segments);


        shape->faces[i + levels * segments + segments].reset(new csgFace);
        csgFace& face = *shape->faces[i + levels * segments + segments].get();
        face.shape = shape;
        face.control_points.push_back(shape->_getControlPoint(a));
        face.control_points.push_back(shape->_getControlPoint(b));
        face.control_points.push_back(shape->_getControlPoint(c));
        face.uvs.push_back(uvs[a]);
        face.uvs.push_back(uvs[b] + uv_seam_fix);
        face.uvs.push_back(uvs[c]);

        auto& p0 = *shape->_getControlPoint(a);
        auto& p1 = *shape->_getControlPoint(b);
        auto& p2 = *shape->_getControlPoint(c);
        p0.faces.insert(&face);
        p1.faces.insert(&face);
        p2.faces.insert(&face);
        gfxm::vec3 N = gfxm::normalize(gfxm::cross(p1.position - p2.position, p1.position - p0.position));
        float D = gfxm::dot(N, p0.position);
        face.lclN = N;
        face.lclD = D;
    }

    shape->transform = transform;

    csgTransformShape(shape, transform);
    csgUpdateShapeWorldSpace(shape);
    csgUpdateShapeNormals(shape);
}

void csgMakeConvexShape(csgBrushShape* shape, const gfxm::vec3* points_, int count, float height, const gfxm::vec3& refN) {
    assert(count >= 3);
    assert(height > FLT_EPSILON);

    shape->planes.clear();
    shape->faces.clear();
    shape->control_points.clear();

    std::vector<gfxm::vec3> points(points_, points_ + count);

    gfxm::vec3 A = points[1] - points[0];
    gfxm::vec3 B = points[2] - points[1];
    gfxm::vec3 N = gfxm::normalize(gfxm::cross(A, B));
    if (gfxm::dot(refN, N) < .0f) {
        N = -N;
        std::reverse(points.begin(), points.end());
    }

    gfxm::vec3 mid_point = gfxm::vec3(0,0,0);
    gfxm::aabb aabb;
    aabb.from = points[0];
    aabb.to = points[0];
    for (int i = 1; i < count; ++i) {
        gfxm::expand_aabb(aabb, points[i]);
    }
    mid_point = gfxm::vec3(
        gfxm::lerp(aabb.from.x, aabb.to.x, .5f),
        gfxm::lerp(aabb.from.y, aabb.to.y, .5f),
        gfxm::lerp(aabb.from.z, aabb.to.z, .5f)
    );
    for (int i = 0; i < count; ++i) {
        points[i] -= mid_point;
    }

    for (int i = 0; i < count; ++i) {
        shape->_createControlPoint(points[i] + (height > .0f ? gfxm::vec3(0, 0, 0) : N * height));
    }
    for (int i = 0; i < count; ++i) {
        shape->_createControlPoint(points[i] + (height > .0f ? N * height : gfxm::vec3(0, 0, 0)));
    }

    shape->faces.resize(count + 2);
    for (int i = 0; i < count; ++i) {
        int a = i % count;
        int b = i % count + count;
        int c = (i + 1) % count;
        int d = (i + 1) % count + count;

        auto cpa = shape->_getControlPoint(a);
        auto cpb = shape->_getControlPoint(b);
        auto cpc = shape->_getControlPoint(c);
        auto cpd = shape->_getControlPoint(d);

        shape->faces[i].reset(new csgFace);
        auto& face = *shape->faces[i].get();
        face.shape = shape;
        face.control_points.push_back(cpb);
        face.control_points.push_back(cpa);
        face.control_points.push_back(cpc);
        face.control_points.push_back(cpd);
        face.uvs.push_back(gfxm::vec2(.0f, .0f));
        face.uvs.push_back(gfxm::vec2(1.f, .0f));
        face.uvs.push_back(gfxm::vec2(1.f, 1.f));
        face.uvs.push_back(gfxm::vec2(.0f, 1.f));

        cpa->faces.insert(&face);
        cpb->faces.insert(&face);
        cpc->faces.insert(&face);
        cpd->faces.insert(&face);
        gfxm::vec3 N = -gfxm::normalize(gfxm::cross(cpb->position - cpc->position, cpb->position - cpa->position));
        float D = gfxm::dot(N, cpa->position);
        face.lclN = N;
        face.lclD = D;
    }

    // Top cap
    {
        shape->faces[count].reset(new csgFace);
        auto& face = *shape->faces[count].get();
        face.shape = shape;

        for (int i = 0; i < count; ++i) {
            auto cp = shape->_getControlPoint(i + count);
            face.control_points.push_back(cp);
            face.uvs.push_back(gfxm::vec2(.0f, .0f));
            cp->faces.insert(&face);
        }
        gfxm::vec3 faceN = N;
        float D = gfxm::dot(faceN, face.control_points[0]->position);
        face.lclN = faceN;
        face.lclD = D;
    }

    // Bottom cap
    {
        shape->faces[count + 1].reset(new csgFace);
        auto& face = *shape->faces[count + 1].get();
        face.shape = shape;

        for (int i = count - 1; i >= 0; --i) {
            auto cp = shape->_getControlPoint(i);
            face.control_points.push_back(cp);
            face.uvs.push_back(gfxm::vec2(.0f, .0f));
            cp->faces.insert(&face);
        }
        gfxm::vec3 faceN = -N;
        float D = gfxm::dot(faceN, face.control_points[0]->position);
        face.lclN = faceN;
        face.lclD = D;
    }

    gfxm::mat4 transform = gfxm::translate(gfxm::mat4(1.f), mid_point);
    shape->transform = transform;

    csgTransformShape(shape, transform);
    csgUpdateShapeWorldSpace(shape);
    csgUpdateShapeNormals(shape, false);
}

void csgMakeConvexShapeFromPlanes(csgBrushShape* shape, const gfxm::vec3* points, int count, float height) {
    assert(count >= 3);
    assert(height > FLT_EPSILON);
    
    shape->planes.clear();
    shape->planes.resize(count + 2);

    gfxm::vec3 A = points[1] - points[0];
    gfxm::vec3 B = points[2] - points[1];
    gfxm::vec3 N = gfxm::normalize(gfxm::cross(A, B));

    for (int i = 0; i < count - 1; ++i) {
        gfxm::vec3 a = points[i];
        gfxm::vec3 b = points[i + 1];
        gfxm::vec3 T = b - a;
        csgPlane plane;
        plane.N = gfxm::normalize(gfxm::cross(T, N));
        float d = gfxm::dot(a, plane.N);
        plane.D = d;
        shape->planes[i] = plane;
    }
    {
        gfxm::vec3 a = points[count - 1];
        gfxm::vec3 b = points[0];
        gfxm::vec3 T = b - a;
        csgPlane plane;
        plane.N = gfxm::normalize(gfxm::cross(T, N));
        float d = gfxm::dot(a, plane.N);
        plane.D = d;
        shape->planes[count - 1] = plane;
    }

    gfxm::mat4 transform = gfxm::mat4(1.f);
    shape->planes[count] = { N, height };
    shape->planes[count + 1] = { -N, .0f };
    shape->transform = transform;

    csgTransformShape(shape, transform);
    csgShapeInitFacesFromPlanes_(shape);
    csgUpdateShapeWorldSpace(shape);
}
