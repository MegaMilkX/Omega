
#include "generate_primitive.hpp"
#include "math/gfxm.hpp"


void meshGenerateCube(Mesh3d* out, float width, float height, float depth) {
    out->clear();

    float w = width / 2.0f;
    float h = height / 2.0f;
    float d = depth / 2.0f;

    float vertices[] = {
        -w, -h,  d,   w, -h,  d,    w,  h,  d,
         w,  h,  d,  -w,  h,  d,   -w, -h,  d,

         w, -h,  d,   w, -h, -d,    w,  h, -d,
         w,  h, -d,   w,  h,  d,    w, -h,  d,

         w, -h, -d,  -w, -h, -d,   -w,  h, -d,
        -w,  h, -d,   w,  h, -d,    w, -h, -d,

        -w, -h, -d,  -w, -h,  d,   -w,  h,  d,
        -w,  h,  d,  -w,  h, -d,   -w, -h, -d,

        -w,  h,  d,   w,  h,  d,    w,  h, -d,
         w,  h, -d,  -w,  h, -d,   -w,  h,  d,

        -w, -h, -d,   w, -h, -d,    w, -h,  d,
         w, -h,  d,  -w, -h,  d,   -w, -h, -d
    };
    unsigned char color[] = {
        255, 255, 255,  255, 255, 255,   255, 255, 255,
        255, 255, 255,  255, 255, 255,   255, 255, 255,

        255, 255, 255,  255, 255, 255,   255, 255, 255,
        255, 255, 255,  255, 255, 255,   255, 255, 255,

        255, 255, 255,  255, 255, 255,   255, 255, 255,
        255, 255, 255,  255, 255, 255,   255, 255, 255,

        255, 255, 255,  255, 255, 255,   255, 255, 255,
        255, 255, 255,  255, 255, 255,   255, 255, 255,

        255, 255, 255,  255, 255, 255,   255, 255, 255,
        255, 255, 255,  255, 255, 255,   255, 255, 255,

        255, 255, 255,  255, 255, 255,   255, 255, 255,
        255, 255, 255,  255, 255, 255,   255, 255, 255
    };
    float uv[] = {
        .0f, .0f,   1.f, .0f,   1.f, 1.f,
        1.f, 1.f,   .0f, 1.f,   .0f, .0f,

        .0f, .0f,   1.f, .0f,   1.f, 1.f,
        1.f, 1.f,   .0f, 1.f,   .0f, .0f,

        .0f, .0f,   1.f, .0f,   1.f, 1.f,
        1.f, 1.f,   .0f, 1.f,   .0f, .0f,

        .0f, .0f,   1.f, .0f,   1.f, 1.f,
        1.f, 1.f,   .0f, 1.f,   .0f, .0f,

        .0f, .0f,   1.f, .0f,   1.f, 1.f,
        1.f, 1.f,   .0f, 1.f,   .0f, .0f,

        .0f, .0f,   1.f, .0f,   1.f, 1.f,
        1.f, 1.f,   .0f, 1.f,   .0f, .0f
    };
    float normals[] = {
        .0f, .0f, 1.f, .0f, .0f, 1.f, .0f, .0f, 1.f, .0f, .0f, 1.f, .0f, .0f, 1.f, .0f, .0f, 1.f,
        1.f, .0f, .0f, 1.f, .0f, .0f, 1.f, .0f, .0f, 1.f, .0f, .0f, 1.f, .0f, .0f, 1.f, .0f, .0f,
        .0f, .0f,-1.f, .0f, .0f,-1.f, .0f, .0f,-1.f, .0f, .0f,-1.f, .0f, .0f,-1.f, .0f, .0f,-1.f,
        -1.f, .0f, .0f,-1.f, .0f, .0f,-1.f, .0f, .0f,-1.f, .0f, .0f,-1.f, .0f, .0f,-1.f, .0f, .0f,
        .0f, 1.f, .0f, .0f, 1.f, .0f, .0f, 1.f, .0f, .0f, 1.f, .0f, .0f, 1.f, .0f, .0f, 1.f, .0f,
        .0f,-1.f, .0f, .0f,-1.f, .0f, .0f,-1.f, .0f, .0f,-1.f, .0f, .0f,-1.f, .0f, .0f,-1.f, .0f
    };
    uint32_t indices[] = {
        0,  1,  2,  3,  4,  5,
        6,  7,  8,  9, 10, 11,
        12, 13, 14, 15, 16, 17,
        18, 19, 20, 21, 22, 23,
        24, 25, 26, 27, 28, 29,
        30, 31, 32, 33, 34, 35
    };
    
    out->setAttribArray(VFMT::Position_GUID, vertices, sizeof(vertices));
    out->setAttribArray(VFMT::ColorRGB_GUID, color, sizeof(color));
    out->setAttribArray(VFMT::UV_GUID, uv, sizeof(uv));
    out->setAttribArray(VFMT::Normal_GUID, normals, sizeof(normals));
    out->setIndexArray(indices, sizeof(indices));
}

void meshGenerateSphereCubic(Mesh3d* out, float radius, int detail) {
    constexpr int CUBE_SIDE_COUNT = 6;
    std::vector<float> vertices;
    std::vector<float> uv;
    std::vector<unsigned char> colorRGB;
    std::vector<float> normals;
    std::vector<uint32_t> indices;
    int side_vert_count = (detail + 1) * (detail + 1);
    int vert_count = side_vert_count * CUBE_SIDE_COUNT;
    int side = detail + 1;
    int side_tri_count = detail * detail * 2;
    int tri_count = side_tri_count * CUBE_SIDE_COUNT;
    vertices.resize(vert_count * 3);
    uv.resize(vert_count * 2);
    colorRGB.resize(vert_count * 3);
    normals.resize(vert_count * 3);
    indices.resize(tri_count * 3);
    
    gfxm::mat4 matrices[CUBE_SIDE_COUNT] = {
        gfxm::mat4(1.0f),
        gfxm::mat4(1.0f),
        gfxm::mat4(1.0f),
        gfxm::mat4(1.0f),
        gfxm::mat4(1.0f),
        gfxm::mat4(1.0f)
    };
    matrices[1][0] = gfxm::vec4(0, 0, 1, 0);
    matrices[1][1] = gfxm::vec4(0, 1, 0, 0);
    matrices[1][2] = gfxm::vec4(-1, 0, 0, 0);
    matrices[2][0] = gfxm::vec4(-1, 0, 0, 0);
    matrices[2][1] = gfxm::vec4(0, 1, 0, 0);
    matrices[2][2] = gfxm::vec4(0, 0, -1, 0);
    matrices[3][0] = gfxm::vec4(0, 0, -1, 0);
    matrices[3][1] = gfxm::vec4(0, 1, 0, 0);
    matrices[3][2] = gfxm::vec4(1, 0, 0, 0);
    matrices[4][0] = gfxm::vec4(1, 0, 0, 0);
    matrices[4][1] = gfxm::vec4(0, 0, -1, 0);
    matrices[4][2] = gfxm::vec4(0, 1, 0, 0);
    matrices[5][0] = gfxm::vec4(1, 0, 0, 0);
    matrices[5][1] = gfxm::vec4(0, 0, 1, 0);
    matrices[5][2] = gfxm::vec4(0, -1, 0, 0);

    
    for (int m = 0; m < CUBE_SIDE_COUNT; ++m) {
        int base_index = side_vert_count * m;
        for (int i = 0; i < side_vert_count; ++i) {
            int vi = side_vert_count * m + i;
            int ix = i % side;
            int iy = i / side;
            float fx = ix / (float)(side - 1) - 0.5f;
            float fy = iy / (float)(side - 1) - 0.5f;
            float u = ix / (float)(side - 1);
            float v = iy / (float)(side - 1);
            gfxm::vec3 vert(fx, fy, .5f);
            vert = matrices[m] * gfxm::vec4(vert, .0f);
            gfxm::vec3 norm = gfxm::normalize(vert);
            vert = norm * radius;
            vertices[vi * 3] = vert.x;
            vertices[vi * 3 + 1] = vert.y;
            vertices[vi * 3 + 2] = vert.z;
            uv[vi * 2] = u;
            uv[vi * 2 + 1] = v;
            colorRGB[vi * 3] = 255;
            colorRGB[vi * 3 + 1] = 255;
            colorRGB[vi * 3 + 2] = 255;
            normals[vi * 3] = norm.x;
            normals[vi * 3 + 1] = norm.y;
            normals[vi * 3 + 2] = norm.z;
        }    
        for (int i = 0; i < side_tri_count / 2; ++i) {
            int vi = (side_tri_count / 2) * m + i;
            int ix = i % (side - 1);
            int iy = i / (side - 1);
            int ii = iy + i;
            indices[vi * 6    ] = base_index + ii;
            indices[vi * 6 + 1] = base_index + ii + side + 1;
            indices[vi * 6 + 2] = base_index + ii + side;
            indices[vi * 6 + 3] = base_index + ii + side + 1;
            indices[vi * 6 + 4] = base_index + ii;
            indices[vi * 6 + 5] = base_index + ii + 1;
        }
    }

    out->setAttribArray(VFMT::Position_GUID, vertices.data(), vertices.size() * sizeof(vertices[0]));
    out->setAttribArray(VFMT::UV_GUID, uv.data(), uv.size() * sizeof(uv[0]));
    out->setAttribArray(VFMT::ColorRGB_GUID, colorRGB.data(), colorRGB.size() * sizeof(colorRGB[0]));
    out->setAttribArray(VFMT::Normal_GUID, normals.data(), normals.size() * sizeof(normals[0]));
    out->setIndexArray(indices.data(), indices.size() * sizeof(indices[0]));
}

void meshGenerateCheckerPlane(Mesh3d* out, float width, float depth, int checker_density) {
    out->clear();

    float w_checker = width / checker_density;
    float d_checker = depth / checker_density;

    std::vector<float> vertices;
    std::vector<unsigned char> colors;
    std::vector<float> normals;
    std::vector<uint32_t> indices;
    vertices.resize(3 * 4 * checker_density * checker_density);
    colors.resize(3 * 4 * checker_density * checker_density);
    normals.resize(3 * 4 * checker_density * checker_density);
    indices.resize(checker_density * checker_density * 6);

    for (int i = 0; i < checker_density * checker_density; ++i) {
        int x = i % (int)checker_density;
        int z = i / checker_density;
        float fx = w_checker * (float)x - (width / 2.0f);
        float fz = d_checker * (float)z - (depth / 2.0f);
        float checker_vertices[] = {
            fx, .0f, fz, 
            fx, .0f, fz + d_checker,
            fx + w_checker, .0f, fz + d_checker,
            fx + w_checker, .0f, fz
        };
        memcpy(&vertices[i * 4 * 3], checker_vertices, sizeof(checker_vertices));
        float checker_normals[] = {
            .0f, 1.f, .0f,  .0f, 1.f, .0f,  .0f, 1.f, .0f,  .0f, 1.f, .0f
        };
        memcpy(&normals[i * 4 * 3], checker_normals, sizeof(checker_normals));

        unsigned char col[] = {
            150,150,150,
            90,90,90
        };
        int color_index = (x + z) % 2;
        unsigned char checker_colors[] = {
            col[color_index * 3], col[color_index * 3 + 1], col[color_index * 3 + 2],
            col[color_index * 3], col[color_index * 3 + 1], col[color_index * 3 + 2],
            col[color_index * 3], col[color_index * 3 + 1], col[color_index * 3 + 2],
            col[color_index * 3], col[color_index * 3 + 1], col[color_index * 3 + 2]
        };
        memcpy(&colors[i * 4 * 3], checker_colors, sizeof(checker_colors));

        uint32_t base_index = i * 4;
        uint32_t checker_indices[] = {
            base_index, base_index + 1, base_index + 3, base_index + 1, base_index + 2, base_index + 3
        };
        memcpy(&indices[i * 6], checker_indices, sizeof(checker_indices));
    }

    out->setAttribArray(VFMT::Position_GUID, vertices.data(), vertices.size() * sizeof(vertices[0]));
    out->setAttribArray(VFMT::ColorRGB_GUID, colors.data(), colors.size() * sizeof(colors[0]));
    out->setAttribArray(VFMT::Normal_GUID, normals.data(), normals.size() * sizeof(normals[0]));
    out->setIndexArray(indices.data(), indices.size() * sizeof(indices[0]));
}


#include "common/util/marching_cubes/tables.hpp"
#include <FastNoiseSIMD.h>
gfxm::vec3 interpolateEdgeVertex(const gfxm::vec3& v1, const gfxm::vec3& v2, float a, float b) {
    constexpr float threshold = 0.1f;
    float mu = (threshold - a) / (b - a);
    gfxm::vec3 p;
    p.x = v1.x + mu * (float)(v2.x - v1.x);
    p.y = v1.y + mu * (float)(v2.y - v1.y);
    p.z = v1.z + mu * (float)(v2.z - v1.z);
    return p;
}
size_t indexFromCoord(int x, int y, int z, int sideSize) {
    return x * sideSize * sideSize + y * sideSize + z;
}
gfxm::vec2 sampleSphericalMap(gfxm::vec3 v) {
    const gfxm::vec2 invAtan = gfxm::vec2(0.1591f, 0.3183f);
    gfxm::vec2 uv = gfxm::vec2(atan2f(v.z, v.x), asinf(v.y));
    uv *= invAtan;
    uv = gfxm::vec2(uv.x + 0.5, uv.y + 0.5);
    return uv;
}
void meshGenerateVoxelField(Mesh3d* out, float offset_x, float offset_y, float offset_z) {
    constexpr float threshold = 0.1f;
    constexpr int fieldSize = 64;
    constexpr int voxelArraySize = fieldSize * fieldSize * fieldSize;

    float* voxelField = new float[voxelArraySize];

    FastNoiseSIMD* noise = FastNoiseSIMD::NewFastNoiseSIMD();
    noise->SetNoiseType(FastNoiseSIMD::PerlinFractal);
    //noise->SetAxisScales(noiseScale, noiseScale, noiseScale);
    noise->SetFrequency(0.02f);
    float* ptr = noise->GetNoiseSet(offset_x, offset_y, offset_z, fieldSize, fieldSize, fieldSize, 3.5f);
    memcpy(voxelField, ptr, fieldSize*fieldSize*fieldSize * sizeof(float));
    noise->FreeNoiseSet(ptr);
    delete noise;

    for (int i = 0; i < fieldSize; ++i) {
        for (int j = 0; j < fieldSize; ++j) {
            for (int k = 0; k < fieldSize; ++k) {
                if (i <= 1 || i >= fieldSize - 2 || j <= 1 || j >= fieldSize - 2 || k <= 1 || k >= fieldSize - 2) {
                    voxelField[i] = 0.0f;
                }
            }
        }
    }

    std::vector<gfxm::vec3> tangents;
    std::vector<gfxm::vec3> bitangents;
    std::vector<uint32_t> indices;

    std::vector<gfxm::vec3> edge_vertex_grid;
    edge_vertex_grid.resize(fieldSize*fieldSize*fieldSize * 3);
    std::vector<gfxm::vec3> edge_vertex_normal_grid;
    edge_vertex_normal_grid.resize(fieldSize*fieldSize*fieldSize * 3);
    std::vector<gfxm::vec2> edge_vertex_uv_grid;
    edge_vertex_uv_grid.resize(fieldSize*fieldSize*fieldSize * 3);
    std::vector<unsigned char> edge_vertex_color_grid;
    edge_vertex_color_grid.resize(fieldSize * fieldSize * fieldSize * 3);
    memset(edge_vertex_color_grid.data(), 255, edge_vertex_color_grid.size());

    for (int z = 0; z < fieldSize - 1; ++z) {
        for (int y = 0; y < fieldSize - 1; ++y) {
            for (int x = 0; x < fieldSize - 1; ++x) {
                size_t cell_idx = indexFromCoord(x, y, z, fieldSize);
                // ngb - short for neighbor
                size_t cell_idx_x_ngb = indexFromCoord(x + 1, y, z, fieldSize);
                size_t cell_idx_z_ngb = indexFromCoord(x, y, z + 1, fieldSize);
                size_t cell_idx_y_ngb = indexFromCoord(x, y + 1, z, fieldSize);
                size_t cell_idx_xy_ngb = indexFromCoord(x + 1, y + 1, z, fieldSize);
                size_t cell_idx_yz_ngb = indexFromCoord(x, y + 1, z + 1, fieldSize);
                size_t cell_idx_xz_ngb = indexFromCoord(x + 1, y, z + 1, fieldSize);
                size_t arrayIndices[] = {
                    indexFromCoord(x, y, z, fieldSize),
                    indexFromCoord(x + 1, y, z, fieldSize),
                    indexFromCoord(x + 1, y, z + 1, fieldSize),
                    indexFromCoord(x, y, z + 1, fieldSize),
                    indexFromCoord(x, y + 1, z, fieldSize),
                    indexFromCoord(x + 1, y + 1, z, fieldSize),
                    indexFromCoord(x + 1, y + 1, z + 1, fieldSize),
                    indexFromCoord(x, y + 1, z + 1, fieldSize)
                };
                float densities[8];
                const gfxm::vec3 corners[] = {
                    {  .0f + x,  .0f + y,  .0f + z },
                    { 1.0f + x,  .0f + y,  .0f + z },
                    { 1.0f + x,  .0f + y, 1.0f + z },
                    {  .0f + x,  .0f + y, 1.0f + z },
                    {  .0f + x, 1.0f + y,  .0f + z },
                    { 1.0f + x, 1.0f + y,  .0f + z },
                    { 1.0f + x, 1.0f + y, 1.0f + z },
                    {  .0f + x, 1.0f + y, 1.0f + z }
                };
                size_t cubeIndex = 0;
                for (int i = 0; i < 8; ++i) {
                    densities[i] = voxelField[arrayIndices[i]];
                    if (voxelField[arrayIndices[i]] > threshold) {
                        cubeIndex |= 1 << i;
                    }
                }

                if (edgeTable[cubeIndex] == 0 || edgeTable[cubeIndex] == 255) continue;

                gfxm::vec3 edge_vertices[12];

                const int local_vertex_to_edge_grid[] = {
                    cell_idx * 3 + 0,
                    cell_idx_x_ngb * 3 + 1,
                    cell_idx_z_ngb * 3 + 0,
                    cell_idx * 3 + 1,
                    cell_idx_y_ngb * 3 + 0,
                    cell_idx_xy_ngb * 3 + 1,
                    cell_idx_yz_ngb * 3 + 0,
                    cell_idx_y_ngb * 3 + 1,
                    cell_idx * 3 + 2,
                    cell_idx_x_ngb * 3 + 2,
                    cell_idx_xz_ngb * 3 + 2,
                    cell_idx_z_ngb * 3 + 2
                };

                if ((edgeTable[cubeIndex] & 1) == 1)
                    edge_vertex_grid[local_vertex_to_edge_grid[0]] = interpolateEdgeVertex(corners[0], corners[1], densities[0], densities[1]);
                if ((edgeTable[cubeIndex] & 2) == 2)
                    edge_vertex_grid[local_vertex_to_edge_grid[1]] = interpolateEdgeVertex(corners[1], corners[2], densities[1], densities[2]);
                if ((edgeTable[cubeIndex] & 4) == 4)
                    edge_vertex_grid[local_vertex_to_edge_grid[2]] = interpolateEdgeVertex(corners[2], corners[3], densities[2], densities[3]);
                if ((edgeTable[cubeIndex] & 8) == 8)
                    edge_vertex_grid[local_vertex_to_edge_grid[3]] = interpolateEdgeVertex(corners[3], corners[0], densities[3], densities[0]);
                if ((edgeTable[cubeIndex] & 16) == 16)
                    edge_vertex_grid[local_vertex_to_edge_grid[4]] = interpolateEdgeVertex(corners[4], corners[5], densities[4], densities[5]);
                if ((edgeTable[cubeIndex] & 32) == 32)
                    edge_vertex_grid[local_vertex_to_edge_grid[5]] = interpolateEdgeVertex(corners[5], corners[6], densities[5], densities[6]);
                if ((edgeTable[cubeIndex] & 64) == 64)
                    edge_vertex_grid[local_vertex_to_edge_grid[6]] = interpolateEdgeVertex(corners[6], corners[7], densities[6], densities[7]);
                if ((edgeTable[cubeIndex] & 128) == 128)
                    edge_vertex_grid[local_vertex_to_edge_grid[7]] = interpolateEdgeVertex(corners[7], corners[4], densities[7], densities[4]);
                if ((edgeTable[cubeIndex] & 256) == 256)
                    edge_vertex_grid[local_vertex_to_edge_grid[8]] = interpolateEdgeVertex(corners[0], corners[4], densities[0], densities[4]);
                if ((edgeTable[cubeIndex] & 512) == 512)
                    edge_vertex_grid[local_vertex_to_edge_grid[9]] = interpolateEdgeVertex(corners[1], corners[5], densities[1], densities[5]);
                if ((edgeTable[cubeIndex] & 1024) == 1024)
                    edge_vertex_grid[local_vertex_to_edge_grid[10]] = interpolateEdgeVertex(corners[2], corners[6], densities[2], densities[6]);
                if ((edgeTable[cubeIndex] & 2048) == 2048)
                    edge_vertex_grid[local_vertex_to_edge_grid[11]] = interpolateEdgeVertex(corners[3], corners[7], densities[3], densities[7]);

                for (int i = 0; i < 16; ++i) {
                    int vertex_idx = triTable[cubeIndex][i];
                    if (vertex_idx < 0) break;

                    indices.emplace_back(local_vertex_to_edge_grid[vertex_idx]);
                }
            }
        }
    }

    // for each triangle
    for (size_t i = 0; i < indices.size(); i += 3) {
        size_t idx_a = indices[i];
        size_t idx_b = indices[i + 1];
        size_t idx_c = indices[i + 2];
        const gfxm::vec3& a = edge_vertex_grid[idx_a];
        const gfxm::vec3& b = edge_vertex_grid[idx_b];
        const gfxm::vec3& c = edge_vertex_grid[idx_c];
        gfxm::vec3 edge_a = (b - a);
        gfxm::vec3 edge_b = (c - a);
        gfxm::vec3 normal = gfxm::normalize(gfxm::cross(edge_a, edge_b));

        gfxm::vec3& na = edge_vertex_normal_grid[idx_a];
        gfxm::vec3& nb = edge_vertex_normal_grid[idx_b];
        gfxm::vec3& nc = edge_vertex_normal_grid[idx_c];


        na += normal;
        na = gfxm::normalize(na);
        nb += normal;
        nb = gfxm::normalize(nb);
        nc += normal;
        nc = gfxm::normalize(nc);
    }

    for (size_t i = 0; i < edge_vertex_normal_grid.size(); ++i) {
        gfxm::vec3 normal = edge_vertex_normal_grid[i];

        gfxm::vec2 u = sampleSphericalMap(normal);

        edge_vertex_uv_grid[i] = u;
    }

    tangents.resize(edge_vertex_grid.size());
    bitangents.resize(edge_vertex_grid.size());
    for (size_t i = 0; i < indices.size(); i += 3) {
        size_t idx_a = indices[i];
        size_t idx_b = indices[i + 1];
        size_t idx_c = indices[i + 2];

        gfxm::vec3 a = edge_vertex_grid[idx_a];
        gfxm::vec3 b = edge_vertex_grid[idx_b];
        gfxm::vec3 c = edge_vertex_grid[idx_c];
        gfxm::vec2 uva = edge_vertex_uv_grid[idx_a];
        gfxm::vec2 uvb = edge_vertex_uv_grid[idx_b];
        gfxm::vec2 uvc = edge_vertex_uv_grid[idx_c];

        gfxm::vec3 deltaPos1 = b - a;
        gfxm::vec3 deltaPos2 = c - a;

        gfxm::vec2 deltaUV1 = uvb - uva;
        gfxm::vec2 deltaUV2 = uvc - uva;

        float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
        gfxm::vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
        gfxm::vec3 bitangent = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * r;
        tangent = gfxm::normalize(tangent);
        bitangent = gfxm::normalize(bitangent);

        tangents[idx_a] = tangent;
        tangents[idx_b] = tangent;
        tangents[idx_c] = tangent;

        bitangents[idx_a] = bitangent;
        bitangents[idx_b] = bitangent;
        bitangents[idx_c] = bitangent;
    }

    out->setAttribArray(VFMT::Position_GUID, edge_vertex_grid.data(), edge_vertex_grid.size() * sizeof(gfxm::vec3));
    out->setAttribArray(VFMT::Normal_GUID, edge_vertex_normal_grid.data(), edge_vertex_normal_grid.size() * sizeof(gfxm::vec3));
    out->setAttribArray(VFMT::ColorRGB_GUID, edge_vertex_color_grid.data(), edge_vertex_color_grid.size());
    //out->setAttribArray(VFMT::ENUM_GENERIC::Tangent, tangents.data(), tangents.size() * sizeof(gfxm::vec3));
    //out->setAttribArray(VFMT::ENUM_GENERIC::Bitangent, bitangents.data(), bitangents.size() * sizeof(gfxm::vec3));
    out->setAttribArray(VFMT::UV_GUID, edge_vertex_uv_grid.data(), edge_vertex_uv_grid.size() * sizeof(gfxm::vec2));
    out->setIndexArray(indices.data(), indices.size() * sizeof(indices[0]));

    delete [] voxelField;
}