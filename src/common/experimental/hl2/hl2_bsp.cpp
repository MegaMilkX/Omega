#include "hl2_bsp.hpp"

#include <stdio.h>
#include <vector>
#include <filesystem>
#include "log/log.hpp"

#include "hl2_vtf.hpp"
#include "hl2_material.hpp"
#include "hl2_mdl.hpp"

#include "valve_data/parser/parse.hpp"

#include "typeface/rect_pack.hpp"

// little-endian "VBSP"   0x50534256
#define IDBSPHEADER_LE	(('P'<<24)+('S'<<16)+('B'<<8)+'V')
#define IDBSPHEADER_BE	(('V'<<24)+('B'<<16)+('S'<<8)+'P')

#define FREAD(BUFFER, SIZE, COUNT, FILE) \
if (fread(BUFFER, SIZE, COUNT, FILE) != COUNT) { \
    assert(false); \
    return false; \
}
#define FREAD_MSG(BUFFER, SIZE, COUNT, FILE, COMMENT) \
if (fread(BUFFER, SIZE, COUNT, FILE) != COUNT) { \
    LOG_ERR("'" << COMMENT << "': " << "Failed to FREAD"); \
    assert(false); \
    return false; \
}

#define ON_ERROR \
fclose(f); \
assert(false); \
return false;

constexpr int N_BSP_LUMPS = 64;

enum LUMP_TYPE {
    LUMP_ENTITIES,
    LUMP_PLANES,
    LUMP_TEXDATA,
    LUMP_VERTEXES,
    LUMP_VISIBILITY,
    LUMP_NODES,
    LUMP_TEXINFO,
    LUMP_FACES,
    LUMP_LIGHTING,
    LUMP_OCCLUSION,
    LUMP_LEAFS,
    LUMP_FACEIDS,
    LUMP_EDGES,
    LUMP_SURFEDGES,
    LUMP_MODELS,
    LUMP_WORLDLIGHTS,
    LUMP_LEAFFACES,
    LUMP_LEAFBRUSHES,
    LUMP_BRUSHES,
    LUMP_BRUSHSIDES,
    LUMP_AREAS,
    LUMP_AREAPORTALS,
    LUMP_PORTALS            = 22,
    LUMP_UNUSED0            = 22,
    LUMP_PROPCOLLISION      = 22,
    LUMP_CLUSTERS           = 23,
    LUMP_UNUSED1            = 23,
    LUMP_PROPHULLS          = 23,
    LUMP_PORTALVERTS        = 24,
    LUMP_UNUSED2            = 24,
    LUMP_FAKEENTITIES       = 24,
    LUMP_PROPHULLVERTS      = 24,
    LUMP_CLUSTERPORTALS     = 25,
    LUMP_UNUSED3            = 25,
    LUMP_PROPTRIS           = 25,
    LUMP_DISPINFO,
    LUMP_ORIGINALFACES,
    LUMP_PHYSDISP,
    LUMP_PHYSCOLLIDE,
    LUMP_VERTNORMALS,
    LUMP_VERTNORMALINDICES,
    LUMP_DISP_LIGHTMAP_ALPHAS,
    LUMP_DISP_VERTS,
    LUMP_DISP_LIGHTMAP_SAMPLE_POSITIONS,
    LUMP_GAME_LUMP,
    LUMP_LEAFWATERDATA,
    LUMP_PRIMITIVES,
    LUMP_PRIMVERTS,
    LUMP_PRIMINDICES,
    LUMP_PAKFILE,
    LUMP_CLIPPORTALVERTS,
    LUMP_CUBEMAPS,
    LUMP_TEXDATA_STRING_DATA,
    LUMP_TEXDATA_STRING_TABLE,
    LUMP_OVERLAYS,
    LUMP_LEAFMINDISTTOWATER,
    LUMP_FACE_MACRO_TEXTURE_INFO,
    LUMP_DISP_TRIS,
    LUMP_PHYSCOLLIDESURFACE         = 49,
    LUMP_PROP_BLOB                  = 49,
    LUMP_WATEROVERLAYS,
    LUMP_LIGHTMAPPAGES              = 51,
    LUMP_LEAF_AMBIENT_INDEX_HDR     = 51,
    LUMP_LIGHTMAPPAGEINFOS          = 52,
    LUMP_LEAF_AMBIENT_INDEX         = 52,
    LUMP_LIGHTING_HDR,
    LUMP_WORLDLIGHTS_HDR,
    LUMP_LEAF_AMBIENT_LIGHTING_HDR,
    LUMP_LEAF_AMBIENT_LIGHTING,
    LUMP_XZIPPAKFILE,
    LUMP_FACES_HDR,
    LUMP_MAP_FLAGS,
    LUMP_OVERLAY_FADES,
    LUMP_OVERLAY_SYSTEM_LEVELS,
    LUMP_PHYSLEVEL,
    LUMP_DISP_MULTIBLEND
};

struct lump_t {
    uint32_t offset;
    uint32_t length;
    uint32_t version;
    char fourCC[4];
};

struct dheader_t {
    uint32_t    ident;
    uint32_t    version;
    lump_t      lumps[N_BSP_LUMPS];
    uint32_t    mapRevision;
    /*
    std::vector<gfxm::vec3> vertices;
    std::vector<texinfo_t> texinfos;
    std::vector<dtexdata_t> texdata;
    std::vector<uint32_t> texdata_string_table;
    std::vector<char> texdata_string_data;
    std::vector<dedge_t> edges;
    std::vector<int32_t> surfedges;
    std::vector<dface_t> faces;
    std::vector<ddispinfo_t> displacements;
    std::vector<dDispVert> disp_verts;
    std::vector<gfxm::vec3> lm_samples;
    std::vector<dispsamplepos_t> lm_disp_sample_positions;
    */

    template<LUMP_TYPE TYPE, typename T>
    uint32_t count() const {
        return lumps[TYPE].length / sizeof(T);
    }
    template<LUMP_TYPE TYPE, typename T>
    const T* get() const {
        return (T*)((uint8_t*)this + lumps[TYPE].offset);
    }
    template<LUMP_TYPE TYPE, typename T>
    const T& get(int i) const {
        return get()[i];
    }
};

struct texinfo_t {
    float textureVecs[2][4];
    float lightmapVecs[2][4];
    uint32_t flags;
    int32_t texdata;
};

struct dtexdata_t
{
    gfxm::vec3  reflectivity;            // RGB reflectivity
    int         nameStringTableID;       // index into TexdataStringTable
    int         width, height;           // source image
    int         view_width, view_height;
};

struct dedge_t {
    uint16_t v[2];
};

struct dface_t {
    uint16_t    planenum;               // the plane number
    int8_t      side;                   // faces opposite to the node's plane direction
    int8_t      onNode;                 // 1 of on node, 0 if in leaf
    int32_t     firstedge;              // index into surfedges
    int16_t     numedges;               // number of surfedges
    int16_t     texinfo;                // texture info
    int16_t     dispinfo;               // displacement info
    int16_t     surfaceFogVolumeID;     // ?
    int8_t      styles[4];              // switchable lighting info
    int32_t     lightofs;               // offset into lightmap lump
    float       area;                   // face area in units^2
    int32_t     LightmapTextureMinsInLuxels[2]; // texture lighting info
    int32_t     LightmapTextureSizeInLuxels[2]; // texture lighting info
    uint32_t    origFace;               // original face this was split from
    uint16_t    numPrims;               // primitives
    uint16_t    firstPrimID;
    uint32_t    smoothingGroups;        // lightmap smoothing group
};

struct CDispSubNeighbor {
    unsigned short m_iNeighbor; // This indexes into ddispinfos.
    // 0xFFFF if there is no neighbor here.

    unsigned char m_NeighborOrientation; // (CCW) rotation of the neighbor wrt this displacement.

    // These use the NeighborSpan type.
    unsigned char m_Span;         // Where the neighbor fits onto this side of our displacement.
    unsigned char m_NeighborSpan; // Where we fit onto our neighbor.
};

struct CDispNeighbor {
    CDispSubNeighbor m_SubNeighbors[2];
};

constexpr int MAX_DISP_CORNER_NEIGHBORS = 4;

struct CDispCornerNeighbors {
    unsigned short m_Neighbors[MAX_DISP_CORNER_NEIGHBORS]; // indices of neighbors.
    unsigned char m_nNeighbors;
};

struct ddispinfo_t {
    gfxm::vec3            startPosition;                // start position used for orientation
    int32_t               DispVertStart;                // Index into LUMP_DISP_VERTS.
    int32_t               DispTriStart;                 // Index into LUMP_DISP_TRIS.
    int32_t               power;                        // power - indicates size of surface (2^power 1)
    int32_t               minTess;                      // minimum tesselation allowed
    float                 smoothingAngle;               // lighting smoothing angle
    int32_t               contents;                     // surface contents
    uint16_t              MapFace;                      // Which map face this displacement comes from.
    int32_t               LightmapAlphaStart;           // Index into ddisplightmapalpha.
    int32_t               LightmapSamplePositionStart;  // Index into LUMP_DISP_LIGHTMAP_SAMPLE_POSITIONS.
    CDispNeighbor         EdgeNeighbors[4];             // Indexed by NEIGHBOREDGE_ defines.
    CDispCornerNeighbors  CornerNeighbors[4];           // Indexed by CORNER_ defines.
    uint32_t              AllowedVerts[10];             // active verticies
};

struct dDispVert {
    gfxm::vec3  vec;    // Vector field defining displacement volume.
    float       dist;   // Displacement distances.
    float       alpha;  // "per vertex" alpha values.
};

#pragma pack(push, 1)
struct dispsamplepos_t {
    uint16_t index; // ?
    uint8_t x; // barycentric coordinates
    uint8_t y;
    uint8_t z;
};
#pragma pack(pop)

struct lmsample_t {
    uint8_t r, g, b;
    int8_t exp;
};
static_assert(sizeof(lmsample_t) == 4);

struct dgamelump_t {
    int32_t     id;        // gamelump ID
    uint16_t    flags;     // flags
    uint16_t    version;   // gamelump version
    int32_t     fileofs;   // offset to this gamelump
    int32_t     filelen;   // length
};

struct StaticPropLump_t {
	// v4
	gfxm::vec3          Origin;            // origin
	gfxm::vec3          Angles;            // orientation (pitch yaw roll)
	
	// v4
	uint16_t  PropType;          // index into model name dictionary
    uint16_t  FirstLeaf;         // index into leaf array
    uint16_t  LeafCount;
	uint8_t   Solid;             // solidity type
	// every version except v7*
    uint8_t   Flags0;
	// v4 still
	int32_t             Skin;              // model skin numbers
	float           FadeMinDist;
	float           FadeMaxDist;
	gfxm::vec3          LightingOrigin;    // for lighting
	// since v5
	float           ForcedFadeScale;   // fade distance scale
	// v6, v7, and v7* only
    uint16_t  MinDXLevel;        // minimum DirectX version to be visible
    uint16_t  MaxDXLevel;        // maximum DirectX version to be visible
    /*
	// v7* only
    uint32_t    Flags1;
    uint16_t  LightmapResX;      // lightmap image width
    uint16_t	LightmapResY;      // lightmap image height
	// since v8
	uint8_t   MinCPULevel;
    uint8_t   MaxCPULevel;
    uint8_t   MinGPULevel;
    uint8_t   MaxGPULevel;
	// since v7
	uint32_t         DiffuseModulation; // per instance color and alpha modulation
	// v9 and v10 only
	uint32_t            DisableX360;       // if true, don't show on XBox 360 (4-bytes long)
	// since v10
	uint32_t    FlagsEx;           // Further bitflags.
	// since v11
	float           UniformScale;      // Prop scale
    */
};

static_assert(sizeof(dheader_t) == 1036);
static_assert(sizeof(lump_t) == 16);
static_assert(sizeof(texinfo_t) == 72);
static_assert(sizeof(dedge_t) == 4);
static_assert(sizeof(dface_t) == 56);
static_assert(sizeof(ddispinfo_t) == 176);
static_assert(sizeof(dDispVert) == 20);
static_assert(sizeof(dgamelump_t) == 16);
static_assert(sizeof(StaticPropLump_t) == 64);

template<LUMP_TYPE LTYPE, typename T>
struct BspLumpView {
    const dheader_t* head = 0;

    BspLumpView() {}
    BspLumpView(const dheader_t* head)
        : head(head) {}

    uint64_t size() const {
        return head->lumps[LTYPE].length / sizeof(T);
    }
    const T* data() const {
        return (T*)((uint8_t*)head + head->lumps[LTYPE].offset);
    }
    const T& operator[](int i) const {
        return data()[i];
    }
};

struct BspFile {
    const dheader_t* head = 0;
    std::vector<uint8_t> file_data;
    
    BspLumpView<LUMP_VERTEXES, gfxm::vec3> vertices;
    BspLumpView<LUMP_TEXINFO, texinfo_t> texinfos;
    BspLumpView<LUMP_TEXDATA, dtexdata_t> texdata;
    BspLumpView<LUMP_TEXDATA_STRING_TABLE, uint32_t> texdata_string_table;
    BspLumpView<LUMP_TEXDATA_STRING_DATA, char> texdata_string_data;
    BspLumpView<LUMP_EDGES, dedge_t> edges;
    BspLumpView<LUMP_SURFEDGES, int32_t> surfedges;
    BspLumpView<LUMP_FACES, dface_t> faces;
    BspLumpView<LUMP_DISPINFO, ddispinfo_t> displacements;
    BspLumpView<LUMP_DISP_VERTS, dDispVert> disp_verts;
    BspLumpView<LUMP_LIGHTING, lmsample_t> lm_samples_raw;
    BspLumpView<LUMP_DISP_LIGHTMAP_SAMPLE_POSITIONS, dispsamplepos_t> lm_disp_sample_positions;
    BspLumpView<LUMP_ENTITIES, char> entities;

    std::vector<gfxm::vec3> lm_samples;

    void init() {
        vertices.head                   = head;
        texinfos.head                   = head;
        texdata.head                    = head;
        texdata_string_table.head       = head;
        texdata_string_data.head        = head;
        edges.head                      = head;
        surfedges.head                  = head;
        faces.head                      = head;
        displacements.head              = head;
        disp_verts.head                 = head;
        lm_samples_raw.head             = head;
        lm_disp_sample_positions.head   = head;
        entities.head                   = head;
        
        lm_samples.resize(lm_samples_raw.size());
        for (int i = 0; i < lm_samples.size(); ++i) {
            gfxm::vec3& lmf = lm_samples[i];
            const lmsample_t& lm = lm_samples_raw[i];
            int exp = lm.exp;
            float R = (lm.r / 255.f) * powf(2, exp);
            float G = (lm.g / 255.f) * powf(2, exp);
            float B = (lm.b / 255.f) * powf(2, exp);
            lmf = gfxm::vec3(R, G, B);
        }
    }

    /*
    std::vector<gfxm::vec3> vertices;
    std::vector<texinfo_t> texinfos;
    std::vector<dtexdata_t> texdata;
    std::vector<uint32_t> texdata_string_table;
    std::vector<char> texdata_string_data;
    std::vector<dedge_t> edges;
    std::vector<int32_t> surfedges;
    std::vector<dface_t> faces;
    std::vector<ddispinfo_t> displacements;
    std::vector<dDispVert> disp_verts;
    std::vector<gfxm::vec3> lm_samples;
    std::vector<dispsamplepos_t> lm_disp_sample_positions;
    */
    // Generated
    std::vector<gfxm::vec2> lm_uv_offsets;
    std::vector<gfxm::vec2> lm_uv_scales;
};

#pragma pack(push, 1)
struct color24 {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    color24() : r(0), g(0), b(0) {}
    color24(uint32_t c32) {
        r = c32 & 0x000000FF;
        g = (c32 & 0x0000FF00) >> 8;
        b = (c32 & 0x00FF0000) >> 16;
    }
};
#pragma pack(pop)

struct MeshData {
    std::vector<gfxm::vec3> vertices;
    std::vector<color24>    colors;
    std::vector<gfxm::vec2> uvs;
    std::vector<gfxm::vec2> uvs_lightmap;
    std::vector<gfxm::vec3> normals;
    std::vector<gfxm::vec3> tangents;
    std::vector<gfxm::vec3> binormals;
    std::vector<uint32_t>   indices;
};


void makeTanBinorm(
    const gfxm::vec3* vertices, const gfxm::vec3* normals,
    const gfxm::vec2* uvs, int vertex_count,
    const uint32_t* indices, int index_count,
    std::vector<gfxm::vec3>& out_tan,
    std::vector<gfxm::vec3>& out_bitan
) {
    assert(index_count % 3 == 0);
    out_tan.resize(vertex_count);
    out_bitan.resize(vertex_count);
    for (int i = 0; i < index_count; i += 3) {
        uint32_t a = indices[i];
        uint32_t b = indices[i + 1];
        uint32_t c = indices[i + 2];

        gfxm::vec3 Va = vertices[a];
        gfxm::vec3 Vb = vertices[b];
        gfxm::vec3 Vc = vertices[c];
        gfxm::vec3 Na = normals[a];
        gfxm::vec3 Nb = normals[b];
        gfxm::vec3 Nc = normals[c];
        gfxm::vec2 UVa = uvs[a];
        gfxm::vec2 UVb = uvs[b];
        gfxm::vec2 UVc = uvs[c];

        float x1 = Vb.x - Va.x;
        float x2 = Vc.x - Va.x;
        float y1 = Vb.y - Va.y;
        float y2 = Vc.y - Va.y;
        float z1 = Vb.z - Va.z;
        float z2 = Vc.z - Va.z;

        float s1 = UVb.x - UVa.x;
        float s2 = UVc.x - UVa.x;
        float t1 = UVb.y - UVa.y;
        float t2 = UVc.y - UVa.y;

        float r = 1.f / (s1 * t2 - s2 * t1);

        gfxm::vec3 sdir(
            (t2 * x1 - t1 * x2) * r,
            (t2 * y1 - t1 * y2) * r,
            (t2 * z1 - t1 * z2) * r
        );
        gfxm::vec3 tdir(
            (s1 * x2 - s2 * x1) * r,
            (s1 * y2 - s2 * y1) * r,
            (s1 * z2 - s2 * z1) * r
        );

        if(sdir.is_valid() && tdir.is_valid()) {
            out_tan[a] += sdir;
            out_tan[b] += sdir;
            out_tan[c] += sdir;
            out_bitan[a] += tdir;
            out_bitan[b] += tdir;
            out_bitan[c] += tdir;
        }
    }
    for (int i = 0; i < vertex_count; ++i) {
        out_tan[i] = gfxm::normalize(out_tan[i]);
        out_bitan[i] = gfxm::normalize(out_bitan[i]);
    }
}

static gfxm::vec3 getVertex(const BspFile& bspf, const dface_t& face, int vidx) {
    auto surfedges_begin = &bspf.surfedges[face.firstedge];
    int32_t surfedge = surfedges_begin[vidx];
    uint32_t edge[2];
    if (surfedge < 0) {
        edge[0] = bspf.edges[-surfedge].v[1];
        edge[1] = bspf.edges[-surfedge].v[0];
    } else {
        edge[0] = bspf.edges[surfedge].v[0];
        edge[1] = bspf.edges[surfedge].v[1];
    }
    return bspf.vertices[edge[0]];
}

static void makeDisplacement(const BspFile& bspf, MeshData& mdata, int face_idx) {
    const auto& face = bspf.faces[face_idx];
    const auto& texinfo = bspf.texinfos[face.texinfo];
    assert(face.dispinfo >= 0);
    assert(face.numedges == 4);

    gfxm::vec2 lm_uv_offset = bspf.lm_uv_offsets[face_idx];
    gfxm::vec2 lm_uv_scale = bspf.lm_uv_scales[face_idx];

    gfxm::vec2 texture_size_factor(1.f / 512.f, 1.f / 512.f);
    if (texinfo.texdata >= 0) {
        int tw = bspf.texdata[texinfo.texdata].width;
        int th = bspf.texdata[texinfo.texdata].height;
        texture_size_factor = gfxm::vec2(1.f / float(tw), 1.f / float(th));
    }
    gfxm::vec2 lm_size_factor(
        1.f / float(face.LightmapTextureSizeInLuxels[0] + 1),
        1.f / float(face.LightmapTextureSizeInLuxels[1] + 1)
    );

    const gfxm::vec4 tvs = gfxm::vec4(
        texinfo.textureVecs[0][0], texinfo.textureVecs[0][1],
        texinfo.textureVecs[0][2], texinfo.textureVecs[0][3]
    );
    const gfxm::vec4 tvt = gfxm::vec4(
        texinfo.textureVecs[1][0], texinfo.textureVecs[1][1],
        texinfo.textureVecs[1][2], texinfo.textureVecs[1][3]
    );
    const gfxm::vec4 tvs_lm = gfxm::vec4(
        texinfo.lightmapVecs[0][0], texinfo.lightmapVecs[0][1],
        texinfo.lightmapVecs[0][2], texinfo.lightmapVecs[0][3]
    );
    const gfxm::vec4 tvt_lm = gfxm::vec4(
        texinfo.lightmapVecs[1][0], texinfo.lightmapVecs[1][1],
        texinfo.lightmapVecs[1][2], texinfo.lightmapVecs[1][3]
    );

    const ddispinfo_t& disp = bspf.displacements[face.dispinfo];
    const dDispVert* disp_verts = &bspf.disp_verts[disp.DispVertStart];
    const gfxm::vec3& start_pos = disp.startPosition;

    gfxm::vec3 pts[4] = {
        getVertex(bspf, face, 0),
        getVertex(bspf, face, 3),
        getVertex(bspf, face, 2),
        getVertex(bspf, face, 1)
    };
    gfxm::vec2 pts_uvs[4] = {
        gfxm::vec2(0, 0),
        gfxm::vec2(1, 0),
        gfxm::vec2(1, 1),
        gfxm::vec2(0, 1)
    };
    gfxm::vec2 pts_uvs_lm[4] = {
        gfxm::vec2(0, 0),
        gfxm::vec2(1, 0),
        gfxm::vec2(1, 1),
        gfxm::vec2(0, 1)
    };

    int start_idx = 0;
    float min_dist = INFINITY;
    for (int i = 0; i < 4; ++i) {
        float dist = gfxm::length(start_pos - pts[i]);
        if (min_dist > dist) {
            start_idx = i;
            min_dist = dist;
        }
    }

    for (int i = 0; i < 4; ++i) {
        //int pidx = (start_idx + 3 + i) % 4;
        const gfxm::vec3& P = pts[i];
        gfxm::vec2& UV = pts_uvs[i];
        gfxm::vec2& UVlm = pts_uvs_lm[i];
        
        UV = gfxm::vec2(
            tvs.x * P.x + tvs.y * P.y + tvs.z * P.z + tvs.w,
            tvt.x * P.x + tvt.y * P.y + tvt.z * P.z + tvt.w
        );
        UV *= texture_size_factor;
        /*
        UVlm = gfxm::vec2(
            tvs_lm.x * P.x + tvs_lm.y * P.y + tvs_lm.z * P.z + tvs_lm.w,
            tvt_lm.x * P.x + tvt_lm.y * P.y + tvt_lm.z * P.z + tvt_lm.w
        );
        UVlm -= gfxm::vec2(
            face.LightmapTextureMinsInLuxels[0],
            face.LightmapTextureMinsInLuxels[1]
        );
        UVlm *= lm_size_factor;*/
        UVlm *= lm_uv_scale;
        UVlm += lm_uv_offset;
    }
    /*
    const gfxm::vec3 A = pts[start_idx];
    const gfxm::vec3 B = pts[(start_idx + 3) % 4];
    const gfxm::vec3 C = pts[(start_idx + 2) % 4];
    const gfxm::vec3 D = pts[(start_idx + 1) % 4];
    */
    gfxm::vec3 Nface;
    {
        Nface = -gfxm::normalize(gfxm::cross(pts[1] - pts[0], pts[2] - pts[1]));
        if (Nface.length() <= FLT_EPSILON) {
            assert(false);
        }
    }

    assert(disp.power == 2 || disp.power == 3 || disp.power == 4);
    const int size = (1 << disp.power) + 1;

    const int width = size;
    const int height = size;
    const int vcount = width * height;
    std::vector<gfxm::vec3> vertices(vcount);
    std::vector<gfxm::vec2> uvs(vcount);
    std::vector<gfxm::vec2> uvs_lm(vcount);
    std::vector<gfxm::vec3> normals(vcount);
    std::vector<color24>    colors(vcount);

    const int i0 = start_idx;
    const int i1 = (start_idx + 1) % 4;
    const int i2 = (start_idx + 2) % 4;
    const int i3 = (start_idx + 3) % 4;

    for (int iy = 0; iy < height; ++iy) {
        const float fy = iy / float(height - 1);
        for (int ix = 0; ix < width; ++ix) {
            const float fx = ix / float(width - 1);

            const int ivert = (ix) + (iy) * width;
            const dDispVert& disp_vert = disp_verts[ivert];

            assert(disp_vert.dist == disp_vert.dist);
            gfxm::vec3 disp = disp_vert.vec * disp_vert.dist;

            gfxm::vec3 P =
                gfxm::lerp(
                    gfxm::lerp(pts[i0], pts[i1], fx),
                    gfxm::lerp(pts[i3], pts[i2], fx),
                    fy
                ) + disp;
            /*
            gfxm::vec2 UV = gfxm::vec2(
                tvs.x * P.x + tvs.y * P.y + tvs.z * P.z + tvs.w,
                tvt.x * P.x + tvt.y * P.y + tvt.z * P.z + tvt.w
            );
            UV *= texture_size_factor;

            gfxm::vec2 UVlm = gfxm::vec2(
                tvs_lm.x * P.x + tvs_lm.y * P.y + tvs_lm.z * P.z + tvs_lm.w,
                tvt_lm.x * P.x + tvt_lm.y * P.y + tvt_lm.z * P.z + tvt_lm.w
            );
            UVlm -= gfxm::vec2(
                face.LightmapTextureMinsInLuxels[0],
                face.LightmapTextureMinsInLuxels[1]
            );
            UVlm *= lm_size_factor;
            UVlm *= lm_uv_scale;
            UVlm += lm_uv_offset;
            */
            gfxm::vec2 UV =
                gfxm::lerp(
                    gfxm::lerp(pts_uvs[i0], pts_uvs[i1], fx),
                    gfxm::lerp(pts_uvs[i3], pts_uvs[i2], fx),
                    fy
                );
            gfxm::vec2 UVlm =
                gfxm::lerp(
                    gfxm::lerp(pts_uvs_lm[0], pts_uvs_lm[1], fx),
                    gfxm::lerp(pts_uvs_lm[3], pts_uvs_lm[2], fx),
                    fy
                );

            vertices[ix + iy * width] = P;
            uvs[ix + iy * width] = UV;
            uvs_lm[ix + iy * width] = UVlm;

            // TODO: Color from lightmap data?
            //colors[ix + iy * width] = gfxm::make_rgba32(fx, fy, .0f, 1.f);
            colors[ix + iy * width] = 0xFFFFFF;
            //colors[ix + iy * width] = 0x0000FF;
        }
    }

    std::vector<uint32_t> indices;//((width / 3) * (height / 3) * 24);
    const uint32_t base_index = mdata.vertices.size();
    for (int iy = 0; iy < height - 1; iy += 2) {
        for (int ix = 0; ix < width - 1; ix += 2) {
            const uint32_t ibase = base_index + ix + iy * width;
            const uint32_t i00 = ibase;
            const uint32_t i10 = ibase + 1;
            const uint32_t i20 = ibase + 2;
            const uint32_t i01 = i00 + width;
            const uint32_t i11 = i10 + width;
            const uint32_t i21 = i20 + width;
            const uint32_t i02 = i01 + width;
            const uint32_t i12 = i11 + width;
            const uint32_t i22 = i21 + width;
            /*
            uint32_t tindices[24] = {
                i01, i00, i11, i11, i00, i10,
                i10, i20, i11, i11, i20, i21,
                i21, i22, i11, i11, i22, i12,
                i12, i02, i11, i11, i02, i01
            };*/
            uint32_t tindices[24] = {
                i01, i11, i00, i11, i10, i00,
                i10, i11, i20, i11, i21, i20,
                i21, i11, i22, i11, i12, i22,
                i12, i11, i02, i11, i01, i02
            };

            indices.insert(indices.end(), tindices, tindices + 24);
        }
    }

    for (int j = 0; j < indices.size(); j += 3) {
        gfxm::vec3 N;
        const gfxm::vec3 A = vertices[indices[j] - base_index];
        const gfxm::vec3 B = vertices[indices[j + 1] - base_index];
        const gfxm::vec3 C = vertices[indices[j + 2] - base_index];
        N = -gfxm::normalize(gfxm::cross(B - A, C - B));
        if (N.length() <= FLT_EPSILON) {
            //assert(false);
        }
        normals[indices[j] - base_index] += N;
        normals[indices[j + 1] - base_index] += N;
        normals[indices[j + 2] - base_index] += N;
    }
    for (int j = 0; j < normals.size(); ++j) {
        normals[j] = gfxm::normalize(normals[j]);
    }

    mdata.vertices.insert(mdata.vertices.end(), vertices.begin(), vertices.end());
    mdata.uvs.insert(mdata.uvs.end(), uvs.begin(), uvs.end());
    mdata.uvs_lightmap.insert(mdata.uvs_lightmap.end(), uvs_lm.begin(), uvs_lm.end());
    mdata.normals.insert(mdata.normals.end(), normals.begin(), normals.end());
    mdata.colors.insert(mdata.colors.end(), colors.begin(), colors.end());
    mdata.indices.insert(mdata.indices.end(), indices.begin(), indices.end());
}

static void makeFace(const BspFile& bspf, MeshData& mdata, int face_idx) {
    const auto& face = bspf.faces[face_idx];
    const auto& texinfo = bspf.texinfos[face.texinfo];
    const uint32_t base_index = mdata.vertices.size();

    gfxm::vec2 lm_uv_offset = bspf.lm_uv_offsets[face_idx];
    gfxm::vec2 lm_uv_scale = bspf.lm_uv_scales[face_idx];

    gfxm::vec2 texture_size_factor(1.f / 512.f, 1.f / 512.f);
    if (texinfo.texdata >= 0) {
        int tw = bspf.texdata[texinfo.texdata].width;
        int th = bspf.texdata[texinfo.texdata].height;
        texture_size_factor = gfxm::vec2(1.f / float(tw), 1.f / float(th));
    }
    gfxm::vec2 lm_size_factor(
        1.f / float(face.LightmapTextureSizeInLuxels[0] + 1),
        1.f / float(face.LightmapTextureSizeInLuxels[1] + 1)
    );

    const gfxm::vec4 tvs = gfxm::vec4(
        texinfo.textureVecs[0][0],
        texinfo.textureVecs[0][1],
        texinfo.textureVecs[0][2],
        texinfo.textureVecs[0][3]
    );
    const gfxm::vec4 tvt = gfxm::vec4(
        texinfo.textureVecs[1][0],
        texinfo.textureVecs[1][1],
        texinfo.textureVecs[1][2],
        texinfo.textureVecs[1][3]
    );
    const gfxm::vec4 tvs_lm = gfxm::vec4(
        texinfo.lightmapVecs[0][0],
        texinfo.lightmapVecs[0][1],
        texinfo.lightmapVecs[0][2],
        texinfo.lightmapVecs[0][3]
    );
    const gfxm::vec4 tvt_lm = gfxm::vec4(
        texinfo.lightmapVecs[1][0],
        texinfo.lightmapVecs[1][1],
        texinfo.lightmapVecs[1][2],
        texinfo.lightmapVecs[1][3]
    );

    std::vector<uint32_t> face_indices;
    std::vector<uint32_t> new_face_indices;

    auto surfedges_begin = &bspf.surfedges[face.firstedge];
    for (int j = 0; j < face.numedges; ++j) {
        int32_t surfedge = surfedges_begin[j];
        uint32_t edge[2];
        if (surfedge < 0) {
            edge[0] = bspf.edges[-surfedge].v[1];
            edge[1] = bspf.edges[-surfedge].v[0];
        } else {
            edge[0] = bspf.edges[surfedge].v[0];
            edge[1] = bspf.edges[surfedge].v[1];
        }

        gfxm::vec3 v = bspf.vertices[edge[0]];

        mdata.vertices.push_back(v);
        face_indices.push_back(edge[0]);
        new_face_indices.push_back(base_index + j);

        gfxm::vec2 uv = gfxm::vec2(
            tvs.x * v.x + tvs.y * v.y + tvs.z * v.z + tvs.w,
            tvt.x * v.x + tvt.y * v.y + tvt.z * v.z + tvt.w
        );
        mdata.uvs.push_back(uv * texture_size_factor);

        gfxm::vec2 uv_lm = gfxm::vec2(
            tvs_lm.x * v.x + tvs_lm.y * v.y + tvs_lm.z * v.z + tvs_lm.w,
            tvt_lm.x * v.x + tvt_lm.y * v.y + tvt_lm.z * v.z + tvt_lm.w
        );
        uv_lm -= gfxm::vec2(
            face.LightmapTextureMinsInLuxels[0],
            face.LightmapTextureMinsInLuxels[1]
        );
        uv_lm *= lm_size_factor;
        uv_lm *= lm_uv_scale;
        uv_lm += lm_uv_offset;
        mdata.uvs_lightmap.push_back(uv_lm);

        mdata.colors.push_back(0xFFFFFF);
    }

    gfxm::vec3 N = gfxm::vec3(0,0,0);
    int j = 0;
    while(face_indices.size() > 2) {
        if (j >= face_indices.size()) {
            j = 0;
        }
                
        uint32_t ia = j;
        uint32_t ib = (j + 1) % face_indices.size();
        uint32_t ic = (j + 2) % face_indices.size();

        uint32_t a = face_indices[ia];
        uint32_t b = face_indices[ib];
        uint32_t c = face_indices[ic];

        gfxm::vec3 A = bspf.vertices[a];
        gfxm::vec3 B = bspf.vertices[b];
        gfxm::vec3 C = bspf.vertices[c];

        float d = gfxm::dot(gfxm::normalize(B - A), gfxm::normalize(C - B));
        if (d >= 1.0f - FLT_EPSILON) {
            ++j;
            continue;
        }

        mdata.indices.push_back(new_face_indices[ia]);
        mdata.indices.push_back(new_face_indices[ib]);
        mdata.indices.push_back(new_face_indices[ic]);

        gfxm::vec3 tmpN = -gfxm::normalize(gfxm::cross(B - A, C - A));
        if(tmpN.length() >= .5f) {
            N = tmpN;
        }

        face_indices.erase(face_indices.begin() + ib);
        new_face_indices.erase(new_face_indices.begin() + ib);

        ++j;              
    }

    //assert(N.length() > .0f);

    for (int j = 0; j < face.numedges; ++j) {
        mdata.normals.push_back(N);
    }
}

static void convertVertices(MeshData& mdata) {
    for (int i = 0; i < mdata.vertices.size(); ++i) {
        gfxm::mat4 transform = gfxm::to_mat4(gfxm::angle_axis(gfxm::radian(-90.f), gfxm::vec3(1, 0, 0)));
        mdata.vertices[i] = transform * gfxm::vec4(mdata.vertices[i], 1.f);
        //float tmp = mdata.vertices[i].y;
        //mdata.vertices[i].y = mdata.vertices[i].z;
        //mdata.vertices[i].z = -tmp;
        /*
        float scale = 1.f / 200.f;
        vertices_triangulated[i] *= scale;
        */
        //float scale = .01905f;
        float scale = 1.f / 41.f;
        mdata.vertices[i] *= scale;
    }
    for (int i = 0; i < mdata.normals.size(); ++i) {
        gfxm::mat4 transform = gfxm::to_mat4(gfxm::angle_axis(gfxm::radian(-90.f), gfxm::vec3(1, 0, 0)));
        mdata.normals[i] = transform * gfxm::vec4(mdata.normals[i], 0.f);
        //float tmp = mdata.normals[i].y;
        //mdata.normals[i].y = mdata.normals[i].z;
        //mdata.normals[i].z = -tmp;
    }
    for (int i = 0; i < mdata.tangents.size(); ++i) {
        gfxm::mat4 transform = gfxm::to_mat4(gfxm::angle_axis(gfxm::radian(-90.f), gfxm::vec3(1, 0, 0)));
        mdata.tangents[i] = transform * gfxm::vec4(mdata.tangents[i], 0.f);
        //float tmp = mdata.tangents[i].y;
        //mdata.tangents[i].y = mdata.tangents[i].z;
        //mdata.tangents[i].z = -tmp;
    }
    for (int i = 0; i < mdata.binormals.size(); ++i) {
        gfxm::mat4 transform = gfxm::to_mat4(gfxm::angle_axis(gfxm::radian(-90.f), gfxm::vec3(1, 0, 0)));
        mdata.binormals[i] = transform * gfxm::vec4(mdata.binormals[i], 0.f);
        //float tmp = mdata.binormals[i].y;
        //mdata.binormals[i].y = mdata.binormals[i].z;
        //mdata.binormals[i].z = -tmp;
    }
    for (int i = 0; i < mdata.indices.size(); i += 3) {
        uint32_t tmp = mdata.indices[i + 2];
        mdata.indices[i + 2] = mdata.indices[i + 1];
        mdata.indices[i + 1] = tmp;
    }
}

struct ZIP_EOCD {
    uint32_t signature;
    uint16_t disk_num;
    uint16_t central_dir_start_disk;
    uint16_t idk;
    uint16_t central_dir_rec_num;
    uint32_t central_dir_size;
    uint32_t central_dir_offset;
};
static_assert(sizeof(ZIP_EOCD) == 20);
#pragma pack(push, 1)
struct ZIP_CDIR_RECORD {
    uint32_t signature;
    uint16_t version;
    uint16_t min_ver;
    uint16_t flags;
    uint16_t compression;
    uint16_t last_modified_time;    // File last modification time (MS-DOS format)
    uint16_t last_modified_date;    // File last modification date (MS-DOS format)
    uint32_t crc32;                 // CRC-32 of uncompressed data
    uint32_t comp_size;             // Compressed size
    uint32_t uncomp_size;           // Uncompressed size
    uint16_t name_len;              // File name length (n)
    uint16_t extra_len;             // Extra field length (m)
    uint16_t comment_len;           // File comment length (k)
    uint16_t disk_num;              // Disk number where file starts
    uint16_t int_attribs;           // Internal file attributes
    uint32_t ext_attribs;           // External file attributes
    uint32_t lcl_file_header_offset;// Offset of local file header (from start of disk)
};
#pragma pack(pop)
static_assert(sizeof(ZIP_CDIR_RECORD) == 46);
#pragma pack(push, 1)
struct ZIP_FILE_HEADER {
    uint32_t signature;
    uint16_t min_ver;
    uint16_t flags;
    uint16_t compression;
    uint16_t last_modified_time;    // File last modification time (MS-DOS format)
    uint16_t last_modified_date;    // File last modification date (MS-DOS format)
    uint32_t crc32;                 // CRC-32 of uncompressed data
    uint32_t comp_size;             // Compressed size
    uint32_t uncomp_size;           // Uncompressed size
    uint16_t name_len;              // File name length (n)
    uint16_t extra_len;             // Extra field length (m)
};
#pragma pack(pop)
static_assert(sizeof(ZIP_FILE_HEADER) == 30);

bool hl2LoadBSP(const char* path, hl2BSPModel* scene) {
    std::filesystem::path fspath = path;
    LOG("Map file name: " << fspath.stem().string());
    std::string texture_file_prefix = "materials/maps/" + fspath.stem().string() + "/";

    bool is_big_endian = false;

    if (!path) {
        LOG_ERR("bsp file path is null");
        assert(false);
        return false;
    }
    
    FILE* f = fopen(path, "rb");
    if (!f) {
        LOG_ERR("Failed to open bsp file: " << path);
        assert(false);
        return false;
    }

    fseek(f, 0, SEEK_END);
    uint64_t fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    BspFile bspf = { 0 };
    bspf.file_data.resize(fsize);
    FREAD_MSG(bspf.file_data.data(), fsize, 1, f, "BSP file");
    bspf.head = (const dheader_t*)&bspf.file_data[0];
    /*
    fseek(f, 0, SEEK_SET);
    FREAD(&bspf.head, sizeof(bspf.head), 1, f);
    if (bspf.head.ident != IDBSPHEADER_LE && bspf.head.ident != IDBSPHEADER_BE) {
        LOG_ERR("bsp file header mismatch: " << path);
        ON_ERROR;
    }
    if (bspf.head.ident == IDBSPHEADER_BE) {
        is_big_endian = true;
    }
    */
    LOG("BSP VERSION: " << bspf.head->version);
    LOG("MAP REVISION: " << bspf.head->mapRevision);
    
    bspf.init();

    // Static props
    {
        auto lump = bspf.head->lumps[LUMP_GAME_LUMP];
        fseek(f, lump.offset, SEEK_SET);
        int32_t count = 0;
        FREAD_MSG(&count, sizeof(count), 1, f, "LUMP_GAME_LUMP count");
        std::unordered_map<int32_t, dgamelump_t> gamelumps;
        for (int i = 0; i < count; ++i) {
            dgamelump_t gamelump = { 0 };
            FREAD_MSG(&gamelump, sizeof(dgamelump_t), 1, f, "dgamelump_t");
            gamelumps[gamelump.id] = gamelump;
        }

        // Static props
        auto it = gamelumps.find(1936749168);
        if (it != gamelumps.end()) {
            LOG("Static prop lump VERSION: " << it->second.version);
            fseek(f, it->second.fileofs, SEEK_SET);
            
            // Load static prop model list
            int32_t entry_count = 0;
            FREAD_MSG(&entry_count, sizeof(entry_count), 1, f, "static prop count");

            LOG("Unique static prop model count: " << entry_count);
            std::vector<std::string> mdl_names(entry_count);
            for (int i = 0; i < entry_count; ++i) {
                char name[128] = { 0 };
                FREAD_MSG(name, 128, 1, f, "mdl name");
                LOG(name);
                mdl_names[i] = name;
            }
            for (int i = 0; i < entry_count; ++i) {
                const char* name = mdl_names[i].c_str();

                scene->models.push_back(std::unique_ptr<MDLModel>(new MDLModel));
                MDLModel* mdl = scene->models.back().get();
                if (!hl2LoadModel(name, mdl)) {
                    scene->models.back().reset();
                    continue;
                }

                for (int j = 0; j < mdl->meshes.size(); ++j) {
                    const auto& mesh = mdl->meshes[j];

                    const std::string matname = "materials/csg/default_floor.mat";
                    RHSHARED<gpuMaterial> material = resGet<gpuMaterial>(matname.c_str());
                    scene->static_prop_materials[matname] = material;
                }
            }

            // Skip leaf entries for now
            int32_t leaf_entry_count = 0;
            FREAD_MSG(&leaf_entry_count, sizeof(leaf_entry_count), 1, f, "leaf entry count");
            fseek(f, leaf_entry_count * sizeof(uint16_t), SEEK_CUR);

            int32_t static_prop_count = 0;
            FREAD(&static_prop_count, sizeof(static_prop_count), 1, f);
            for (int i = 0; i < static_prop_count; ++i) {
                StaticPropLump_t prop;
                FREAD_MSG(&prop, sizeof(prop), 1, f, "static prop");
                LOG("prop.PropType: " << prop.PropType);

                const auto& model = scene->models[prop.PropType];
                if (!model) {
                    continue;
                }

                float scale = 1.f / 41.f;

                gfxm::vec3 position = prop.Origin;
                float tmp = position.y;
                position.y = position.z;
                position.z = -tmp;
                position *= scale;

                gfxm::vec3 model_scale(1, 1, 1);
                gfxm::vec3 pyr = prop.Angles;

                gfxm::vec3 euler(pyr.z, pyr.x, pyr.y);
                euler = gfxm::radian(euler);

                gfxm::quat rotation_fix = gfxm::angle_axis(gfxm::radian(-90.f), gfxm::vec3(1, 0, 0));
                gfxm::quat qx = gfxm::angle_axis(euler.x, gfxm::vec3(1, 0, 0));
                gfxm::quat qy = gfxm::angle_axis(euler.y, gfxm::vec3(0, 0, -1));
                gfxm::quat qz = gfxm::angle_axis(euler.z, gfxm::vec3(0, 1, 0));
                gfxm::quat q = gfxm::normalize(qz * qy * qx);
                gfxm::mat4 transform =
                    gfxm::translate(gfxm::mat4(1.f), position) *
                    gfxm::to_mat4(q) *
                    gfxm::scale(gfxm::mat4(1.f), model_scale);

                for (int j = 0; j < model->meshes.size(); ++j) {
                    const auto& mesh = model->meshes[j];

                    gpuGeometryRenderable* renderable = new gpuGeometryRenderable(
                        mesh->material.get(),
                        &mesh->mesh_desc,
                        nullptr,
                        "static_prop"
                    );
                    renderable->setTransform(transform);
                    scene->renderables.push_back(std::unique_ptr<gpuGeometryRenderable>(renderable));
                }
            }

            // Load test model
            {
                const std::string name = "models/gunship.mdl";
                LOG(name);
                scene->models.push_back(std::unique_ptr<MDLModel>(new MDLModel));
                MDLModel* mdl = scene->models.back().get();
                hl2LoadModel(MKSTR(name).c_str(), mdl);

                for (int j = 0; j < mdl->meshes.size(); ++j) {
                    const auto& mesh = mdl->meshes[j];

                    gpuGeometryRenderable* renderable = new gpuGeometryRenderable(
                        mesh->material.get(),
                        &mesh->mesh_desc,
                        nullptr,
                        "mdl test"
                    );
                    gfxm::quat qbase = gfxm::angle_axis(
                        gfxm::radian(0),
                        gfxm::vec3(1, 0, 0)
                    );
                    gfxm::vec3 euler(0, 0, 0);
                    gfxm::quat qx =
                        gfxm::angle_axis(
                            gfxm::radian(euler.x),
                            gfxm::vec3(1, 0, 0)
                        );
                    gfxm::quat qy =
                        gfxm::angle_axis(
                            gfxm::radian(euler.y),
                            gfxm::to_mat3(qx) * gfxm::vec3(0, 1, 0)
                        );
                    gfxm::quat qz =
                        gfxm::angle_axis(
                            gfxm::radian(euler.z),
                            gfxm::to_mat3(qy * qx) * gfxm::vec3(0, 0, 1)
                        );
                    gfxm::quat q = qz * qy * qx;
                    renderable->setTransform(
                        gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(100.f, 50.f, 5.f))
                        * gfxm::to_mat4(q)
                    );
                    scene->renderables.push_back(std::unique_ptr<gpuGeometryRenderable>(renderable));
                }
            }
        }
    }

    // Entities
    if(bspf.entities.size() != 0) {
        valve_data list;
        if (valve::parse_entity_list(list, bspf.entities.data(), bspf.entities.size())) {
            assert(list.is_array());
            LOG("Entities: ");
            for (int i = 0; i < list.size(); ++i) {
                valve_data& e = list[i];
                assert(e.is_object());
                valve_data& cname = e["classname"];
                LOG("classname: " << cname.to_string());
            }
        }
    }

    if(1) {
        auto lump = bspf.head->lumps[LUMP_PAKFILE];
        const int MAX_COMMENT_LENGTH = 0xFFFF;
        const int EOCD_SIGNATURE = 0x06054B50;
        const int64_t SEARCH_SIZE = 22 + MAX_COMMENT_LENGTH;
        const int64_t SEARCH_OFFSET = std::max(int64_t(0), int64_t(lump.length) - (22 + MAX_COMMENT_LENGTH));
        fseek(f, lump.offset + SEARCH_OFFSET, SEEK_SET);
        std::vector<uint8_t> zip_tail(SEARCH_SIZE);
        FREAD_MSG(zip_tail.data(), SEARCH_SIZE, 1, f, "zip tail");
        uint64_t EOCD_at = 0;
        bool EOCD_found = false;
        for (int i = zip_tail.size() - 1; i >= 0; --i) {
            uint32_t sig = 0;
            sig += zip_tail[i] << 24;
            sig += zip_tail[i - 1] << 16;
            sig += zip_tail[i - 2] << 8;
            sig += zip_tail[i - 3];
            if (sig == EOCD_SIGNATURE) {
                EOCD_at = lump.offset + SEARCH_OFFSET + i - 3;
                EOCD_found = true;
                break;
            }
        }
        if (EOCD_found) {
            LOG("ZIP: Found EOCD at " << EOCD_at);
            fseek(f, EOCD_at, SEEK_SET);
            ZIP_EOCD eocd = { 0 };
            FREAD_MSG(&eocd, sizeof(eocd), 1, f, "eocd");
            LOG("EOCD signature: " << std::hex << "0x" << eocd.signature);
            if (eocd.signature != EOCD_SIGNATURE) {
                LOG_ERR("EOCD signature mismatch");
            }
            fseek(f, lump.offset + eocd.central_dir_offset, SEEK_SET);

            LOG("CDIR offset: " << eocd.central_dir_offset);
            LOG("Record count: " << eocd.central_dir_rec_num);

            std::vector<uint64_t> vmt_offsets;
            std::vector<uint64_t> vtf_offsets;
            vmt_offsets.reserve(eocd.central_dir_rec_num);
            vtf_offsets.reserve(eocd.central_dir_rec_num);
            for(int i = 0; i < eocd.central_dir_rec_num; ++i) {
                ZIP_CDIR_RECORD rec = { 0 };
                FREAD_MSG(&rec, sizeof(rec), 1, f, "ZIP_CDIR_RECORD");

                std::string fname;
                fname.resize(rec.name_len);
                FREAD_MSG(fname.data(), rec.name_len, 1, f, "zip fname");

                fseek(f, rec.extra_len + rec.comment_len, SEEK_CUR);

                if (std::string(".vtf") == &fname[fname.size() - 1 - 3]) {
                    vtf_offsets.push_back(rec.lcl_file_header_offset);
                } else if (std::string(".vmt") == &fname[fname.size() - 1 - 3]) {
                    vmt_offsets.push_back(rec.lcl_file_header_offset);
                }
            }

            for (int i = 0; i < vtf_offsets.size(); ++i) {
                fseek(f, lump.offset + vtf_offsets[i], SEEK_SET);
                ZIP_FILE_HEADER head = { 0 };
                FREAD_MSG(&head, sizeof(head), 1, f, "ZIP_FILE_HEADER");

                std::string fname;
                fname.resize(head.name_len);
                FREAD_MSG(fname.data(), head.name_len, 1, f, "zip file name");
                LOG(fname);

                fseek(f, head.extra_len, SEEK_CUR);
                /*
                assert(fname.size() > texture_file_prefix.size());
                fname.erase(fname.begin(), fname.begin() + texture_file_prefix.size());
                LOG("fname modified: " << fname);*/

                RHSHARED<gpuTexture2d> texture;
                if(hl2LoadTextureFromFile(f, texture)) {
                    hl2StoreTexture(MKSTR("experimental/hl2/" << fname).c_str(), texture);
                }
            }
            for (int i = 0; i < vmt_offsets.size(); ++i) {
                fseek(f, lump.offset + vmt_offsets[i], SEEK_SET);
                ZIP_FILE_HEADER head = { 0 };
                FREAD_MSG(&head, sizeof(head), 1, f, "ZIP_FILE_HEADER");

                std::string fname;
                fname.resize(head.name_len);
                FREAD_MSG(fname.data(), head.name_len, 1, f, "zip fname");
                LOG(fname);

                fseek(f, head.extra_len, SEEK_CUR);

                std::vector<uint8_t> bytes(head.comp_size);
                FREAD_MSG(bytes.data(), head.comp_size, 1, f, "zip file content");
                RHSHARED<gpuMaterial> material;
                LOG("Loading VMT from ZIP: " << fname);
                if (hl2LoadMaterialFromMemory(bytes.data(), bytes.size(), material, fname.c_str())) {
                    hl2StoreMaterial(MKSTR("experimental/hl2/" << fname).c_str(), material);
                } else {
                    LOG_ERR("Failed to load VMT from ZIP: " << fname);
                    LOG("\n" << std::string((const char*)bytes.data(), (const char*)bytes.data() + bytes.size()));
                }
            }
        } else {
            LOG_ERR("ZIP: EOCD not found");
        }
    }

    struct LM_INFO {
        uint32_t* data;
        uint32_t  width;
        uint32_t  height;
    };
    struct FACE_SET {
        std::vector<uint32_t> faces;
        MeshData mdata;
        std::string material_name;
        RHSHARED<gpuTexture2d> lm_texture;
    };

    std::vector<LM_INFO> face_lms(bspf.faces.size());
    std::unordered_map<uint32_t, FACE_SET> faces_per_material;
    {
        for (int i = 0; i < bspf.faces.size(); ++i) {
            const auto& face = bspf.faces[i];
            const auto& texinfo = bspf.texinfos[face.texinfo];

            if (texinfo.texdata < 0) {
                continue;
            }

            if (texinfo.flags & 0x0004 /* SURF_SKY */) {
                continue;
            }
            if (texinfo.flags & 0x0040 /* SURF_TRIGGER */) {
                continue;
            }
            if (texinfo.flags & 0x0080 /* SURF_NODRAW */) {
                continue;
            }
            if (texinfo.flags & 0x0200 /* SURF_SKIP */) {
                continue;
            }

            const auto& texdatum = bspf.texdata[texinfo.texdata];
            uint32_t tex_name_data_offs = bspf.texdata_string_table[texdatum.nameStringTableID];
            const char* texname = &bspf.texdata_string_data[tex_name_data_offs];
            faces_per_material[texdatum.nameStringTableID].faces.push_back(i);
            faces_per_material[texdatum.nameStringTableID].material_name = texname;
        }
    }

    // Build a single lm atlas for the entire map
    // and store new lm texture coordinates
    {
        std::vector<RectPack::Rect> rects(bspf.faces.size());
        bspf.lm_uv_offsets.resize(bspf.faces.size());
        bspf.lm_uv_scales.resize(bspf.faces.size());

        for (int i = 0; i < bspf.faces.size(); ++i) {
            const auto& face = bspf.faces[i];
            const auto& texinfo = bspf.texinfos[face.texinfo];

            rects[i].w = face.LightmapTextureSizeInLuxels[0] + 1;
            rects[i].h = face.LightmapTextureSizeInLuxels[1] + 1;
            rects[i].id = i;
        }

        RectPack packer;
        RectPack::Rect image_rect = packer.pack(
            rects.data(), rects.size(), 2 /* border */,
            RectPack::MAXSIDE, RectPack::POWER_OF_TWO
        );
        const int ATLAS_W = image_rect.w;
        const int ATLAS_H = image_rect.h;

        std::vector<gfxm::vec3> image(ATLAS_W * ATLAS_H);
        std::vector<uint8_t>    mask(ATLAS_W * ATLAS_H);
        // TODO: Fix lm seams
        std::fill(image.begin(), image.end(), gfxm::vec3(1,0,1));
        //std::fill(image.begin(), image.end(), gfxm::vec3(0,0,0));
        std::fill(mask.begin(), mask.end(), 0);
        for (int ir = 0; ir < rects.size(); ++ir) {
            const auto& rc = rects[ir];
            int faceidx = rc.id;
            gfxm::vec3* dst = &image[rc.x + rc.y * ATLAS_W];
            gfxm::vec3* src = &bspf.lm_samples[bspf.faces[faceidx].lightofs / 4];
            uint8_t* dst_mask = &mask[rc.x + rc.y * ATLAS_W];
            for (int iy = 0; iy < rc.h; ++iy) {
                for (int ix = 0; ix < rc.w; ++ix) {
                    dst[ix + iy * int(image_rect.w)] = src[ix + iy * int(rc.w)];
                    dst_mask[ix + iy * int(image_rect.w)] = 1;
                }
            }
        }
        for (int iy = 0; iy < ATLAS_H; ++iy) {
            for (int ix = 0; ix < ATLAS_W; ++ix) {
                if (mask[ix + iy * ATLAS_W]) {
                    continue;
                }
                gfxm::vec3 col;
                int sample_count = 0;
                for (int ky = -1; ky <= 1; ++ky) {
                    for (int kx = -1; kx <= 1; ++kx) {
                        int s = std::clamp(kx + ix, 0, ATLAS_W - 1);
                        int t = std::clamp(ky + iy, 0, ATLAS_H - 1);
                        col += image[s + t * ATLAS_W] * float(mask[s + t * ATLAS_W]);
                        sample_count += mask[s + t * ATLAS_W];
                    }
                }
                if (sample_count == 0) {
                    continue;
                }
                col /= float(sample_count);
                image[ix + iy * ATLAS_W] = col;
            }
        }
        /*
        for (int iy = 0; iy < ATLAS_H; ++iy) {
            for (int ix = 0; ix < ATLAS_W; ++ix) {
                float fx = ix / float(ATLAS_W);
                float fy = iy / float(ATLAS_H);
                image[ix + iy * ATLAS_W] = gfxm::vec3(fx, fy, .0f);
            }
        }*/
        RHSHARED<gpuTexture2d> lm_tex;
        lm_tex.reset_acquire();
        lm_tex->setData(image.data(), ATLAS_W, ATLAS_H, 3, IMAGE_CHANNEL_FLOAT);
        scene->lm_texture = lm_tex;
        //stbi_write_png(MKSTR("experimental/lm/lm_full.png").c_str(), image_rect.w, image_rect.h, 4, image.data(), 0);

        for (int ir = 0; ir < rects.size(); ++ir) {
            auto& rc = rects[ir];
            const auto& face = bspf.faces[rc.id];
            auto& texinfo = bspf.texinfos[face.texinfo];
            /*
            texinfo.lightmapVecs[0][0] *= rc.w / float(ATLAS_W);
            texinfo.lightmapVecs[0][1] *= rc.w / float(ATLAS_W);
            texinfo.lightmapVecs[0][2] *= rc.w / float(ATLAS_W);
            texinfo.lightmapVecs[0][3] += rc.x / float(ATLAS_W);
            texinfo.lightmapVecs[1][0] *= rc.h / float(ATLAS_H);
            texinfo.lightmapVecs[1][1] *= rc.h / float(ATLAS_H);
            texinfo.lightmapVecs[1][2] *= rc.h / float(ATLAS_H);
            texinfo.lightmapVecs[1][3] += rc.y / float(ATLAS_H);
            */
            bspf.lm_uv_offsets[rc.id] = 
                gfxm::vec2(
                    float(rc.x) / float(ATLAS_W),
                    float(rc.y) / float(ATLAS_H)
                );
            bspf.lm_uv_scales[rc.id] = 
                gfxm::vec2(
                    float(rc.w) / float(ATLAS_W),
                    float(rc.h) / float(ATLAS_H)
                );
        }
    }

    for (auto& kv : faces_per_material) {
        auto& face_set = kv.second;
        auto& faces = face_set.faces;

        for (int i = 0; i < faces.size(); ++i) {
            uint32_t iface = faces[i];
            const auto& face = bspf.faces[iface];
            const auto& texinfo = bspf.texinfos[face.texinfo];

            if (face.dispinfo >= 0) {
                assert(face.numedges == 4);
                makeDisplacement(bspf, face_set.mdata, iface);
            } else {
                makeFace(bspf, face_set.mdata, iface);
            }
        }
    }

    for (auto& kv : faces_per_material) {
        auto& face_set = kv.second;

        makeTanBinorm(
            &face_set.mdata.vertices[0],
            &face_set.mdata.normals[0],
            &face_set.mdata.uvs[0],
            face_set.mdata.vertices.size(),
            &face_set.mdata.indices[0],
            face_set.mdata.indices.size(),
            face_set.mdata.tangents,
            face_set.mdata.binormals
        );
    }

    for (auto& kv : faces_per_material) {
        auto& face_set = kv.second;
        convertVertices(face_set.mdata);
    }

    for (auto& kv : faces_per_material) {
        auto& face_set = kv.second;
        auto& mdata = face_set.mdata;

        scene->parts.push_back(std::unique_ptr<hl2BSPPart>(new hl2BSPPart));
        hl2BSPPart* part = scene->parts.back().get();
        auto& mesh = part->mesh;

        Mesh3d mesh3d;
        mesh3d.setAttribArray(VFMT::Position_GUID, mdata.vertices.data(), mdata.vertices.size() * sizeof(gfxm::vec3));
        mesh3d.setAttribArray(VFMT::ColorRGB_GUID, mdata.colors.data(), mdata.colors.size() * sizeof(color24));
        mesh3d.setAttribArray(VFMT::UV_GUID, mdata.uvs.data(), mdata.uvs.size() * sizeof(mdata.uvs[0]));
        mesh3d.setAttribArray(VFMT::UVLightmap_GUID, mdata.uvs_lightmap.data(), mdata.uvs_lightmap.size() * sizeof(mdata.uvs_lightmap[0]));
        mesh3d.setAttribArray(VFMT::Normal_GUID, mdata.normals.data(), mdata.normals.size() * sizeof(mdata.normals[0]));
        mesh3d.setAttribArray(VFMT::Tangent_GUID, mdata.tangents.data(), mdata.tangents.size() * sizeof(mdata.tangents[0]));
        mesh3d.setAttribArray(VFMT::Bitangent_GUID, mdata.binormals.data(), mdata.binormals.size() * sizeof(mdata.binormals[0]));
        mesh3d.setIndexArray(mdata.indices.data(), mdata.indices.size() * sizeof(uint32_t));

        mesh.reset(new gpuMesh);
        mesh->setData(&mesh3d);
        mesh->setDrawMode(MESH_DRAW_TRIANGLES);

        std::string matpath = MKSTR("experimental/hl2/materials/" << face_set.material_name << ".vmt");
        if (!hl2LoadMaterial(matpath.c_str(), part->material)) {
            LOG_ERR("Failed to find VMT: " << matpath);
        }/*
        part->material->addSampler(
            "texLightmap",
            scene->lm_texture
        );
        part->material->compile();*/
        //part->material = resGet<gpuMaterial>("materials/csg/breen_face.mat");

        part->renderable.reset(new gpuGeometryRenderable(part->material.get(), part->mesh->getMeshDesc(), 0, "hl2bsp"));
        part->renderable->setTransform(gfxm::mat4(1.f));
        if (scene->lm_texture) {
            part->renderable->addSamplerOverride(
                "texLightmap", scene->lm_texture
            );
        }
        part->renderable->compile();

        part->col_trimesh.reset(new CollisionTriangleMesh);
        part->col_trimesh->setData(
            mdata.vertices.data(), mdata.vertices.size(),
            mdata.indices.data(), mdata.indices.size()
        );
        part->col_shape.reset(new CollisionTriangleMeshShape);
        part->col_shape->setMesh(part->col_trimesh.get());
        part->collider.reset(new Collider);
        part->collider->setShape(part->col_shape.get());
        part->collider->setFlags(COLLIDER_STATIC);
        part->collider->collision_group |= COLLISION_LAYER_DEFAULT;
        part->collider->collision_mask |= COLLISION_LAYER_PROJECTILE;
    }
    fclose(f);
}


