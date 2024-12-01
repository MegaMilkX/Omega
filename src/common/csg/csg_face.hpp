#pragma once

#include "csg_material.hpp"
#include "csg_fragment.hpp"

struct csgBrushShape;
struct csgFace {
    csgBrushShape* shape;
    gfxm::vec3 N;
    gfxm::vec3 lclN;
    float D;
    float lclD;

    gfxm::vec3 mid_point;
    gfxm::aabb aabb_world;
    std::vector<csgFragment> fragments;
    //std::vector<csgVertex> vertices;
    //std::vector<int> indices;
    //std::set<int> tmp_indices;
    std::set<csgVertex*> tmp_control_points;
    std::vector<csgVertex*> control_points;
    std::vector<gfxm::vec3> lcl_normals;
    std::vector<gfxm::vec3> normals;
    std::vector<gfxm::vec2> uvs;

    csgMaterial* material = 0;
    gfxm::vec2 uv_scale = gfxm::vec2(1.f, 1.f);
    gfxm::vec2 uv_offset = gfxm::vec2(.0f, .0f);

    int vertexCount() const { return control_points.size(); }
    const gfxm::vec3& getLocalVertexPos(int i) const;
    const gfxm::vec3& getWorldVertexPos(int i) const;
};

