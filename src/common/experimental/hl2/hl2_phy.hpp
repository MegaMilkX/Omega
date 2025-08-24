#pragma once

#include <vector>
#include "math/gfxm.hpp"
#include "collision/collider.hpp"


struct legacysurfaceheader_t {
    float	mass_center[3];
    float	rotation_inertia[3];
    float	upper_limit_radius;
    int32_t	max_deviation : 8;
    int32_t	byte_size : 24;
    int32_t	offset_ledgetree_root;
    int32_t	dummy[3]; 		// dummy[2] is "IVPS" or 0
};
static_assert(sizeof(legacysurfaceheader_t) == 48);

struct compactsurfaceheader_t {
    int32_t	vphysicsID;		// Generally the ASCII for "VPHY" in newer files
    int16_t	version;
    int16_t	modelType;
    int32_t	surfaceSize;
    gfxm::vec3	dragAxisAreas;
    int32_t	axisMapSize;
};
static_assert(sizeof(compactsurfaceheader_t) == 28);

struct convexsolidheader_t {
    int32_t vertices_offset; // For each convexsolidheader_t it will point to the same address of the file
    int32_t bone_index;
    int32_t flags;
    int32_t triangles_count;
};
static_assert(sizeof(convexsolidheader_t) == 16);

#pragma pack(push, 1)
struct triangledata_t {
    uint8_t vertex_index; // can be used to calculate the number of vertices used below
    int8_t unused1;
    uint16_t unused2;

    int16_t vertex1_index;
    int16_t unused3;
    int16_t vertex2_index;
    int16_t unused4;
    int16_t vertex3_index;
    int16_t unused5;
};
static_assert(sizeof(triangledata_t) == 16);
#pragma pack(pop)

struct phyvertex_t {
    gfxm::vec3 pos; // relative to bone
    float unknown;
};

struct phynode_t {
    int32_t rightnodeindex;
    int32_t convexindex;

    gfxm::vec3 center;
    float radius;

    int32_t bboxsize; // volume?
};

typedef struct phyheader_s {
    int32_t		size;          // Size of this header section (generally 16)
    int32_t		id;            // Often zero, unknown purpose.
    int32_t		solidCount;    // Number of solids in file
    int32_t     checkSum;	   // checksum of source .mdl file (32 bits)
    /*
    const legacysurfaceheader_t* getSurfaceHeader(int i) const {
        return (const legacysurfaceheader_t*)((uint8_t*)this + sizeof(phyheader_t)) + i;
    }*/
    const compactsurfaceheader_t* getSurfaceHeader(int i) const {
        return (const compactsurfaceheader_t*)((uint8_t*)this + sizeof(phyheader_t)) + i;
    }
} phyheader_t;
static_assert(sizeof(phyheader_t) == 16);

struct PHYShape {
    std::vector<gfxm::vec3> vertices;
};

struct PHYFile {
    phyheader_t head;

    gfxm::mat3 inertia_tensor;

    std::shared_ptr<CollisionConvexMesh> root_mesh;
    std::vector<std::shared_ptr<CollisionConvexMesh>> meshes;
};

bool hl2LoadPHY(const char* path, PHYFile& out);