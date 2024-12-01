#pragma once

#include <memory>
#include <unordered_set>
#include "csg_plane.hpp"
#include "csg_vertex.hpp"
#include "csg_face.hpp"
#include "csg_triangulation.hpp"


class csgScene;
struct csgBrushShape {
    csgScene* scene = 0;
    int index = 0;
    int uid = 0;
    uint32_t rgba = 0xFFFFFFFF;
    gfxm::aabb aabb;
    gfxm::mat4 transform = gfxm::mat4(1.f);
    csgMaterial* material = 0;
    CSG_VOLUME_TYPE volume_type = CSG_VOLUME_SOLID;
    bool automatic_uv = true;

    std::vector<csgPlane> planes;

    std::vector<gfxm::vec3> world_space_vertices;

    std::vector<std::unique_ptr<csgVertex>> control_points;
    std::vector<std::unique_ptr<csgFace>> faces;

    std::unordered_set<csgBrushShape*> intersecting_shapes;
    std::vector<csgBrushShape*> intersecting_sorted;

    std::unique_ptr<csgMeshData> triangulated_mesh;

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
        }
    }

    void invalidate();
    bool transformFace(int face_id, const gfxm::mat4& transform);
    void setTransform(const gfxm::mat4& transform);

    void serializeJson(nlohmann::json& json);
    bool deserializeJson(const nlohmann::json& json);
};

