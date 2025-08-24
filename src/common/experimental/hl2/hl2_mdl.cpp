#include "hl2_mdl.hpp"

#include <assert.h>
#include <stdint.h>
#include "log/log.hpp"
#include "math/gfxm.hpp"

#include "gpu/gpu_mesh.hpp"

#include "hl2_material.hpp"

#include "hl2_phy.hpp"


const int MAX_NUM_LODS = 8;
const int MAX_BONES_PER_VERT = 3;

struct mstudio_meshvertexdata_t {
    // indirection to this mesh's model's vertex data
    int32_t modelvertexdata;

    // used for fixup calcs when culling top level lods
    // expected number of mesh verts at desired lod
    int32_t	numLODVertexes[MAX_NUM_LODS];
};

struct mstudiomesh_t {
    int32_t		material;

    int32_t		modelindex;

    int32_t		numvertices;		// number of unique vertices/normals/texcoords
    int32_t		vertexoffset;		// vertex mstudiovertex_t

    int32_t		numflexes;			// vertex animation
    int32_t		flexindex;
    // inline mstudioflex_t* pFlex(int i) const { return (mstudioflex_t*)(((byte*)this) + flexindex) + i; };

    // special codes for material operations
    int32_t		materialtype;
    int32_t		materialparam;

    // a unique ordinal for this mesh
    int32_t		meshid;

    gfxm::vec3	center;

    mstudio_meshvertexdata_t vertexdata;

    int					unused[8]; // remove as appropriate
};

struct mstudio_modelvertexdata_t {
    // base of external vertex data stores
    int32_t pVertexData;
    int32_t pTangentData;
};

struct mstudiomodel_t {
    char				name[64];
    //FString GetName() const { return ReadString(this, 0); }

    int32_t					type;

    float				boundingradius;

    int32_t					nummeshes;
    int32_t					meshindex;
    const mstudiomesh_t* getMeshes() const {
        return (const mstudiomesh_t*)((uint8_t*)this + meshindex);
    }

    // cache purposes
    int32_t					numvertices;		// number of unique vertices/normals/texcoords
    int32_t					vertexindex;		// vertex Vector
    int32_t					tangentsindex;		// tangents Vector

    int32_t					numattachments;
    int32_t					attachmentindex;

    int32_t					numeyeballs;
    int32_t					eyeballindex;
    // inline  mstudioeyeball_t* pEyeball(int i) { return (mstudioeyeball_t*)(((byte*)this) + eyeballindex) + i; };

    mstudio_modelvertexdata_t vertexdata;

    int32_t					unused[8];		// remove as appropriate
};

struct mstudiobodyparts_t {
    int32_t					sznameindex;
    //FString GetName() const { return ReadString(this, sznameindex); }
    int32_t					nummodels;
    int32_t					base;
    int32_t					modelindex; // index into models array
    const mstudiomodel_t* getModels() const { 
        return (const mstudiomodel_t*)((uint8_t*)this + modelindex);
    }
};

struct mstudiotexture_t {
    // Number of bytes past the beginning of this structure
    // where the first character of the texture name can be found.
    int32_t    name_offset; // Offset for null-terminated string
    const char* getName() const {
        return (const char*)((uint8_t*)this + name_offset);
    }
    int32_t    flags;

    int32_t    used;        // Padding?
    int32_t    unused;      // Padding.

    int32_t    material;        // Placeholder for IMaterial
    int32_t    client_material; // Placeholder for void*

    int32_t    unused2[10]; // Final padding
    // Struct is 64 bytes long
};
static_assert(sizeof(mstudiotexture_t) == 64);

struct mstudiobone_t {
    int32_t					sznameindex;
    const char* getName() const { return (const char*)((uint8_t*)this + sznameindex); }

    int32_t		 			parent;		// parent bone
    int32_t					bonecontroller[6];	// bone controller index, -1 == none

    // default values
    float			pos[3];
    float			quat[4];
    float			rot[3];
    // compression scale
    float				posscale[3];
    float				rotscale[3];

    float			poseToBone[12]; // 3x4 matrix
    float			qAlignment[4];
    int32_t					flags;
    int32_t					proctype;
    int32_t					procindex;		// procedural rule
    int32_t					physicsbone;	// index into physically simulated bone
    int32_t					surfacepropidx;	// index into string tablefor property name
    int32_t					contents;		// See BSPFlags.h for the contents flags

    int32_t					unused[8];		// remove as appropriate
};
static_assert(sizeof(mstudiobone_t) == 216);

struct studiohdr_t
{
    int32_t         id;             // Model format ID, such as "IDST" (0x49 0x44 0x53 0x54)
    int32_t         version;        // Format version number, such as 48 (0x30,0x00,0x00,0x00)
    int32_t         checksum;       // This has to be the same in the phy and vtx files to load!
    char            name[64];       // The internal name of the model, padding with null bytes.
                                // Typically "my_model.mdl" will have an internal name of "my_model"
    int32_t         dataLength;     // Data size of MDL file in bytes.
 
    // A vector is 12 bytes, three 4-byte float-values in a row.
    gfxm::vec3      eyeposition;    // Position of player viewpoint relative to model origin
    gfxm::vec3      illumposition;  // Position (relative to model origin) used to calculate ambient light contribution and cubemap reflections for the entire model.
    gfxm::vec3      hull_min;       // Corner of model hull box with the least X/Y/Z values
    gfxm::vec3      hull_max;       // Opposite corner of model hull box
    gfxm::vec3      view_bbmin;     // Same, but for bounding box,
    gfxm::vec3      view_bbmax;     // which is used for view culling
 
    int32_t         flags;          // Binary flags in little-endian order. 
                                // ex (0x010000C0) means flags for position 0, 30, and 31 are set. 
                                // Set model flags section for more information
 
    /*
     * After this point, the header contains many references to offsets
     * within the MDL file and the number of items at those offsets.
     *
     * Offsets are from the very beginning of the file.
     * 
     * Note that indexes/counts are not always paired and ordered consistently.
     */    
 
    // mstudiobone_t
    int32_t         bone_count;    // Number of data sections (of type mstudiobone_t)
    int32_t         bone_offset;   // Offset of first data section
    const mstudiobone_t* getBones() const {
        return (const mstudiobone_t*)((uint8_t*)this + bone_offset);
    }
 
    // mstudiobonecontroller_t
    int32_t         bonecontroller_count;
    int32_t         bonecontroller_offset;
 
    // mstudiohitboxset_t
    int32_t         hitbox_count;
    int32_t         hitbox_offset;
 
    // mstudioanimdesc_t
    int32_t         localanim_count;
    int32_t         localanim_offset;
 
    // mstudioseqdesc_t
    int32_t         localseq_count;
    int32_t         localseq_offset;
 
    int32_t         activitylistversion; // ??
    int32_t         eventsindexed;       // ??
 
    // VMT texture filenames
    // mstudiotexture_t
    int32_t         texture_count;
    int32_t         texture_offset;
    const mstudiotexture_t* getMaterials() const {
        return (const mstudiotexture_t*)((uint8_t*)this + texture_offset);
    }
 
    // This offset points to a series of ints.
    // Each int value, in turn, is an offset relative to the start of this header/the-file,
    // At which there is a null-terminated string.
    int32_t         texturedir_count;
    int32_t         texturedir_offset;

    // Each skin-family assigns a texture-id to a skin location
    int32_t         skinreference_count;
    int32_t         skinrfamily_count;
    int32_t         skinreference_index;

    // mstudiobodyparts_t
    int32_t         bodypart_count;
    int32_t         bodypart_offset;
    const mstudiobodyparts_t* getBodyParts() const {
        return (const mstudiobodyparts_t*)((uint8_t*)this + bodypart_offset);
    }

    // Local attachment points        
    // mstudioattachment_t
    int32_t         attachment_count;
    int32_t         attachment_offset;
 
    // Node values appear to be single bytes, while their names are null-terminated strings.
    int32_t         localnode_count;
    int32_t         localnode_index;
    int32_t         localnode_name_index;

    // mstudioflexdesc_t
    int32_t         flexdesc_count;
    int32_t         flexdesc_index;

    // mstudioflexcontroller_t
    int32_t         flexcontroller_count;
    int32_t         flexcontroller_index;
 
    // mstudioflexrule_t
    int32_t         flexrules_count;
    int32_t         flexrules_index;

    // IK probably referse to inverse kinematics
    // mstudioikchain_t
    int32_t         ikchain_count;
    int32_t         ikchain_index;

    // Information about any "mouth" on the model for speech animation
    // More than one sounds pretty creepy.
    // mstudiomouth_t
    int32_t         mouths_count; 
    int32_t         mouths_index;

    // mstudioposeparamdesc_t
    int32_t         localposeparam_count;
    int32_t         localposeparam_index;
 
    /*
     * For anyone trying to follow along, as of this writing,
     * the next "surfaceprop_index" value is at position 0x0134 (308)
     * from the start of the file.
     */
 
    // Surface property value (single null-terminated string)
    int32_t         surfaceprop_index;

    // Unusual: In this one index comes first, then count.
    // Key-value data is a series of strings. If you can't find
    // what you're interested in, check the associated PHY file as well.
    int32_t         keyvalue_index;
    int32_t         keyvalue_count;    

    // More inverse-kinematics
    // mstudioiklock_t
    int32_t         iklock_count;
    int32_t         iklock_index;
 
 
    float           mass;      // Mass of object (4-bytes) in kilograms

    int32_t         contents;    // contents flag, as defined in bspflags.h
    // not all content types are valid; see 
    // documentation on $contents QC command

    // Other models can be referenced for re-used sequences and animations
    // (See also: The $includemodel QC option.)
    // mstudiomodelgroup_t
    int32_t         includemodel_count;
    int32_t         includemodel_index;

    int32_t         virtualModel;    // Placeholder for mutable-void*
    // Note that the SDK only compiles as 32-bit, so an int and a pointer are the same size (4 bytes)

    // mstudioanimblock_t
    int32_t         animblocks_name_index;
    int32_t         animblocks_count;
    int32_t         animblocks_index;

    int32_t         animblockModel; // Placeholder for mutable-void*

    // Points to a series of bytes?
    int32_t         bonetablename_index;

    int32_t         vertex_base;    // Placeholder for void*
    int32_t         offset_base;    // Placeholder for void*
    
    // Used with $constantdirectionallight from the QC 
    // Model should have flag #13 set if enabled
    int8_t          directionaldotproduct;
    
    int8_t          rootLod;    // Preferred rather than clamped
    
    // 0 means any allowed, N means Lod 0 -> (N-1)
    int8_t          numAllowedRootLods;    
    
    int8_t          unused0; // ??
    int32_t         unused1; // ??

    // mstudioflexcontrollerui_t
    int32_t         flexcontrollerui_count;
    int32_t         flexcontrollerui_index;

    float           vertAnimFixedPointScale; // ??
    int32_t         unused2;
    
    /**
     * Offset for additional header information.
     * May be zero if not present, or also 408 if it immediately 
     * follows this studiohdr_t
     */
    // studiohdr2_t
    int32_t         studiohdr2index;

    int32_t         unused3; // ??
    
    /**
     * As of this writing, the header is 408 bytes long in total
     */
};
static_assert(sizeof(studiohdr_t) == 408);

struct MDLFile {
    std::vector<uint8_t> raw_data;

    gfxm::vec3 root_translation;
    gfxm::quat root_rotation;

    const studiohdr_t* getHeader() const {
        return (const studiohdr_t*)&raw_data[0];
    }
};


struct vertexFileHeader_t {
	int32_t	id;								// MODEL_VERTEX_FILE_ID
	int32_t	version;						// MODEL_VERTEX_FILE_VERSION
	int32_t	checksum;						// same as studiohdr_t, ensures sync      ( Note: maybe long instead of int in versions other than 4. )
	int32_t	numLODs;						// num of valid lods
	int32_t	numLODVertices[MAX_NUM_LODS];	// num verts for desired root lod
	int32_t	numFixups;						// num of vertexFileFixup_t
	int32_t	fixupTableStart;				// offset from base to fixup table
	int32_t	vertexDataStart;				// offset from base to vertex block
	int32_t	tangentDataStart;				// offset from base to tangent block
};

struct boneweight_t {
    float weights[MAX_BONES_PER_VERT];
    char bones[MAX_BONES_PER_VERT];
    uint8_t numbones;
};
static_assert(sizeof(boneweight_t) == 16);

struct vertex_t {
    boneweight_t weights;
    gfxm::vec3 position;
    gfxm::vec3 normal;
    gfxm::vec2 uv;
};
static_assert(sizeof(vertex_t) == 48);

// apply sequentially to lod sorted vertex and tangent pools to re-establish mesh order
struct vertexFileFixup_t
{
    int32_t	lod;			// used to skip culled root lod
    int32_t	sourceVertexID;		// absolute index from start of vertex/tangent blocks
    int32_t	numVertexes;
};
static_assert(sizeof(vertexFileFixup_t) == 12);

struct VVDLod {
    std::vector<gfxm::vec3> vertices;
    std::vector<gfxm::vec3> normals;
    std::vector<gfxm::vec2> uvs;
    std::vector<gfxm::vec3> tangents;
    std::vector<gfxm::vec3> binormals;
    std::vector<uint32_t> colors;
};
struct VVDFile {
    std::vector<std::unique_ptr<VVDLod>> lods;
};

#define FREAD(BUFFER, SIZE, COUNT, FILE) \
if (fread(BUFFER, SIZE, COUNT, FILE) != COUNT) { \
    assert(false); \
    return false; \
}

static void convertVertices(
    std::vector<gfxm::vec3>& vertices,
    std::vector<gfxm::vec3>& normals,
    std::vector<gfxm::vec3>& tangents,
    std::vector<gfxm::vec3>& binormals
) {
    for (int i = 0; i < vertices.size(); ++i) {
        gfxm::mat4 transform = gfxm::to_mat4(gfxm::angle_axis(gfxm::radian(-90.f), gfxm::vec3(1, 0, 0)));
        vertices[i] = transform * gfxm::vec4(vertices[i], 1.f);
        //float tmp = vertices[i].y;
        //vertices[i].y = vertices[i].z;
        //vertices[i].z = -tmp;
        /*
        float scale = 1.f / 200.f;
        vertices_triangulated[i] *= scale;
        */
        //float scale = .01905f;
        float scale = 1.f / 41.f;
        vertices[i] *= scale;
    }
    for (int i = 0; i < normals.size(); ++i) {
        gfxm::mat4 transform = gfxm::to_mat4(gfxm::angle_axis(gfxm::radian(-90.f), gfxm::vec3(1, 0, 0)));
        normals[i] = transform * gfxm::vec4(normals[i], 0.f);
        //float tmp = normals[i].y;
        //normals[i].y = normals[i].z;
        //normals[i].z = -tmp;
    }
    for (int i = 0; i < tangents.size(); ++i) {
        gfxm::mat4 transform = gfxm::to_mat4(gfxm::angle_axis(gfxm::radian(-90.f), gfxm::vec3(1, 0, 0)));
        tangents[i] = transform * gfxm::vec4(tangents[i], 0.f);
        //float tmp = tangents[i].y;
        //tangents[i].y = tangents[i].z;
        //tangents[i].z = -tmp;
    }
    for (int i = 0; i < binormals.size(); ++i) {
        gfxm::mat4 transform = gfxm::to_mat4(gfxm::angle_axis(gfxm::radian(-90.f), gfxm::vec3(1, 0, 0)));
        binormals[i] = transform * gfxm::vec4(binormals[i], 0.f);
        //float tmp = binormals[i].y;
        //binormals[i].y = binormals[i].z;
        //binormals[i].z = -tmp;
    }
}

bool hl2LoadVVD(const MDLFile& mdl, const char* path_, VVDFile& vvd) {
    std::string vvdpath = MKSTR("experimental/hl2/" << path_ << ".vvd");

    FILE* f = fopen(vvdpath.c_str(), "rb");
    if (!f) {
        LOG_ERR("Failed to OPEN file '" << vvdpath << "'");
        return false;
    }

    gfxm::mat4 root_transform = 
        gfxm::translate(gfxm::mat4(1.f), mdl.root_translation)
        * gfxm::to_mat4(mdl.root_rotation);

    vertexFileHeader_t head = { 0 };
    FREAD(&head, sizeof(head), 1, f);
    LOG_DBG("VVD id: " << head.id);
    LOG_DBG("VVD version: " << head.version);
    /*LOG_DBG("VVD checksum: " << head.checksum);
    LOG_DBG("VVD numLODs: " << head.numLODs);
    for(int i = 0; i < head.numLODs; ++i) {
        LOG_DBG("VVD numLODVertices(" << i << "): " << head.numLODVertices[i]);
    }
    LOG_DBG("VVD numFixups: " << head.numFixups);
    LOG_DBG("VVD fixupTableStart: " << head.fixupTableStart);
    LOG_DBG("VVD vertexDataStart: " << head.vertexDataStart);
    LOG_DBG("VVD tangentDataStart: " << head.tangentDataStart);*/

    std::vector<gfxm::vec3> vertices;
    std::vector<gfxm::vec3> normals;
    std::vector<gfxm::vec2> uvs;
    std::vector<gfxm::vec3> tangents;
    std::vector<gfxm::vec3> binormals;
    std::vector<uint32_t> colors;

    std::vector<vertex_t> vvd_vertices;
    std::vector<gfxm::vec4> tangents4;

    const int LOD = 0;

    if(head.numFixups > 0) {
        fseek(f, head.fixupTableStart, SEEK_SET);
        std::vector<vertexFileFixup_t> fixups(head.numFixups);
        FREAD(fixups.data(), sizeof(vertexFileFixup_t), head.numFixups, f);
    
        for (int i = 0; i < fixups.size(); ++i) {
            const auto& fixup = fixups[i];
            if (LOD > fixup.lod) {
                continue;
            }
            /*LOG("fixup[" << i << "].lod: " << fixup.lod);
            LOG("fixup[" << i << "].sourceVertexID: " << fixup.sourceVertexID);
            LOG("fixup[" << i << "].numVertexes: " << fixup.numVertexes);*/
            uint64_t old_size = vvd_vertices.size();
            vvd_vertices.resize(vvd_vertices.size() + fixup.numVertexes);
            tangents4.resize(tangents4.size() + fixup.numVertexes);

            fseek(f, head.vertexDataStart + fixup.sourceVertexID * sizeof(vertex_t), SEEK_SET);
            FREAD(vvd_vertices.data() + old_size, sizeof(vertex_t), fixup.numVertexes, f);
            fseek(f, head.tangentDataStart + fixup.sourceVertexID * sizeof(gfxm::vec4), SEEK_SET);
            FREAD(tangents4.data() + old_size, sizeof(gfxm::vec4), fixup.numVertexes, f);
        }
    } else {
        fseek(f, head.vertexDataStart, SEEK_SET);
        vvd_vertices.resize(head.numLODVertices[0]);
        FREAD(vvd_vertices.data(), head.numLODVertices[0] * sizeof(vertex_t), 1, f);

        fseek(f, head.tangentDataStart, SEEK_SET);
        tangents4.resize(head.numLODVertices[0]);
        FREAD(tangents4.data(), head.numLODVertices[0] * sizeof(gfxm::vec4), 1, f);
    }

    vertices.resize(vvd_vertices.size());
    normals.resize(vvd_vertices.size());
    uvs.resize(vvd_vertices.size());
    for (int i = 0; i < vvd_vertices.size(); ++i) {
        vertices[i] = vvd_vertices[i].position;
        normals[i] = vvd_vertices[i].normal;
        uvs[i] = vvd_vertices[i].uv;
    }

    tangents.resize(vvd_vertices.size());
    for (int i = 0; i < vvd_vertices.size(); ++i) {
        tangents[i] = tangents4[i];
    }

    binormals.resize(vvd_vertices.size());
    for (int i = 0; i < vvd_vertices.size(); ++i) {
        binormals[i] = gfxm::normalize(gfxm::cross(normals[i], gfxm::vec3(tangents[i])));
    }

    colors.resize(vertices.size());
    std::fill(colors.begin(), colors.end(), 0xFFFFFFFF);

    fclose(f);
    
    for (int i = 0; i < vvd_vertices.size(); ++i) {
        vertices[i] = root_transform * gfxm::vec4(vertices[i], 1);
        normals[i] = root_transform * gfxm::vec4(normals[i], 0);
        tangents[i] = root_transform * gfxm::vec4(tangents[i], 0);
        binormals[i] = root_transform * gfxm::vec4(binormals[i], 0);
    }
    
    convertVertices(
        vertices,
        normals,
        tangents,
        binormals
    );

    vvd.lods.push_back(std::unique_ptr<VVDLod>(new VVDLod));
    VVDLod* lod = vvd.lods.back().get();
    lod->vertices = vertices;
    lod->normals = normals;
    lod->uvs = uvs;
    lod->tangents = tangents;
    lod->binormals = binormals;
    lod->colors = colors;

    return true;
}

struct vtxheader_t {
    // file version as defined by OPTIMIZED_MODEL_FILE_VERSION (currently 7)
    int32_t version;

    // hardware params that affect how the model is to be optimized.
    int32_t vertCacheSize;
    uint16_t maxBonesPerStrip;
    uint16_t maxBonesPerTri;
    int32_t maxBonesPerVert;

    // must match checkSum in the .mdl
    int32_t checkSum;

    int32_t numLODs; // Also specified in ModelHeader_t's and should match

    // Offset to materialReplacementList Array. one of these for each LOD, 8 in total
    int32_t materialReplacementListOffset;

    //Defines the size and location of the body part array
    int32_t numBodyParts;
    int32_t bodyPartOffset;
};
static_assert(sizeof(vtxheader_t) == 36);

struct bodypartheader_t {
    int32_t numModels;
    int32_t modelOffset;
};
static_assert(sizeof(bodypartheader_t) == 8);

struct modelheader_t {
    int32_t numLODs;   //This is also specified in FileHeader_t
    int32_t lodOffset;
};
static_assert(sizeof(bodypartheader_t) == 8);

struct lodheader_t {
    int32_t numMeshes;
    int32_t meshOffset;
    float   switchPoint;
};
static_assert(sizeof(lodheader_t) == 12);

#pragma pack(push, 1)
struct meshheader_t {
    int32_t numStripGroups;
    int32_t stripGroupHeaderOffset;
    uint8_t flags;
};
#pragma pack(pop)
static_assert(sizeof(meshheader_t) == 9);

#pragma pack(push, 1)
struct stripgroupheader_t {
    int32_t numVerts;
    int32_t vertOffset;

    int32_t numIndices;
    int32_t indexOffset;

    int32_t numStrips;
    int32_t stripOffset;

    uint8_t flags;

    // The following fields are only present if MDL version is >=49
    // Points to an array of unsigned shorts (16 bits each)
    //int32_t numTopologyIndices;
    //int32_t topologyOffset;
};
#pragma pack(pop)
static_assert(sizeof(stripgroupheader_t) == 25);

enum STRIP_HEADER_FLAGS {
    STRIP_IS_TRILIST	= 0x01,
    STRIP_IS_TRISTRIP	= 0x02
};

#pragma pack(push, 1)
struct strip_t {
    int32_t numIndices;
    int32_t indexOffset;

    int32_t numVerts;    
    int32_t vertOffset;

    int16_t numBones;

    uint8_t flags;

    int32_t numBoneStateChanges;
    int32_t boneStateChangeOffset;

    // MDL Version 49 and up only
    //int32_t numTopologyIndices;
    //int32_t topologyOffset;
};
#pragma pack(pop)
static_assert(sizeof(strip_t) == 27);

#pragma pack(push, 1)
struct vertexinfo_t
{
    // these index into the mesh's vert[origMeshVertID]'s bones
    uint8_t boneWeightIndex[3];
    uint8_t numBones;

    uint16_t origMeshVertID;

    // for sw skinned verts, these are indices into the global list of bones
    // for hw skinned verts, these are hardware bone indices
    int8_t boneID[3];
};
#pragma pack(pop)
static_assert(sizeof(vertexinfo_t) == 9);

struct VTXMesh {
    std::vector<uint32_t> indices;
    uint32_t materialIdx = 0;
};
struct VTXLOD {
    std::vector<VTXMesh> meshes;
};
struct VTXModel {
    std::vector<VTXLOD> lods;
};
struct VTXFile {
    std::vector<VTXModel> models;
};

static void convertIndices(
    uint32_t* indices,
    size_t count
) {
    for (int i = 0; i < count; i += 3) {
        uint32_t tmp = indices[i + 2];
        indices[i + 2] = indices[i + 1];
        indices[i + 1] = tmp;
    }
}

bool hl2LoadVTX(const MDLFile& mdl, const char* path_, VTXFile& vtx) {
    std::string vtxpath = MKSTR("experimental/hl2/" << path_ << ".dx90.vtx");

    FILE* f = fopen(vtxpath.c_str(), "rb");
    if (!f) {
        LOG_ERR("Failed to open file '" << vtxpath << "'");
        assert(false);
        return false;
    }

    vtxheader_t head = { 0 };
    FREAD(&head, sizeof(head), 1, f);

    //LOG("VTX numBodyParts: " << head.numBodyParts);
    //LOG("VTX bodyPartOffset: " << head.bodyPartOffset);
    //LOG("VTX numLODs: " << head.numLODs);

    std::vector<bodypartheader_t> bodypartheads(head.numBodyParts);
    fseek(f, head.bodyPartOffset, SEEK_SET);
    FREAD(bodypartheads.data(), sizeof(bodypartheader_t), head.numBodyParts, f);
    for (int i = 0; i < bodypartheads.size(); ++i) {
        const bodypartheader_t& part = bodypartheads[i];
        //LOG("\tbodyparthead.numModels: " << bodypartheads[i].numModels);
        //LOG("\tbodyparthead.modelOffset: " << bodypartheads[i].modelOffset);

        std::vector<modelheader_t> models(part.numModels);
        fseek(
            f,
            head.bodyPartOffset + i * sizeof(bodypartheader_t) +
            part.modelOffset,
            SEEK_SET
        );
        FREAD(models.data(), sizeof(modelheader_t), part.numModels, f);
        for(int j = 0; j < models.size(); ++j) {
            const modelheader_t& model = models[j];
            //LOG("\t\tmodel.numLODs: " << model.numLODs);
            //LOG("\t\tmodel.lodOffset: " << model.lodOffset);

            vtx.models.push_back(VTXModel());
            VTXModel& vtxmodel = vtx.models.back();

            std::vector<lodheader_t> lods(model.numLODs);
            fseek(
                f,
                head.bodyPartOffset + i * sizeof(bodypartheader_t) +
                part.modelOffset + j * sizeof(modelheader_t) +
                model.lodOffset,
                SEEK_SET
            );
            FREAD(lods.data(), sizeof(lodheader_t), model.numLODs, f);
            for (int k = 0; k < lods.size(); ++k) {
                const lodheader_t& lod = lods[k];
                //LOG("\t\t\tlod.numMeshes: " << lod.numMeshes);
                //LOG("\t\t\tlod.meshOffset: " << lod.meshOffset);
                //LOG("\t\t\tlod.switchPoint: " << lod.switchPoint);

                vtxmodel.lods.push_back(VTXLOD());
                VTXLOD& vtxlod = vtxmodel.lods.back();

                std::vector<meshheader_t> meshes(lod.numMeshes);
                fseek(
                    f,
                    head.bodyPartOffset + i * sizeof(bodypartheader_t) +
                    part.modelOffset + j * sizeof(modelheader_t) +
                    model.lodOffset + k * sizeof(lodheader_t) +
                    lod.meshOffset,
                    SEEK_SET
                );
                FREAD(meshes.data(), sizeof(meshheader_t), lod.numMeshes, f);
                for (int l = 0; l < meshes.size(); ++l) {
                    const mstudiomesh_t& mdlmesh = mdl.getHeader()->getBodyParts()[i].getModels()[j].getMeshes()[l];

                    const meshheader_t& mesh = meshes[l];
                    //LOG("\t\t\t\tmesh.numStripGroups: " << mesh.numStripGroups);
                    //LOG("\t\t\t\tmesh.stripGroupHeaderOffset: " << mesh.stripGroupHeaderOffset);

                    vtxlod.meshes.push_back(VTXMesh());
                    VTXMesh& vtxmesh = vtxlod.meshes.back();

                    std::vector<stripgroupheader_t> strip_groups(mesh.numStripGroups);
                    fseek(
                        f,
                        head.bodyPartOffset + i * sizeof(bodypartheader_t) +
                        part.modelOffset + j * sizeof(modelheader_t) +
                        model.lodOffset + k * sizeof(lodheader_t) +
                        lod.meshOffset + l * sizeof(meshheader_t) +
                        mesh.stripGroupHeaderOffset,
                        SEEK_SET
                    );
                    FREAD(strip_groups.data(), sizeof(stripgroupheader_t), mesh.numStripGroups, f);
                    for (int isg = 0; isg < strip_groups.size(); ++isg) {
                        const stripgroupheader_t& strip_group = strip_groups[isg];
                        //LOG("\t\t\t\t\tstrip_group.numVerts: " << strip_group.numVerts);
                        //LOG("\t\t\t\t\tstrip_group.numIndices: " << strip_group.numIndices);
                        //LOG("\t\t\t\t\tstrip_group.indexOffset: " << strip_group.indexOffset);
                        //LOG("\t\t\t\t\tstrip_group.numStrips: " << strip_group.numStrips);
                        //LOG("\t\t\t\t\tstrip_group.stripOffset: " << strip_group.stripOffset);

                        uint64_t strip_group_indexOffset =
                            head.bodyPartOffset + i * sizeof(bodypartheader_t) +
                            part.modelOffset + j * sizeof(modelheader_t) +
                            model.lodOffset + k * sizeof(lodheader_t) +
                            lod.meshOffset + l * sizeof(meshheader_t) +
                            mesh.stripGroupHeaderOffset + isg * sizeof(stripgroupheader_t) +
                            strip_group.indexOffset;
                        //LOG("\t\t\t\t\t*strip_group_indexOffset: " << strip_group_indexOffset);
                        fseek(
                            f,
                            strip_group_indexOffset,
                            SEEK_SET
                        );
                        std::vector<uint16_t> indices16(strip_group.numIndices);
                        FREAD(indices16.data(), sizeof(uint16_t), strip_group.numIndices, f);
                        /*int begin = vtxmesh.indices.size();
                        vtxmesh.indices.resize(vtxmesh.indices.size() + strip_group.numIndices);
                        for (int i16 = 0; i16 < strip_group.numIndices; ++i16) {
                            vtxmesh.indices[begin + i16] = uint32_t(indices16[i16]);
                        }*/

                        uint64_t strip_group_vertOffset =
                            head.bodyPartOffset + i * sizeof(bodypartheader_t) +
                            part.modelOffset + j * sizeof(modelheader_t) +
                            model.lodOffset + k * sizeof(lodheader_t) +
                            lod.meshOffset + l * sizeof(meshheader_t) +
                            mesh.stripGroupHeaderOffset + isg * sizeof(stripgroupheader_t) +
                            strip_group.vertOffset;
                        //LOG("\t\t\t\t\t*strip_group_vertOffset: " << strip_group_vertOffset);
                        fseek(
                            f,
                            strip_group_vertOffset,
                            SEEK_SET
                        );
                        std::vector<vertexinfo_t> verts(strip_group.numVerts);
                        FREAD(verts.data(), 9, strip_group.numVerts, f);

                        std::vector<strip_t> strips(strip_group.numStrips);
                        fseek(
                            f,
                            head.bodyPartOffset + i * sizeof(bodypartheader_t) +
                            part.modelOffset + j * sizeof(modelheader_t) +
                            model.lodOffset + k * sizeof(lodheader_t) +
                            lod.meshOffset + l * sizeof(meshheader_t) +
                            mesh.stripGroupHeaderOffset + isg * sizeof(stripgroupheader_t) +
                            strip_group.stripOffset,
                            SEEK_SET
                        );
                        FREAD(strips.data(), sizeof(strip_t), strip_group.numStrips, f);
                        for (int is = 0; is < strips.size(); ++is) {
                            const strip_t& strip = strips[is];
                            /*
                            LOG("\t\t\t\t\t\tSTRIP ===========================");
                            LOG("\t\t\t\t\t\tstrip.numIndices: " << strip.numIndices);
                            LOG("\t\t\t\t\t\tstrip.indexOffset: " << strip.indexOffset);

                            LOG("\t\t\t\t\t\tstrip.numVerts: " << strip.numVerts);    
                            LOG("\t\t\t\t\t\tstrip.vertOffset: " << strip.vertOffset);

                            LOG("\t\t\t\t\t\tstrip.numBones: " << strip.numBones);

                            LOG("\t\t\t\t\t\tstrip.flags: " << int32_t(strip.flags));

                            LOG("\t\t\t\t\t\tstrip.numBoneStateChanges: " << strip.numBoneStateChanges);
                            LOG("\t\t\t\t\t\tstrip.boneStateChangeOffset: " << strip.boneStateChangeOffset);
                            */
                            uint64_t strip_indexOffset =
                                head.bodyPartOffset + i * sizeof(bodypartheader_t) +
                                part.modelOffset + j * sizeof(modelheader_t) +
                                model.lodOffset + k * sizeof(lodheader_t) +
                                lod.meshOffset + l * sizeof(meshheader_t) +
                                mesh.stripGroupHeaderOffset + isg * sizeof(stripgroupheader_t) +
                                strip_group.indexOffset +
                                strip.indexOffset;
                            //LOG("\t\t\t\t\t\t*strip_indexOffset: " << strip_indexOffset);
                            fseek(
                                f,
                                strip_indexOffset,
                                SEEK_SET
                            );
                            std::vector<uint16_t> strip_indices16(strip.numIndices);
                            FREAD(strip_indices16.data(), sizeof(uint16_t), strip.numIndices, f);

                            uint64_t strip_vertOffset =
                                head.bodyPartOffset + i * sizeof(bodypartheader_t) +
                                part.modelOffset + j * sizeof(modelheader_t) +
                                model.lodOffset + k * sizeof(lodheader_t) +
                                lod.meshOffset + l * sizeof(meshheader_t) +
                                mesh.stripGroupHeaderOffset + isg * sizeof(stripgroupheader_t) +
                                strip_group.vertOffset +
                                strip.vertOffset;
                            //LOG("\t\t\t\t\t\t*i: " << i << ", j: " << j << ", k: " << k << ", l: " << l << ", isg: " << isg);
                            fseek(
                                f,
                                strip_vertOffset,
                                SEEK_SET
                            );
                            std::vector<vertexinfo_t> strip_verts(strip.numVerts);
                            FREAD(strip_verts.data(), sizeof(vertexinfo_t), strip.numVerts, f);                            

                            if(strip.flags == STRIP_HEADER_FLAGS::STRIP_IS_TRILIST) {
                                for (int si = 0; si < strip.numIndices; ++si) {
                                    uint16_t idx0 = strip_indices16[si];
                                    vtxmesh.indices.push_back(mdlmesh.vertexoffset + strip_verts[idx0].origMeshVertID);
                                }
                            } else if (strip.flags == STRIP_HEADER_FLAGS::STRIP_IS_TRISTRIP) {
                                for (int si = 0; si < strip.numIndices - 2; ++si) {
                                    uint16_t idx0 = strip_indices16[si];
                                    uint16_t idx1 = strip_indices16[si + 1];
                                    uint16_t idx2 = strip_indices16[si + 2];
                                    vtxmesh.indices.push_back(mdlmesh.vertexoffset + strip_verts[idx0].origMeshVertID);
                                    vtxmesh.indices.push_back(mdlmesh.vertexoffset + strip_verts[idx1].origMeshVertID);
                                    vtxmesh.indices.push_back(mdlmesh.vertexoffset + strip_verts[idx2].origMeshVertID);
                                }
                            } else {
                                LOG_ERR("Unknown strip flag");
                                assert(false);
                            }
                        }

                        convertIndices(vtxmesh.indices.data(), vtxmesh.indices.size());
                    }

                    // TODO: Material
                    vtxmesh.materialIdx = mdlmesh.material;
                }
            }
        }
    }

    fclose(f);
    return true;
}


bool hl2LoadMDL(const char* path_, MDLFile& out_mdl) {
    std::string mdlpath = MKSTR("experimental/hl2/" << path_ << ".mdl");
    FILE* f = fopen(mdlpath.c_str(), "rb");
    if (!f) {
        LOG_ERR("Failed to OPEN file '" << mdlpath << "'");
        return false;
    }

    fseek(f, 0, SEEK_END);
    uint64_t fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    out_mdl.raw_data.resize(fsize);
    if (fread(out_mdl.raw_data.data(), fsize, 1, f) != 1) {
        LOG_ERR("Failed to READ file '" << mdlpath << "'");
        assert(false);
        fclose(f);
        return false;
    }

    const studiohdr_t* head = out_mdl.getHeader();

    LOG("MDL version: " << head->version);
    const mstudiobone_t* bones = head->getBones();
    if (head->bone_count > 0) {
        out_mdl.root_translation.x = bones[0].pos[0];
        out_mdl.root_translation.y = bones[0].pos[1];
        out_mdl.root_translation.z = bones[0].pos[2];
        /*
        out_mdl.root_rotation.x = bones[0].quat[3];
        out_mdl.root_rotation.y = bones[0].quat[1];
        out_mdl.root_rotation.z = bones[0].quat[2];
        out_mdl.root_rotation.w = bones[0].quat[0];*/

        gfxm::vec3 euler(
            bones[0].rot[0],
            bones[0].rot[1],
            bones[0].rot[2]
        );
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
        out_mdl.root_rotation = q;

        LOG(
            "translation: { " << 
            out_mdl.root_translation.x << ", " <<
            out_mdl.root_translation.y << ", " <<
            out_mdl.root_translation.z << " }"
        );
        LOG(
            "rotation: { " << 
            out_mdl.root_rotation.x << ", " <<
            out_mdl.root_rotation.y << ", " <<
            out_mdl.root_rotation.z << ", " <<
            out_mdl.root_rotation.w << " }"
        );
    }
    const mstudiobodyparts_t* bodyparts = head->getBodyParts();
    for (int i = 0; i < head->bodypart_count; ++i) {
        const mstudiobodyparts_t& bodypart = bodyparts[i];
        LOG("bodypart(" << i << ").base: " << bodypart.base);
        LOG("bodypart(" << i << ").modelindex: " << bodypart.modelindex);
        LOG("bodypart(" << i << ").nummodels: " << bodypart.nummodels);
        
        const mstudiomodel_t* models = bodypart.getModels();
        for (int j = 0; j < bodypart.nummodels; ++j) {
            const mstudiomodel_t& model = models[j];
            LOG("model(" << j << ").name: " << model.name);

            const mstudiomesh_t* meshes = model.getMeshes();
            for (int k = 0; k < model.nummeshes; ++k) {
                const mstudiomesh_t& mesh = meshes[k];
                LOG("mesh(" << k << ").numvertices: " << mesh.numvertices);
                
            }
        }
    }

    fclose(f);
    return true;
}

bool hl2LoadModel(const char* path, MDLModel* out_model) {
    LOG("hl2LoadModel: " << path);
    std::string spath(path);
    spath.resize(spath.size() - 4); // remove ".mdl" part
    
    MDLFile mdl;
    if (!hl2LoadMDL(spath.c_str(), mdl)) {
        return false;
    }
    VVDFile vvd;
    if (!hl2LoadVVD(mdl, spath.c_str(), vvd)) {
        return false;
    }
    VTXFile vtx;
    if (!hl2LoadVTX(mdl, spath.c_str(), vtx)) {
        return false;
    }
    if (!hl2LoadPHY(spath.c_str(), out_model->phy)) {
        // ???
    }

    if (vvd.lods.size() == 0) {
        return false;
    }

    {
        LOG("MDL: Loading materials");
        out_model->materials.resize(mdl.getHeader()->texture_count);
        for (int i = 0; i < mdl.getHeader()->texture_count; ++i) {
            const mstudiotexture_t& tex = mdl.getHeader()->getMaterials()[i];
            std::filesystem::path path_dir = spath;
            path_dir = path_dir.remove_filename();
            std::string vmtpath = MKSTR("experimental/hl2/materials/" << path_dir.string() << tex.getName() << ".vmt");
            LOG(vmtpath);
            hl2LoadMaterial(vmtpath.c_str(), out_model->materials[i]);
        }
    }

    out_model->static_model.reset_acquire();
    for(int i = 0; i < out_model->materials.size(); ++i) {
        out_model->static_model->addMaterial(out_model->materials[i]);
    }

    for (int imodel = 0; imodel < vtx.models.size(); ++imodel) {
        const auto& lod = vtx.models[imodel].lods[0];
        for (int imesh = 0; imesh < lod.meshes.size(); ++imesh) {
            const auto& vtxmesh = lod.meshes[imesh];

            RHSHARED<gpuMesh> gpu_mesh;
            gpu_mesh.reset_acquire();

            Mesh3d cpu_mesh;
            cpu_mesh.setAttribArray(VFMT::Position_GUID, vvd.lods[0]->vertices.data(), vvd.lods[0]->vertices.size() * sizeof(vvd.lods[0]->vertices[0]));
            cpu_mesh.setAttribArray(VFMT::Normal_GUID, vvd.lods[0]->normals.data(), vvd.lods[0]->normals.size() * sizeof(vvd.lods[0]->normals[0]));
            cpu_mesh.setAttribArray(VFMT::UV_GUID, vvd.lods[0]->uvs.data(), vvd.lods[0]->uvs.size() * sizeof(vvd.lods[0]->uvs[0]));
            cpu_mesh.setAttribArray(VFMT::Tangent_GUID, vvd.lods[0]->tangents.data(), vvd.lods[0]->tangents.size() * sizeof(vvd.lods[0]->tangents[0]));
            cpu_mesh.setAttribArray(VFMT::Bitangent_GUID, vvd.lods[0]->binormals.data(), vvd.lods[0]->binormals.size() * sizeof(vvd.lods[0]->binormals[0]));
            cpu_mesh.setAttribArray(VFMT::ColorRGB_GUID, vvd.lods[0]->colors.data(), vvd.lods[0]->colors.size() * sizeof(vvd.lods[0]->colors[0]));
            cpu_mesh.setIndexArray((uint32_t*)vtxmesh.indices.data(), vtxmesh.indices.size() * sizeof(uint32_t));
            
            gpu_mesh->setData(&cpu_mesh);
            gpu_mesh->setDrawMode(MESH_DRAW_TRIANGLES);

            StaticModelPart part;
            part.material_idx = vtxmesh.materialIdx;
            part.mesh = gpu_mesh;
            out_model->static_model->addMesh(part);
        }
    }


    out_model->vertex_buffer.setArrayData(vvd.lods[0]->vertices.data(), vvd.lods[0]->vertices.size() * sizeof(vvd.lods[0]->vertices[0]));
    out_model->normal_buffer.setArrayData(vvd.lods[0]->normals.data(), vvd.lods[0]->normals.size() * sizeof(vvd.lods[0]->normals[0]));
    out_model->uv_buffer.setArrayData(vvd.lods[0]->uvs.data(), vvd.lods[0]->uvs.size() * sizeof(vvd.lods[0]->uvs[0]));
    out_model->tangent_buffer.setArrayData(vvd.lods[0]->tangents.data(), vvd.lods[0]->tangents.size() * sizeof(vvd.lods[0]->tangents[0]));
    out_model->binormal_buffer.setArrayData(vvd.lods[0]->binormals.data(), vvd.lods[0]->binormals.size() * sizeof(vvd.lods[0]->binormals[0]));
    out_model->color_buffer.setArrayData(vvd.lods[0]->colors.data(), vvd.lods[0]->colors.size() * sizeof(vvd.lods[0]->colors[0]));

    for (int imodel = 0; imodel < vtx.models.size(); ++imodel) {
        const auto& lod = vtx.models[imodel].lods[0];
        for (int imesh = 0; imesh < lod.meshes.size(); ++imesh) {
            const auto& vtxmesh = lod.meshes[imesh];

            MDLMesh* mesh = new MDLMesh;
            mesh->index_buffer.setArrayData(vtxmesh.indices.data(), vtxmesh.indices.size() * sizeof(uint32_t));
            mesh->mesh_desc.setAttribArray(VFMT::Position_GUID, &out_model->vertex_buffer, 0, 0);
            mesh->mesh_desc.setAttribArray(VFMT::Normal_GUID, &out_model->normal_buffer, 0, 0);
            mesh->mesh_desc.setAttribArray(VFMT::UV_GUID, &out_model->uv_buffer, 0, 0);
            mesh->mesh_desc.setAttribArray(VFMT::Tangent_GUID, &out_model->tangent_buffer, 0, 0);
            mesh->mesh_desc.setAttribArray(VFMT::Bitangent_GUID, &out_model->binormal_buffer, 0, 0);
            mesh->mesh_desc.setAttribArray(VFMT::ColorRGB_GUID, &out_model->color_buffer, 0, 0);
            mesh->mesh_desc.setIndexArray(&mesh->index_buffer);
            mesh->mesh_desc.setDrawMode(MESH_DRAW_TRIANGLES);

            mesh->material = out_model->materials[vtxmesh.materialIdx];

            out_model->meshes.push_back(std::unique_ptr<MDLMesh>(mesh));
        }
    }

    return true;
}

