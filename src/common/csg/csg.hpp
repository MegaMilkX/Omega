#pragma once

#include <set>
#include <unordered_set>
#include <unordered_map>
#include "gpu/gpu_texture_2d.hpp"
#include "gpu/gpu_material.hpp"
#include "math/gfxm.hpp"

constexpr float VERTEX_HASH_PRECISION = 100.0f;


enum RELATION_TYPE {
    RELATION_FRONT,
    RELATION_OUTSIDE = RELATION_FRONT,
    RELATION_BACK,
    RELATION_INSIDE = RELATION_BACK,
    RELATION_ALIGNED,
    RELATION_REVERSE_ALIGNED,
    RELATION_SPLIT
};
enum VOLUME_TYPE {
    VOLUME_EMPTY,
    VOLUME_SOLID
};
struct csgFace;
struct csgVertex {
    int index; // index to the shape control_point array
    gfxm::vec3 position;
    gfxm::vec3 normal; // Only used for fragments
    gfxm::vec2 uv; // Only used for fragments
    std::set<csgFace*> faces;
    RELATION_TYPE tmp_relation;

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

struct csgLine {
    gfxm::vec3 N;
    gfxm::vec3 P;
};
struct csgMaterial;
struct csgPlane {
    gfxm::vec3 N;
    float D;
    csgMaterial* material = 0;
    gfxm::vec2 uv_scale = gfxm::vec2(1.f, 1.f);
    gfxm::vec2 uv_offset = gfxm::vec2(.0f, .0f);
    std::vector<csgLine> intersection_lines;

    void serializeJson(nlohmann::json& json);
    bool deserializeJson(const nlohmann::json& json);
};
struct csgFragment {
    csgFace* face = 0;
    RELATION_TYPE rel_type;
    VOLUME_TYPE front_volume;
    VOLUME_TYPE back_volume;
    uint32_t rgba = 0xFFFFFFFF;
    gfxm::vec3 N;
    std::vector<csgVertex> vertices;
};
struct csgEdgeTmp {
    csgFace* faces[2];
};
struct csgMaterial {
    int index = -1;
    std::string name;
    std::unique_ptr<gpuTexture2d> texture;
    RHSHARED<gpuMaterial> gpu_material;

    void serializeJson(nlohmann::json& json);
    bool deserializeJson(const nlohmann::json& json);
};
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

class csgScene;
struct csgBrushShape {
    csgScene* scene = 0;
    int index = 0;
    int uid = 0;
    uint32_t rgba = 0xFFFFFFFF;
    gfxm::aabb aabb;
    gfxm::mat4 transform = gfxm::mat4(1.f);
    csgMaterial* material = 0;
    VOLUME_TYPE volume_type = VOLUME_SOLID;
    bool automatic_uv = true;

    std::vector<csgPlane> planes;

    std::vector<gfxm::vec3> world_space_vertices;

    std::vector<std::unique_ptr<csgVertex>> control_points;
    std::vector<std::unique_ptr<csgFace>> faces;

    std::unordered_set<csgBrushShape*> intersecting_shapes;
    std::vector<csgBrushShape*> intersecting_sorted;

    csgVertex* _createControlPoint(const gfxm::vec3& v) {
        csgVertex* cp = new csgVertex;
        cp->position = v;
        cp->index = control_points.size();
        control_points.push_back(std::unique_ptr<csgVertex>(cp));
        return cp;
    }
    csgVertex* _createControlPoint(const csgVertex& v) {
        csgVertex* cp = new csgVertex(v);
        cp->index = control_points.size();
        control_points.push_back(std::unique_ptr<csgVertex>(cp));
        return cp;
    }
    csgVertex* _getControlPoint(int i) {
        return control_points[i].get();
    }
    int _controlPointCount() const {
        return control_points.size();
    }

    void clone(const csgBrushShape* other) {
        aabb = other->aabb;
        transform = other->transform;
        material = other->material;
        volume_type = other->volume_type;
        automatic_uv = other->automatic_uv;
        planes = other->planes;
        world_space_vertices = other->world_space_vertices;
        for (int i = 0; i < other->control_points.size(); ++i) {
            _createControlPoint(other->control_points[i]->position);
        }
        faces.resize(other->faces.size());
        for (int i = 0; i < other->faces.size(); ++i) {
            faces[i].reset(new csgFace);
            auto& face = *faces[i].get();
            face.shape = this;
            for (int j = 0; j < other->faces[i]->control_points.size(); ++j) {
                auto cp = _getControlPoint(other->faces[i]->control_points[j]->index); // Indices should always match
                face.control_points.push_back(cp);
                face.lcl_normals.push_back(other->faces[i]->lcl_normals[j]);
                face.normals.push_back(other->faces[i]->normals[j]);
                face.uvs.push_back(other->faces[i]->uvs[j]);
                cp->faces.insert(&face);
            }
            face.lclN = other->faces[i]->lclN;
            face.lclD = other->faces[i]->lclD;
            /*
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
            */
        }
    }

    void invalidate();
    bool transformFace(int face_id, const gfxm::mat4& transform);
    void setTransform(const gfxm::mat4& transform);

    void serializeJson(nlohmann::json& json);
    bool deserializeJson(const nlohmann::json& json);
};
inline const gfxm::vec3& csgFace::getLocalVertexPos(int i) const { return control_points[i]->position; }
inline const gfxm::vec3& csgFace::getWorldVertexPos(int i) const { return shape->world_space_vertices[control_points[i]->index]; }

class csgScene {
    int next_uid = 0; // replace with uint64_t
    std::unordered_set<csgBrushShape*> invalidated_shapes;
    std::unordered_set<csgBrushShape*> shapes_to_rebuild;

    std::vector<std::unique_ptr<csgBrushShape>> shape_vec;
    std::map<std::string, csgMaterial*> material_map;
    std::vector<std::unique_ptr<csgMaterial>> materials;

    void updateShapeIntersections(csgBrushShape* shape);
public:
    std::unordered_set<csgBrushShape*> shapes;

    void addShape(csgBrushShape* shape);
    void removeShape(csgBrushShape* shape);
    int shapeCount() const;
    csgBrushShape* getShape(int i);

    csgMaterial* createMaterial(const char* name);
    void destroyMaterial(csgMaterial* mat);
    void destroyMaterial(int i);
    csgMaterial* getMaterial(const char* name);
    csgMaterial* getMaterial(int i);

    void invalidateShape(csgBrushShape* shape);
    void update();

    bool castRay(
        const gfxm::vec3& from, const gfxm::vec3& to,
        gfxm::vec3& out_hit, gfxm::vec3& out_normal,
        gfxm::vec3& plane_origin, gfxm::mat3& orient
    );
    bool pickShape(
        const gfxm::vec3& from, const gfxm::vec3& to,
        csgBrushShape** out_shape
    );
    int pickShapeFace(
        const gfxm::vec3& from, const gfxm::vec3& to,
        csgBrushShape* shape, gfxm::vec3* out_pos = 0
    );

    void serializeJson(nlohmann::json& json);
    bool deserializeJson(const nlohmann::json& json);
};


//void csgInit();
//void csgCleanup();

//void csgDraw(const gfxm::mat4& view, const gfxm::mat4& proj);


void csgRecalculatePlanesFromFaceVertices(csgBrushShape* shape, int face_id);

void csgMakeCube(csgBrushShape* shape, float width, float height, float depth, const gfxm::mat4& transform);
void csgMakeBox(csgBrushShape* shape, float width, float height, float depth, const gfxm::mat4& transform);
void csgMakeCylinder(csgBrushShape* shape, float height, float radius, int segments, const gfxm::mat4& transform);
void csgMakeSphere(csgBrushShape* shape, int segments, float radius, const gfxm::mat4& tranform);

struct csgFaceCutData {
    int face_id;
    int va;
    int vb;
    gfxm::vec3 ta;
    gfxm::vec3 tb;
};
struct csgShapeCutData {
    csgBrushShape* shape = 0;
    std::unordered_map<int, csgFaceCutData> cut_faces;
    std::vector<int> discarded_faces;
    std::vector<gfxm::vec3> preview_lines;
    gfxm::vec3 cut_plane_N;
    float cut_plane_D;

    std::vector<csgVertex*> front_control_points;
    std::vector<csgVertex*> back_control_points;
    std::vector<csgVertex*> aligned_control_points;

    void clear() {
        shape = 0;
        cut_faces.clear();
        preview_lines.clear();
        discarded_faces.clear();

        front_control_points.clear();
        back_control_points.clear();
        aligned_control_points.clear();
    }
};
void csgPrepareCut(csgBrushShape* shape, const gfxm::vec3& cut_plane_N, float cut_plane_D, csgShapeCutData& out);
void csgPerformCut(csgShapeCutData& data);


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

void csgMakeShapeTriangles(
    csgBrushShape* shape,
    std::unordered_map<csgMaterial*, csgMeshData>& mesh_data
);
