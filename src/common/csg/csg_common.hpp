#pragma once

#include <unordered_map>
#include <vector>
#include "csg_types.hpp"
#include "math/gfxm.hpp"


struct csgVertex;
struct csgFace;
struct csgFragment;
struct csgBrushShape;

bool csgApproxEqual(float a, float b);

bool csgIntersectAabb(const gfxm::aabb& a, const gfxm::aabb& b);
gfxm::vec2 csgProjectVertexXY(const gfxm::mat3& m, const gfxm::vec3& origin, const gfxm::vec3& v);

bool csgCheckVerticesCoplanar(csgFace* face);
bool csgCheckFaceConvex(csgFace* face);

CSG_RELATION_TYPE csgCheckVertexRelation(const gfxm::vec3& v, const gfxm::vec3& planeN, float planeD);
CSG_RELATION_TYPE csgCheckVertexRelation(const gfxm::vec3& v, csgFace* face);
CSG_RELATION_TYPE csgCheckVertexRelation(csgVertex* v, csgFace* face);
CSG_RELATION_TYPE csgCheckFragmentRelation(csgFragment* frag, csgFace* face);
CSG_RELATION_TYPE csgCheckVertexShapeRelation(csgVertex* v, csgBrushShape* shape);

void csgTransformShape(csgBrushShape* shape, const gfxm::mat4& transform);
void csgUpdateShapeWorldSpace(csgBrushShape* shape);
gfxm::mat3 csgMakeFaceLocalOrientationMatrix(csgFace* face, gfxm::vec3& origin);
gfxm::mat3 csgMakeFaceOrientationMatrix(csgFace* face, gfxm::vec3& origin);

void csgUpdateFaceNormals(csgFace* face, bool smooth = true);
void csgUpdateShapeNormals(csgBrushShape* shape, bool smooth = true);

bool csgMakeVertexLocal(csgFace* FA, csgFace* FB, csgFace* FC, csgVertex* out);
bool csgMakeVertex(csgFace* FA, csgFace* FB, csgFace* FC, csgVertex* out);

struct csgEdgeTmp {
    csgFace* faces[2];
};
bool csgFindEdge(const csgVertex* a, const csgVertex* b, csgEdgeTmp* edge = 0);
void csgFindEdges(csgBrushShape* shape, csgFace* face);
void csgFixWindingLocal(csgBrushShape* shape, csgFace* face);
void csgFixWinding(csgBrushShape* shape, csgFace* face);

void csgShapeInitFacesFromPlanes_(csgBrushShape* shape);


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
