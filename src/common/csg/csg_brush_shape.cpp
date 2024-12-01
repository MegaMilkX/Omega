#include "csg_brush_shape.hpp"

#include "csg_common.hpp"
#include "csg_scene.hpp"


void csgBrushShape::invalidate() {
    if (scene) {
        scene->invalidateShape(this);
    }
}
bool csgBrushShape::transformFace(int face_id, const gfxm::mat4& transform) {
    auto& face = faces[face_id];

    std::vector<csgVertex> backup_lcl_vertices;
    backup_lcl_vertices.resize(_controlPointCount());
    for (int i = 0; i < _controlPointCount(); ++i) {
        backup_lcl_vertices[i] = *_getControlPoint(i);
    }

    std::vector<gfxm::vec3> backup_world_vertices = world_space_vertices;
    struct FaceDataBackup {
        gfxm::vec3 mid_point;
        gfxm::vec3 lclN;
        gfxm::vec3 N;
        float lclD;
        float D;
    };
    std::unordered_map<csgFace*, FaceDataBackup> faces_to_check;

    gfxm::mat4 shape_inv = gfxm::inverse(this->transform);
    gfxm::mat4 lcl_mid_inv = gfxm::inverse(shape_inv * gfxm::translate(gfxm::mat4(1.f), face->mid_point));
    gfxm::mat4 lcl_tr = shape_inv * transform;
    for (auto cp : face->control_points) {
        gfxm::vec3 p = lcl_mid_inv * gfxm::vec4(cp->position, 1.f);
        p = lcl_tr * gfxm::vec4(p, 1.f);
        cp->position = p;
        world_space_vertices[cp->index] = this->transform * gfxm::vec4(p, 1.f);
        for (auto& f : cp->faces) {
            faces_to_check.insert(std::make_pair(f, FaceDataBackup{ f->mid_point, f->lclN, f->N, f->lclD, f->D }));
        }
    }
    
    // Checking if any affected faces became non-coplanar
    // TODO: Check if any faces became concave
    bool faces_ok = true;
    for (auto kv : faces_to_check) {
        auto face = kv.first;

        gfxm::vec3 lcl_mid_point = gfxm::vec3(0, 0, 0);
        for (int j = 0; j < face->vertexCount(); ++j) {
            auto& v = face->getLocalVertexPos(j);
            lcl_mid_point += v;
        }
        lcl_mid_point /= (float)face->vertexCount();
        face->mid_point = this->transform * gfxm::vec4(lcl_mid_point, 1.f);

        const gfxm::vec3& p0 = face->control_points[0]->position;
        const gfxm::vec3& p1 = face->control_points[1]->position;
        const gfxm::vec3& p2 = face->control_points[2]->position;
        gfxm::vec3 N = gfxm::normalize(gfxm::cross(p1 - p2, p1 - p0));
        if (volume_type == CSG_VOLUME_EMPTY) {
            //N *= -1.f;
        }
        gfxm::vec3 P = lcl_mid_point;
        float D = gfxm::dot(N, P);
        face->lclN = N;
        face->lclD = D;

        gfxm::mat4 transp_inv = gfxm::transpose(gfxm::inverse(this->transform));
        face->N = transp_inv * gfxm::vec4(face->lclN, .0f);
        face->N = gfxm::normalize(face->N);
        gfxm::vec3 lclP = P;
        P = this->transform * gfxm::vec4(lclP, 1.f);
        face->D = gfxm::dot(face->N, P);

        if (!csgCheckVerticesCoplanar(face) || !csgCheckFaceConvex(face)) {
            // TODO
            //faces_ok = false;
            break;
        }
    }

    if (!faces_ok) {
        for (int i = 0; i < _controlPointCount(); ++i) {
            *_getControlPoint(i) = backup_lcl_vertices[i];
        }

        world_space_vertices = backup_world_vertices;
        for (auto& kv : faces_to_check) {
            kv.first->mid_point = kv.second.mid_point;
            kv.first->lclN = kv.second.lclN;
            kv.first->N = kv.second.N;
            kv.first->lclD = kv.second.lclD;
            kv.first->D = kv.second.D;
        }
        return false;
    }


    invalidate();
    return true;
}
void csgBrushShape::setTransform(const gfxm::mat4& transform) {
    // TODO: Optimize
    this->transform = transform;
    csgTransformShape(this, transform);
    csgUpdateShapeWorldSpace(this);
    assert(scene);
    if (!scene) {
        return;
    }
    scene->invalidateShape(this);
}
void csgBrushShape::serializeJson(nlohmann::json& json) {
    type_write_json(json["transform"], transform);
    type_write_json(json["rgba"], rgba);
    json["volume_type"] = (int)volume_type;
    if (material) {
        json["material"] = material->name;
    }
    type_write_json(json["auto_uv"], automatic_uv);

    nlohmann::json& jcontrol_points = json["control_points"];
    jcontrol_points = nlohmann::json::array();
    for (auto& cp : control_points) {
        nlohmann::json jcp;
        type_write_json(jcp["pos"], cp->position);
        type_write_json(jcp["uv"], cp->uv);
        type_write_json(jcp["normal"], cp->normal);
        type_write_json(jcp["index"], cp->index);
        jcontrol_points.push_back(jcp);
    }

    nlohmann::json& jfaces = json["faces"];
    jfaces = nlohmann::json::array();
    for (auto& f : faces) {
        nlohmann::json jface;
        type_write_json(jface["D"], f->lclD);
        type_write_json(jface["N"], f->lclN);
        type_write_json(jface["uv_offset"], f->uv_offset);
        type_write_json(jface["uv_scale"], f->uv_scale);
        type_write_json(jface["normals"], f->lcl_normals);
        type_write_json(jface["uv"], f->uvs);
        if (f->material) {
            jface["material"] = f->material->name;
        }

        nlohmann::json& jcontrol_points = jface["cp"];
        jcontrol_points = nlohmann::json::array();
        for (auto& cp : f->control_points) {
            nlohmann::json jcp = cp->index;
            jcontrol_points.push_back(jcp);
        }
        jfaces.push_back(jface);
    }
}
bool csgBrushShape::deserializeJson(const nlohmann::json& json) {
    type_read_json(json["transform"], transform);
    type_read_json(json["rgba"], rgba);
    volume_type = (CSG_VOLUME_TYPE)json["volume_type"].get<int>();
    if (json.count("material")) {
        const nlohmann::json& jmaterial = json["material"];
        material = scene->getMaterial(jmaterial.get<std::string>().c_str());
    }
    type_read_json(json["auto_uv"], automatic_uv);

    const nlohmann::json& jcontrol_points = json["control_points"];
    if (!jcontrol_points.is_array()) {
        return false;
    }
    int cp_count = jcontrol_points.size();
    control_points.resize(0);
    for (int i = 0; i < cp_count; ++i) {
        const nlohmann::json& jcp = jcontrol_points[i];
        gfxm::vec3 pos;
        type_read_json(jcp["pos"], pos);
        auto cp = _createControlPoint(pos);
        type_read_json(jcp["uv"], cp->uv);
        type_read_json(jcp["normal"], cp->normal);
        //type_read_json(jcp["index"], cp->index);
    }

    const nlohmann::json& jfaces = json["faces"];
    if (!jfaces.is_array()) {
        return false;
    }
    int face_count = jfaces.size();
    faces.resize(face_count);
    for (int i = 0; i < face_count; ++i) {
        const nlohmann::json& jface = jfaces[i];
        faces[i].reset(new csgFace);
        faces[i]->shape = this;
        auto f = faces[i].get();
        type_read_json(jface["D"], f->lclD);
        type_read_json(jface["N"], f->lclN);
        type_read_json(jface["uv_offset"], f->uv_offset);
        type_read_json(jface["uv_scale"], f->uv_scale);
        type_read_json(jface["normals"], f->lcl_normals);
        type_read_json(jface["uv"], f->uvs);
        if (jface.count("material")) {
            const nlohmann::json& jmat = jface["material"];
            f->material = scene->getMaterial(jmat.get<std::string>().c_str());
        }
        f->D = f->lclD;
        f->N = f->lclN;

        const nlohmann::json& jcontrol_points = jface["cp"];
        if (!jcontrol_points.is_array()) {
            return false;
        }
        int cp_count = jcontrol_points.size();
        for (int j = 0; j < cp_count; ++j) {
            int idx = jcontrol_points[j].get<int>();
            control_points[idx]->faces.insert(f);
            f->control_points.push_back(control_points[idx].get());
        }
    }

    csgTransformShape(this, transform);
    csgUpdateShapeWorldSpace(this);
    csgUpdateShapeNormals(this);

    return true;
}
