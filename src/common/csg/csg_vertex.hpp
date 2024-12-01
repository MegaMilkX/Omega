#pragma once

#include <set>
#include "math/gfxm.hpp"
#include "csg_types.hpp"


constexpr float VERTEX_HASH_PRECISION = 100.0f;


struct csgFace;
struct csgVertex {
    int index; // index to the shape's control_point array
    gfxm::vec3 position;
    gfxm::vec3 normal; // Only used for fragments
    gfxm::vec2 uv; // Only used for fragments
    std::set<csgFace*> faces;
    CSG_RELATION_TYPE tmp_relation;

    bool operator==(const csgVertex& other) const {
        const int ix = round(position.x * VERTEX_HASH_PRECISION);
        const int iy = round(position.y * VERTEX_HASH_PRECISION);
        const int iz = round(position.z * VERTEX_HASH_PRECISION);
        const int oix = round(other.position.x * VERTEX_HASH_PRECISION);
        const int oiy = round(other.position.y * VERTEX_HASH_PRECISION);
        const int oiz = round(other.position.z * VERTEX_HASH_PRECISION);
        return ix == oix && iy == oiy && iz == oiz;
    }
};
inline bool csgCompareVertices(const csgVertex& a, const csgVertex& b) {
    return a == b;
}
inline void vertex_hash_combine(std::size_t& seed, const int& v) {
    seed ^= std::hash<int>()(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}
template<>
struct std::hash<csgVertex> {
    std::size_t operator()(const csgVertex& v) const {
        std::size_t v_ = 0x778abe;
        vertex_hash_combine(v_, round(v.position.x * VERTEX_HASH_PRECISION));
        vertex_hash_combine(v_, round(v.position.y * VERTEX_HASH_PRECISION));
        vertex_hash_combine(v_, round(v.position.z * VERTEX_HASH_PRECISION));
        return v_;
    }
};
