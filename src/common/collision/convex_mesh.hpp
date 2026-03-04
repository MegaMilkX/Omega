#pragma once

#include <vector>
#include <unordered_set>
#include "math/gfxm.hpp"
#include "debug_draw/debug_draw.hpp"
#include "collision/collision_contact_point.hpp"
#include "collision/intersection/sphere_capsule.hpp"
#include "collision/intersection/ray.hpp"


class phyConvexMesh {
public:
    struct Edge {
        uint32_t a;
        uint32_t b;
    };
private:

    std::vector<gfxm::vec3> vertices;
    std::vector<int> indices;
    std::vector<Edge> edges;
    std::vector<gfxm::vec3> face_normals;
    gfxm::vec3 centroid;

    void fixWinding() {
        if (vertices.empty() || indices.empty()) {
            return;
        }
        gfxm::vec3 centroid;
        for (int i = 0; i < vertices.size(); ++i) {
            centroid += vertices[i];
        }
        centroid /= (float)vertices.size();

        for (int i = 0; i < indices.size(); i += 3) {
            const gfxm::vec3& a = vertices[indices[i + 0]];
            const gfxm::vec3& b = vertices[indices[i + 1]];
            const gfxm::vec3& c = vertices[indices[i + 2]];

            const gfxm::vec3 N = gfxm::normalize(gfxm::cross(b - a, c - a));
            const gfxm::vec3 to_center = centroid - a;
            if (gfxm::dot(N, to_center) > .0f) {
                std::swap(indices[i + 1], indices[i + 2]);
            }
        }
    }
    void makeFaceNormals() {
        face_normals.resize(indices.size() / 3);
        for (int i = 0; i < indices.size(); i += 3) {
            uint32_t i0 = indices[i];
            uint32_t i1 = indices[i + 1];
            uint32_t i2 = indices[i + 2];
            const gfxm::vec3& a = vertices[i0];
            const gfxm::vec3& b = vertices[i1];
            const gfxm::vec3& c = vertices[i2];
            face_normals[i / 3] = gfxm::normalize(gfxm::cross(b - a, c - b));
        }
    }
    void makeEdgeList() {
        edges.clear();

        std::unordered_set<uint64_t> edge_set;
        for (int i = 0; i < indices.size(); i += 3) {
            uint32_t tri[3] = { indices[i], indices[i + 1], indices[i + 2] };
            for (int j = 0; j < 3; ++j) {
                uint32_t a = tri[j];
                uint32_t b = tri[(j + 1) % 3];
                
                uint32_t ka = gfxm::_min(a, b);
                uint32_t kb = gfxm::_max(a, b);
                uint64_t k = ka + (uint64_t(kb) << 32);
                if (edge_set.insert(k).second) {
                    edges.push_back(Edge{a, b});
                }
            }
        }
    }
    void findCentroid() {
        centroid = gfxm::vec3(0, 0, 0);
        for (int i = 0; i < vertices.size(); ++i) {
            centroid += vertices[i];
        }
        centroid /= float(vertices.size());
    }
public:
    void setData(const gfxm::vec3* verts, int vertex_count, const int* inds, int index_count) {
        vertices.clear();
        vertices.insert(vertices.end(), verts, verts + vertex_count);
        indices.clear();
        indices.insert(indices.end(), inds, inds + index_count);

        fixWinding();
        makeFaceNormals();
        makeEdgeList();
        findCentroid();
    }

    int vertexCount() const { return vertices.size(); }
    const gfxm::vec3* getVertexData() const { return vertices.data(); }

    int indexCount() const { return indices.size(); }
    const int* getIndexData() const { return indices.data(); }

    int uniqueEdgeCount() const { return edges.size(); }
    const Edge* getUniqueEdgeData() const { return edges.data(); }

    int faceCount() const { return indices.size() / 3; }
    const gfxm::vec3* getFaceNormalData() const { return face_normals.data(); }

    const gfxm::vec3& getCentroid() const { return centroid; }

    bool intersectRay(const gfxm::vec3& rO, const gfxm::vec3& rV) {
        assert(false);
        return false;
    }

    void rayTest(const gfxm::ray& ray, void* context, void(*callback_fn)(void*, const RayHitPoint&)) const {
        // TODO: Optimize

        for (int i = 0; i < indices.size(); i += 3) {
            const gfxm::vec3& A = vertices[indices[i + 0]];
            const gfxm::vec3& B = vertices[indices[i + 1]];
            const gfxm::vec3& C = vertices[indices[i + 2]];

            RayHitPoint rhp;
            if (intersectRayTriangle(ray, A, B, C, rhp)) {
                callback_fn(context, rhp);
            }
        }
    }

    void sweptSphereTest(const gfxm::vec3& from, const gfxm::vec3& to, float sweep_radius, void* context, void(*callback_fn)(void*, const SweepContactPoint&)) const {
        // TODO: Optimize

        for (int i = 0; i < indices.size(); i += 3) {
            const gfxm::vec3& A = vertices[indices[i + 0]];
            const gfxm::vec3& B = vertices[indices[i + 1]];
            const gfxm::vec3& C = vertices[indices[i + 2]];
            
            SweepContactPoint scp;
            if (intersectionSweepSphereTriangle(from, to, sweep_radius, A, B, C, scp)) {
                callback_fn(context, scp);
            }
        }
    }
    
    void debugDraw(const gfxm::mat4& transform, uint32_t color) const {
        if (indices.empty()) {
            assert(false);
            return;
        }
        for (int i = 0; i < indices.size(); i += 3) {
            dbgDrawLine(
                transform * gfxm::vec4(vertices[indices[i]], 1.f),
                transform * gfxm::vec4(vertices[indices[i + 1]], 1.f),
                color
            );
            dbgDrawLine(
                transform * gfxm::vec4(vertices[indices[i + 1]], 1.f),
                transform * gfxm::vec4(vertices[indices[i + 2]], 1.f),
                color
            );
            dbgDrawLine(
                transform * gfxm::vec4(vertices[indices[i + 2]], 1.f),
                transform * gfxm::vec4(vertices[indices[i]], 1.f),
                color
            );
        }/*
        for (int i = 0; i < vertices.size(); ++i) {
            dbgDrawSphere(transform * gfxm::vec4(vertices[i], 1.f), .01f, 0xFF0000FF);
        }*/
    }
};

