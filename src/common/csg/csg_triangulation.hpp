#pragma once

#include <vector>
#include <unordered_map>
#include "csg_material.hpp"

struct csgBrushShape;


struct csgMeshData {
    csgMaterial* material = 0;
    std::vector<gfxm::vec3> vertices;
    std::vector<gfxm::vec3> normals;
    std::vector<gfxm::vec3> tangents;
    std::vector<gfxm::vec3> bitangents;
    std::vector<uint32_t> colors;
    std::vector<gfxm::vec2> uvs;
    std::vector<gfxm::vec2> uvs_lightmap;
    std::vector<uint32_t> indices;
};


void csgTriangulateShape(csgBrushShape* shape, csgMeshData* mesh_data);

void csgMakeShapeTriangles(
    csgBrushShape* shape,
    std::unordered_map<csgMaterial*, csgMeshData>& mesh_data
);
