#pragma once

#include <assert.h>
#include <vector>
#include <set>
#include <stack>
#include "math/gfxm.hpp"
#include "mesh3d/mesh3d.hpp"
#include "debug_draw/debug_draw.hpp"
#include "collision/intersection/ray.hpp"
#include "collision/intersection/capsule_capsule.hpp"
#include "collision/intersection/sphere_capsule.hpp"
#include "log/log.hpp"


constexpr int MAX_STATIC_OCTREE_LEVELS = 5;
struct StaticOctreeNode {
    gfxm::aabb aabb;
    StaticOctreeNode* nodes[8];
    std::set<int> triangles;
    int level;

    StaticOctreeNode(int lvl)
    : level(lvl) {
        memset(&nodes[0], 0, sizeof(nodes));
    }

    bool isLeaf() const {
        return nodes[0] == nullptr;
    }

    void split() {
        assert(isLeaf());
        if (!isLeaf()) {
            return;
        }
        gfxm::vec3 center = aabb.from + (aabb.to - aabb.from) * .5f;
        nodes[0] = new StaticOctreeNode(level +1);
        nodes[0]->aabb.from = aabb.from;
        nodes[0]->aabb.to = center;
        nodes[1] = new StaticOctreeNode(level + 1);
        nodes[1]->aabb.from = gfxm::vec3(center.x, aabb.from.y, aabb.from.z);
        nodes[1]->aabb.to = gfxm::vec3(aabb.to.x, center.y, center.z);
        nodes[2] = new StaticOctreeNode(level + 1);
        nodes[2]->aabb.from = gfxm::vec3(aabb.from.x, aabb.from.y, center.z);
        nodes[2]->aabb.to = gfxm::vec3(center.x, center.y, aabb.to.z);
        nodes[3] = new StaticOctreeNode(level + 1);
        nodes[3]->aabb.from = gfxm::vec3(center.x, aabb.from.y, center.z);
        nodes[3]->aabb.to = gfxm::vec3(aabb.to.x, center.y, aabb.to.z);
        
        nodes[4] = new StaticOctreeNode(level + 1);
        nodes[4]->aabb.from = gfxm::vec3(aabb.from.x, center.y, aabb.from.z);
        nodes[4]->aabb.to = gfxm::vec3(center.x, aabb.to.y, center.z);
        nodes[5] = new StaticOctreeNode(level + 1);
        nodes[5]->aabb.from = gfxm::vec3(center.x, center.y, aabb.from.z);
        nodes[5]->aabb.to = gfxm::vec3(aabb.to.x, aabb.to.y, center.z);
        nodes[6] = new StaticOctreeNode(level + 1);
        nodes[6]->aabb.from = gfxm::vec3(aabb.from.x, center.y, center.z);
        nodes[6]->aabb.to = gfxm::vec3(center.x, aabb.to.y, aabb.to.z);
        nodes[7] = new StaticOctreeNode(level + 1);
        nodes[7]->aabb.from = center;
        nodes[7]->aabb.to = aabb.to;
    }

    int fitToChild(const gfxm::aabb& box) {
        gfxm::vec3 center = aabb.from + (aabb.to - aabb.from) * .5f;
        gfxm::aabb child_boxes[8];
        child_boxes[0].from = aabb.from;
        child_boxes[0].to = center;
        child_boxes[1].from = gfxm::vec3(center.x, aabb.from.y, aabb.from.z);
        child_boxes[1].to = gfxm::vec3(aabb.to.x, center.y, center.z);
        child_boxes[2].from = gfxm::vec3(aabb.from.x, aabb.from.y, center.z);
        child_boxes[2].to = gfxm::vec3(center.x, center.y, aabb.to.z);
        child_boxes[3].from = gfxm::vec3(center.x, aabb.from.y, center.z);
        child_boxes[3].to = gfxm::vec3(aabb.to.x, center.y, aabb.to.z);

        child_boxes[4].from = gfxm::vec3(aabb.from.x, center.y, aabb.from.z);
        child_boxes[4].to = gfxm::vec3(center.x, aabb.to.y, center.z);
        child_boxes[5].from = gfxm::vec3(center.x, center.y, aabb.from.z);
        child_boxes[5].to = gfxm::vec3(aabb.to.x, aabb.to.y, center.z);
        child_boxes[6].from = gfxm::vec3(aabb.from.x, center.y, center.z);
        child_boxes[6].to = gfxm::vec3(center.x, aabb.to.y, aabb.to.z);
        child_boxes[7].from = center;
        child_boxes[7].to = aabb.to;
        for (int i = 0; i < 8; ++i) {
            if (gfxm::aabb_in_aabb(box, child_boxes[i])) {
                return i;
            }
        }
        return -1;
    }

    bool insert(int triangleId, const gfxm::aabb& box) {
        if (!gfxm::aabb_in_aabb(box, aabb)) {
            return false;
        }
        if (level == MAX_STATIC_OCTREE_LEVELS) {
            triangles.insert(triangleId);
            return true;
        }
        int child_id = fitToChild(box);
        if (child_id == -1) {
            triangles.insert(triangleId);
            return true;
        } else {
            if (isLeaf()) {
                split();
            }
            return nodes[child_id]->insert(triangleId, box);
        }
    }

    void rayTest(const gfxm::ray& ray, void* context, void(*callback_fn)(void*, int)) const {
        if (intersectRayAabb(ray, aabb)) {
            dbgDrawAabb(aabb, DBG_COLOR_RED);
            for (auto& tri : triangles) {
                callback_fn(context, tri);
            }
            if (!isLeaf()) {
                for (int i = 0; i < 8; ++i) {
                    nodes[i]->rayTest(ray, context, callback_fn);
                }
            }
        }
    }

    void debugDraw() const {
        dbgDrawAabb(aabb, 0xFFCCCCCC);
        if (!isLeaf()) {
            for (int i = 0; i < 8; ++i) {
                nodes[i]->debugDraw();
            }
        }
    }
};
class StaticOctree {
    StaticOctreeNode root;
public:
    StaticOctree()
        : root(0) {}
    void init(const gfxm::aabb& aabb) {
        root.aabb = aabb;
    }
    void add(int triangleId, const gfxm::vec3& v0, const gfxm::vec3& v1, const gfxm::vec3& v2) {
        gfxm::aabb box;
        box.from = v0;
        box.to = v0;
        gfxm::expand_aabb(box, v1);
        gfxm::expand_aabb(box, v2);
        if (!root.insert(triangleId, box)) {
            assert(false);
        }
    }


    void rayTest(const gfxm::ray& ray, void* context, void(*callback_fn)(void*, int)) const {
        root.rayTest(ray, context, callback_fn);
    }

    void debugDraw() const {
        root.debugDraw();
    }
};


class CollisionTriangleMesh {
    std::vector<gfxm::vec3> vertices;
    std::vector<uint32_t> indices;

    struct Node {
        gfxm::aabb aabb;
        int left;
        int right;
        int triangle;
    };
    std::vector<Node> nodes;
public:
    void setData(const gfxm::vec3* vertices, size_t vertex_count, const uint32_t* indices, size_t index_count) {
        this->vertices.clear();
        this->indices.clear();
        
        if (vertex_count == 0 || index_count == 0) {
            assert(false);
            return;
        }        
        
        this->vertices.insert(this->vertices.end(), vertices, vertices + vertex_count);
        this->indices.insert(this->indices.end(), indices, indices + index_count);

        gfxm::aabb root_aabb;
        root_aabb.from = vertices[0];
        root_aabb.to = vertices[0];
        for (int i = 1; i < vertex_count; ++i) {
            gfxm::expand_aabb(root_aabb, vertices[i]);
        }
        float side = gfxm::_max(root_aabb.to.x - root_aabb.from.x, root_aabb.to.y - root_aabb.from.y);
        side = gfxm::_max(side, root_aabb.to.z - root_aabb.from.z);
        root_aabb.to.x = root_aabb.from.x + side;
        root_aabb.to.y = root_aabb.from.y + side;
        root_aabb.to.z = root_aabb.from.z + side;

        // aabb tree
        int tri_count = index_count / 3;
        std::vector<Node> nodes_;
        nodes_.resize(tri_count);
        for (int t = 0; t < tri_count; ++t) {
            uint32_t ia = indices[t * 3];
            uint32_t ib = indices[t * 3 + 1];
            uint32_t ic = indices[t * 3 + 2];
            gfxm::vec3 p0 = vertices[ia];
            gfxm::vec3 p1 = vertices[ib];
            gfxm::vec3 p2 = vertices[ic];
            gfxm::aabb aabb;
            aabb.from = p0;
            aabb.to = p0;
            gfxm::expand_aabb(aabb, p1);
            gfxm::expand_aabb(aabb, p2);
            Node n;
            n.aabb = aabb;
            n.left = -1;
            n.right = -1;
            n.triangle = t;
            nodes_[t] = n;
        }

        while (!nodes_.empty()) {
            std::vector<Node> parent_nodes;
            if (nodes_.size() > 1) {
                LOG_DBG("Node count: " << nodes_.size());
                if (nodes_.size() % 2 == 1) {
                    parent_nodes.push_back(nodes_.back());
                    nodes_.erase(nodes_.begin() + nodes_.size() - 1);
                }
                for (int i = 0; i < nodes_.size() - 1; i += 2) {
                    float min_volume_sum = INFINITY;
                    int sibling = -1;
                    gfxm::aabb min_aabb_merged;
                    for (int j = i + 1; j < nodes_.size(); ++j) {
                        gfxm::aabb aabb_merged = gfxm::aabb_union(nodes_[i].aabb, nodes_[j].aabb);
                        float volume_a = gfxm::volume(nodes_[i].aabb);
                        float volume_sum = gfxm::volume(aabb_merged);

                        if (volume_sum < min_volume_sum) {
                            min_volume_sum = volume_sum;
                            sibling = j;
                            min_aabb_merged = aabb_merged;
                        }
                        if (volume_a / volume_sum > .90f) {
                            break;
                        }
                    }
                    auto tmp = nodes_[i + 1];
                    nodes_[i + 1] = nodes_[sibling];
                    nodes_[sibling] = tmp;
                    Node n;
                    n.aabb = min_aabb_merged;
                    n.left = nodes.size() + i;
                    n.right = nodes.size() + i + 1;
                    n.triangle = -1;
                    parent_nodes.push_back(n);
                }
            }
            nodes.insert(nodes.end(), nodes_.begin(), nodes_.end());
            nodes_ = parent_nodes;
        }
    }
    const gfxm::vec3* getVertexData() const {
        return vertices.data();
    }
    const uint32_t* getIndexData() const {
        return indices.data();
    }
    size_t vertexCount() const {
        return vertices.size();
    }
    size_t indexCount() const {
        return indices.size();
    }

    void rayTest(const gfxm::ray& ray, void* context, void(*callback_fn)(void*, const RayHitPoint&)) const {
        //octree.rayTest(ray, context, callback_fn);
        std::stack<int> node_stack;
        node_stack.push(nodes.size() - 1);
        while (!node_stack.empty()) {
            int node_id = node_stack.top();
            node_stack.pop();
            auto& n = nodes[node_id];
            if (intersectRayAabb(ray, n.aabb)) {
                if (n.left == -1) { // is leaf
                    //dbgDrawAabb(n.aabb, DBG_COLOR_RED);
                    
                    uint32_t ia = indices[n.triangle * 3];
                    uint32_t ib = indices[n.triangle * 3 + 1];
                    uint32_t ic = indices[n.triangle * 3 + 2];
                    const gfxm::vec3& A = vertices[ia];
                    const gfxm::vec3& B = vertices[ib];
                    const gfxm::vec3& C = vertices[ic];

                    RayHitPoint rhp;
                    if (intersectRayTriangle(ray, A, B, C, rhp)) {
                        callback_fn(context, rhp);
                    }
                } else {
                    node_stack.push(n.left);
                    node_stack.push(n.right);
                }
            }
        }
    }
    void sweepSphereTest(const gfxm::vec3& from, const gfxm::vec3& to, float sweep_radius, void* context, void(*callback_fn)(void*, const SweepContactPoint&)) const {
        std::stack<int> node_stack;
        node_stack.push(nodes.size() - 1);
        while (!node_stack.empty()) {
            int node_id = node_stack.top();
            node_stack.pop();
            auto& n = nodes[node_id];

            if (intersectCapsuleAabb(from, to, sweep_radius, n.aabb)) {
                if (n.left == -1) { // is leaf
                    //dbgDrawAabb(n.aabb, DBG_COLOR_RED);
                    uint32_t ia = indices[n.triangle * 3];
                    uint32_t ib = indices[n.triangle * 3 + 1];
                    uint32_t ic = indices[n.triangle * 3 + 2];
                    const gfxm::vec3& A = vertices[ia];
                    const gfxm::vec3& B = vertices[ib];
                    const gfxm::vec3& C = vertices[ic];

                    SweepContactPoint scp;
                    if (intersectionSweepSphereTriangle(from, to, sweep_radius, A, B, C, scp)) {
                        callback_fn(context, scp);
                    }
                } else {
                    node_stack.push(n.left);
                    node_stack.push(n.right);
                }
            }
        }
    }

    bool intersectAabbAabb(const gfxm::aabb& a, const gfxm::aabb& b) const {
        bool x = a.from.x <= b.to.x && b.from.x <= a.to.x;
        bool y = a.from.y <= b.to.y && b.from.y <= a.to.y;
        bool z = a.from.z <= b.to.z && b.from.z <= a.to.z;
        return x && y && z;
    }
    int findPotentialTrianglesAabb(const gfxm::aabb& aabb, int* triangles, int max_triangles) const {
        int triangles_found = 0;

        std::stack<int> node_stack;
        node_stack.push(nodes.size() - 1);
        while (!node_stack.empty()) {
            if (max_triangles <= triangles_found) {
                break;
            }

            int node_id = node_stack.top();
            node_stack.pop();
            auto& n = nodes[node_id];
            if (intersectAabbAabb(aabb, n.aabb)) {
                if (n.left == -1) { // is leaf
                    triangles[triangles_found] = n.triangle;
                    triangles_found++;
                } else {
                    node_stack.push(n.left);
                    node_stack.push(n.right);
                }
            }
        }
        return triangles_found;
    }

    void debugDraw(const gfxm::mat4& transform, uint32_t color) const {
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
        }
        /*
        for (auto& n : nodes) {
            dbgDrawAabb(n.aabb, DBG_COLOR_WHITE);
        }*/
        //octree.debugDraw();
    }

    void serialize(std::vector<uint8_t>& data) {
        Mesh3d mesh;
        mesh.setAttribArray(VFMT::Position_GUID, vertices.data(), vertices.size() * sizeof(vertices[0]));
        mesh.setIndexArray(indices.data(), indices.size() * sizeof(indices[0]));
        mesh.serialize(data);
    }
    void deserialize(const std::vector<uint8_t>& data) {
        Mesh3d mesh;
        mesh.deserialize(data.data(), data.size());
        auto vertex_data = mesh.getAttribArrayData(VFMT::Position_GUID);
        auto vertex_array_size = mesh.getAttribArraySize(VFMT::Position_GUID);
        auto index_data = mesh.getIndexArrayData();
        auto index_array_size = mesh.getIndexArraySize();
        setData((const gfxm::vec3*)vertex_data, vertex_array_size / sizeof(gfxm::vec3), (const uint32_t*)index_data, index_array_size / sizeof(uint32_t));
    }
};