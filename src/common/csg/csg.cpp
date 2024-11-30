#include "csg.hpp"
#include <assert.h>
#include <stdint.h>
#include <unordered_set>

#include "math/intersection.hpp"

#include "gpu/gpu.hpp"
#include "gpu/gpu_util.hpp"
#include "gpu/gpu_buffer.hpp"
#include "gpu/gpu_shader_program.hpp"
#include "gpu/gpu_texture_2d.hpp"

#include "CDT/include/CDT.h"

struct csgData {
    std::unique_ptr<gpuShaderProgram> prog;
    std::unique_ptr<gpuShaderProgram> prog_line;/*
    std::unique_ptr<gpuBuffer> vertex_buffer;
    std::unique_ptr<gpuBuffer> normal_buffer;
    std::unique_ptr<gpuBuffer> color_buffer;
    std::unique_ptr<gpuBuffer> uv_buffer;
    std::unique_ptr<gpuBuffer> index_buffer;
    const gpuMeshShaderBinding* mesh_binding = 0;*/

    struct Mesh {
        csgMaterial* material = 0;
        gpuBuffer vertex_buffer;
        gpuBuffer normal_buffer;
        gpuBuffer color_buffer;
        gpuBuffer uv_buffer;
        gpuBuffer index_buffer;
        std::unique_ptr<gpuMeshDesc> mesh_desc;
        gpuMeshShaderBinding mesh_binding;
    };
    std::vector<std::unique_ptr<Mesh>> meshes;

    std::vector<std::unique_ptr<csgMaterial>> materials;

    struct {
        std::unique_ptr<gpuBuffer> vertex_buffer;
        gpuMeshShaderBinding mesh_binding;
        std::vector<gfxm::vec3> points;
    } point;
    struct {
        std::unique_ptr<gpuBuffer> vertex_buffer;
        gpuMeshShaderBinding mesh_binding;
    } lines;
};
static csgData* csg_data = 0;


bool isVertInsidePlane(
    const gfxm::vec3& v,
    const gfxm::vec3& p0,
    const gfxm::vec3& p1,
    const gfxm::vec3& p2
) {
    gfxm::vec3 cross = gfxm::cross(p1 - p0, p2 - p0);
    { // Check if plane is degenerate (zero area)
        float d = cross.length();
        if (d <= FLT_EPSILON) {
            assert(false);
            return false;
        }
    }

    gfxm::vec3 plane_N = gfxm::normalize(cross);
    gfxm::vec3 pt_dir = v - p0;
    float d = gfxm::dot(pt_dir, plane_N);
    if (d < .0f) {
        return true;
    } else {
        return false;
    }
}
bool intersectEdgeFace(
    const gfxm::vec3& e0,
    const gfxm::vec3& e1,
    const gfxm::vec3& p0,
    const gfxm::vec3& p1,
    const gfxm::vec3& p2,
    const gfxm::vec3& p3,
    gfxm::vec3& result
) {
    gfxm::vec3 cross = gfxm::cross(p1 - p0, p2 - p0);
    { // Check if plane is degenerate (zero area)
        float d = cross.length();
        if (d <= FLT_EPSILON) {
            assert(false);
            return false;
        }
    }
    
    gfxm::vec3 N = gfxm::normalize(cross);

    gfxm::vec3 line_plane_intersection;
    gfxm::vec3 edge_V = (e1 - e0);
    float dot_test = (gfxm::dot(N, edge_V));
    float t;
    if (fabsf(dot_test) > FLT_EPSILON) {
        t = gfxm::dot(N, (p0 - e0) / dot_test);
        line_plane_intersection = e0 + edge_V * t;
        if (t < .0f || t > 1.0f) {
            return false;
        }
    } else {
        return false;
    }

    gfxm::vec3 c0 = gfxm::cross(line_plane_intersection - p0, p1 - p0);
    gfxm::vec3 c1 = gfxm::cross(line_plane_intersection - p1, p2 - p1);
    gfxm::vec3 c2 = gfxm::cross(line_plane_intersection - p2, p3 - p2);
    gfxm::vec3 c3 = gfxm::cross(line_plane_intersection - p3, p0 - p3);
    bool inside = gfxm::dot(c0, N) <= 0 && gfxm::dot(c1, N) <= 0 && gfxm::dot(c2, N) <= 0 && gfxm::dot(c3, N) <= 0;
    if (!inside) {
        return false;
    }

    result = line_plane_intersection;
    /*
    if (dot_test < .0f) {
        rhp.normal = N;
        rhp.distance = gfxm::length(ray.direction) * t;
    } else { // backface
        rhp.normal = -N;
        rhp.distance = gfxm::length(ray.direction) * t;
    }*/
    return true;
}
bool isPointOnLineSegment(
    const gfxm::vec3& P,
    const gfxm::vec3& A,
    const gfxm::vec3& B,
    float& t
) {
    gfxm::vec3 cross = gfxm::cross(B - A, P - A);
    float cross_len = cross.length();
    const float EPSILON = 0.000001f;
    if (cross_len > EPSILON) {
        return false;
    }
    float d = gfxm::dot(B - A, P - A);
    // Change to d < .0f to allow matching with the very end of the line segment
    if (d < FLT_EPSILON) {
        return false;
    }
    // Remove -FLT_EPSILON to allow matching with the very end of the line segment
    if (d > (B - A).length2() - FLT_EPSILON) {
        return false;
    }

    t = gfxm::sqrt(d);
    return true;
}


struct FacePairKey {
    uint64_t shape_a;
    uint64_t shape_b;
    int face_a;
    int face_b;

    FacePairKey() {}
    FacePairKey(uint64_t shape_a, uint64_t shape_b, int face_a, int face_b)
        : shape_a(shape_a),
        shape_b(shape_b),
        face_a(face_a),
        face_b(face_b) {
        if (shape_a > shape_b) {
            std::swap(this->shape_a, this->shape_b);
            std::swap(this->face_a, this->face_b);
        }
    }
    bool operator==(const FacePairKey& other) const {
        return shape_a == other.shape_a
            && shape_b == other.shape_b
            && face_a == other.face_a
            && face_b == other.face_b;
    }
};
template<>
struct std::hash<FacePairKey> {
    std::size_t operator()(const FacePairKey& k) const {
        return std::hash<uint64_t>()(k.shape_a)
            ^ std::hash<uint64_t>()(k.shape_b)
            ^ std::hash<uint64_t>()(k.face_a)
            ^ std::hash<uint64_t>()(k.face_b);
    }
};

struct FaceKey {
    uint64_t shape;
    int face;

    FaceKey() {}
    FaceKey(uint64_t shape, int face)
        : shape(shape), face(face) {}
    bool operator==(const FaceKey& other) const {
        return shape == other.shape
            && face == other.face;
    }
};
template<>
struct std::hash<FaceKey> {
    std::size_t operator()(const FaceKey& k) const {
        return std::hash<uint64_t>()(k.shape)
            ^ std::hash<uint64_t>()(k.face);
    }
};

bool intersectAabb(const gfxm::aabb& a, const gfxm::aabb& b) {
    return a.from.x < b.to.x &&
        a.from.y < b.to.y &&
        a.from.z < b.to.z &&
        b.from.x < a.to.x &&
        b.from.y < a.to.y &&
        b.from.z < a.to.z;
}


void csgRebuildFragments(csgBrushShape* shape);
void csgShapeInitFacesFromPlanes_(csgBrushShape* shape);
void csgUpdateShapeWorldSpace(csgBrushShape* shape);
void csgTransformShape(csgBrushShape* shape, const gfxm::mat4& transform);
RELATION_TYPE csgCheckVertexRelation(const gfxm::vec3& v, csgFace* face);
void csgUpdateFaceNormals(csgFace* face);
void csgUpdateShapeNormals(csgBrushShape* shape);
gfxm::mat3 csgMakeFaceLocalOrientationMatrix(csgFace* face, gfxm::vec3& origin);
gfxm::mat3 csgMakeFaceOrientationMatrix(csgFace* face, gfxm::vec3& origin);

gfxm::vec2 projectVertexXY(const gfxm::mat3& m, const gfxm::vec3& origin, const gfxm::vec3& v) {
    gfxm::vec2 v2d;
    v2d.x = gfxm::dot(m[0], v - origin);
    v2d.y = gfxm::dot(m[1], v - origin);
    return v2d;
}

bool checkVerticesCoplanar(csgFace* face) {
    assert(face->control_points.size() >= 3);
    for (auto cp : face->control_points) {
        if (RELATION_ALIGNED != csgCheckVertexRelation(face->shape->world_space_vertices[cp->index], face)) {
            return false;
        }
    }
    return true;
}
bool checkFaceConvex(csgFace* face) {
    assert(face->control_points.size() >= 3);
    float sign = .0f;
    for (int i = 0; i < face->control_points.size(); ++i) {
        int a = i;
        int b = (i + 1) % face->control_points.size();
        int c = (i + 2) % face->control_points.size();
        gfxm::vec3 p0 = face->control_points[a]->position;
        gfxm::vec3 p1 = face->control_points[b]->position;
        gfxm::vec3 p2 = face->control_points[c]->position;
        gfxm::vec3 C = gfxm::cross(p2 - p1, p1 - p0);
        
        float d = gfxm::dot(face->N, C);/*
        if (fabsf(d) <= FLT_EPSILON) {
            // Colinear edges are not allowed too
            return false;
        }*/
        float s = d > .0f ? 1.f : -1.f;
        if (s * sign < .0f) {
            return false;
        } else {
            sign = s;
        }
    }
    return true;
}


void csgPlane::serializeJson(nlohmann::json& json) {
    type_write_json(json["N"], N);
    type_write_json(json["D"], D);
    type_write_json(json["uv_scale"], uv_scale);
    type_write_json(json["uv_offset"], uv_offset);
}
bool csgPlane::deserializeJson(const nlohmann::json& json) {
    type_read_json(json["N"], N);
    type_read_json(json["D"], D);
    type_read_json(json["uv_scale"], uv_scale);
    type_read_json(json["uv_offset"], uv_offset);
    return true;
}


void csgMaterial::serializeJson(nlohmann::json& json) {
    type_write_json(json["name"], name);
    type_write_json(json["render_material"], gpu_material);
}
bool csgMaterial::deserializeJson(const nlohmann::json& json) {
    type_read_json(json["name"], name);
    type_read_json(json["render_material"], gpu_material);
    return true;
}

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
        if (volume_type == VOLUME_EMPTY) {
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

        if (!checkVerticesCoplanar(face) || !checkFaceConvex(face)) {
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
    volume_type = (VOLUME_TYPE)json["volume_type"].get<int>();
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



void csgScene::updateShapeIntersections(csgBrushShape* shape) {
    std::unordered_set<csgBrushShape*> diff;
    std::unordered_set<csgBrushShape*> new_intersections;
    for (auto other : shapes) {
        if (other == shape) {
            continue;
        }
        if (intersectAabb(shape->aabb, other->aabb)) {
            new_intersections.insert(other);
        }
    }
    
    // Remove this shape from shapes that are no longer touching it
    for (auto s : shape->intersecting_shapes) {
        if (!new_intersections.count(s)) {
            diff.insert(s);
        }
    }
    for (auto d : diff) {
        d->intersecting_shapes.erase(shape);
    }

    // Add this shape to shapes that are now touching it
    diff.clear();
    for (auto s : new_intersections) {
        if (!shape->intersecting_shapes.count(s)) {
            diff.insert(s);
        }
    }
    for (auto d : diff) {
        d->intersecting_shapes.insert(shape);
    }

    shape->intersecting_shapes = new_intersections;
}

void csgScene::addShape(csgBrushShape* shape) {
    if (shapes.count(shape)) {
        assert(false);
        return;
    }
    shape->scene = this;
    shape->uid = next_uid++;
    invalidated_shapes.insert(shape);
    shapes.insert(shape);
    shape->index = shape_vec.size();
    shape_vec.push_back(std::unique_ptr<csgBrushShape>(shape));
}
void csgScene::removeShape(csgBrushShape* shape) {
    if (shapes.count(shape) == 0) {
        return;
    }
    for (auto s : shape->intersecting_shapes) {
        s->intersecting_shapes.erase(shape);
        invalidated_shapes.insert(s);
    }
    shape->scene = 0;
    invalidated_shapes.erase(shape);
    shapes_to_rebuild.erase(shape);
    shapes.erase(shape);
    shape_vec.erase(shape_vec.begin() + shape->index);
    for (int i = 0; i < shape_vec.size(); ++i) {
        shape_vec[i]->index = i;
    }
}
int csgScene::shapeCount() const {
    return shape_vec.size();
}
csgBrushShape* csgScene::getShape(int i) {
    return shape_vec[i].get();
}
csgMaterial* csgScene::createMaterial(const char* name) {
    auto it = material_map.find(name);
    if (it != material_map.end()) {
        return it->second;
    }
    csgMaterial* m = new csgMaterial;
    m->name = name;
    LOG("Material: " << name);
    m->index = materials.size();
    m->gpu_material = resGet<gpuMaterial>(name);
    material_map[name] = m;
    materials.push_back(std::unique_ptr<csgMaterial>(m));
    return m;
}
void csgScene::destroyMaterial(csgMaterial* mat) {
    if (mat->index < 0) {
        assert(false);
        return;
    }

    for (auto& s : shape_vec) {
        if (s->material == mat) {
            s->material = 0;
        }
        for (auto& f : s->faces) {
            if (f->material == mat) {
                f->material = 0;
            }
        }
    }

    material_map.erase(mat->name);
    materials.erase(materials.begin() + mat->index);
    for (int i = 0; i < materials.size(); ++i) {
        materials[i]->index = i;
    }
}
void csgScene::destroyMaterial(int i) {
    auto mat = materials[i].get();

    for (auto& s : shape_vec) {
        if (s->material == mat) {
            s->material = 0;
        }
        for (auto& f : s->faces) {
            if (f->material == mat) {
                f->material = 0;
            }
        }
    }

    material_map.erase(materials[i]->name);
    materials.erase(materials.begin() + i);
    for (int i = 0; i < materials.size(); ++i) {
        materials[i]->index = i;
    }
}
csgMaterial* csgScene::getMaterial(const char* name) {
    auto it = material_map.find(name);
    if (it == material_map.end()) {
        return 0;
    }
    return it->second;
}
csgMaterial* csgScene::getMaterial(int i) {
    return materials[i].get();
}
void csgScene::invalidateShape(csgBrushShape* shape) {
    invalidated_shapes.insert(shape);/*
    for (auto s : shape->intersecting_shapes) {
        shapes_to_rebuild.insert(s);
    }*/
}
void csgScene::update() {
    // For all new or moved shapes
    for (auto shape : invalidated_shapes) {
        csgUpdateShapeWorldSpace(shape);

        shapes_to_rebuild.insert(shape);
        for (auto intersecting : shape->intersecting_shapes) {
            shapes_to_rebuild.insert(intersecting);
        }

        // Find new intersecting shapes
        updateShapeIntersections(shape);
        for (auto intersecting : shape->intersecting_shapes) {
            shapes_to_rebuild.insert(intersecting);
        }
    }
    invalidated_shapes.clear();

    std::vector<csgBrushShape*> shape_vec;
    shape_vec.insert(shape_vec.end(), shapes_to_rebuild.begin(), shapes_to_rebuild.end());
    std::sort(shape_vec.begin(), shape_vec.end(), [](const csgBrushShape* a, const csgBrushShape* b)->bool {
        return a->uid < b->uid;
    });
    for (auto shape : shape_vec) {
        shape->intersecting_sorted.clear();
        shape->intersecting_sorted.insert(
            shape->intersecting_sorted.end(), shape->intersecting_shapes.begin(), shape->intersecting_shapes.end()
        );
        std::sort(shape->intersecting_sorted.begin(), shape->intersecting_sorted.end(), [](const csgBrushShape* a, const csgBrushShape* b)->bool {
            return a->uid < b->uid;
        });
    }
    for (auto shape : shape_vec) {
        csgRebuildFragments(shape);
    }
    shapes_to_rebuild.clear();
}
static bool isPointInsideConvexFace(const gfxm::vec3& pt, const std::vector<csgVertex>& vertices) {
    int n = vertices.size();
    if (n < 3) {
        assert(false);
        return false;
    }
    gfxm::vec3 v0 = vertices[0].position;
    gfxm::vec3 v1 = vertices[1].position;
    gfxm::vec3 normal = gfxm::cross(v1 - v0, pt - v0);

    for (int i = 1; i < n; ++i) {
        int j = (i + 1) % n;
        gfxm::vec3 vi = vertices[i].position;
        gfxm::vec3 vj = vertices[j].position;
        if (gfxm::dot(normal, gfxm::cross(vj - vi, pt - vi)) < -FLT_EPSILON) {
            return false;
        }
    }
    return true;
}
static bool isPointInsideConvexFace(const gfxm::vec3& pt, const csgFace* face) {
    int n = face->vertexCount();
    if (n < 3) {
        assert(false);
        return false;
    }
    gfxm::vec3 v0 = face->getWorldVertexPos(0);
    gfxm::vec3 v1 = face->getWorldVertexPos(1);
    gfxm::vec3 normal = gfxm::cross(v1 - v0, pt - v0);

    for (int i = 1; i < n; ++i) {
        int j = (i + 1) % n;
        gfxm::vec3 vi = face->getWorldVertexPos(i);
        gfxm::vec3 vj = face->getWorldVertexPos(j);
        if (gfxm::dot(normal, gfxm::cross(vj - vi, pt - vi)) < -FLT_EPSILON) {
            return false;
        }
    }
    return true;
}
bool csgScene::castRay(
    const gfxm::vec3& from, const gfxm::vec3& to,
    gfxm::vec3& out_hit, gfxm::vec3& out_normal,
    gfxm::vec3& plane_origin, gfxm::mat3& orient
) {
    float dist = INFINITY;
    gfxm::vec3 pt;
    const csgBrushShape* hit_shape = 0;
    const csgFace* hit_face = 0;
    const csgFragment* hit_fragment = 0;

    for (auto& shape : shapes) {
        const auto& aabb = shape->aabb;
        if (!gfxm::intersect_ray_aabb(gfxm::ray(from, (to - from)), aabb)) {
            continue;
        }
        for (int i = 0; i < shape->faces.size(); ++i) {
            const auto& face = *shape->faces[i].get();
            float t = .0f;
            if (!gfxm::intersect_ray_plane_t(from, to, face.N, face.D, t)) {
                continue;
            }
            if (t >= dist) {
                continue;
            }
            gfxm::vec3 tmp_pt = from + (to - from) * t;
            if (!isPointInsideConvexFace(tmp_pt, &face)) {
                continue;
            }

            for (int j = 0; j < face.fragments.size(); ++j) {
                const auto& frag = face.fragments[j];
                if (frag.back_volume == frag.front_volume) {
                    continue;
                }
                gfxm::vec3 fragN = face.N * (frag.back_volume == VOLUME_EMPTY ? -1.f : 1.f);
                if (gfxm::dot(fragN, to - from) > .0f) {
                    continue;
                }
                if (!isPointInsideConvexFace(tmp_pt, frag.vertices)) {
                    continue;
                }

                dist = t;
                out_hit = tmp_pt;
                out_normal = fragN;
                hit_shape = shape;
                hit_face = &face;
                hit_fragment = &frag;
            }
        }
    }
    if (dist == INFINITY) {
        return false;
    }
    
    // Find a face vertex closest to the intersection
    // Use that vertex as a plane origin
    {
        int closest_pt_idx = 0;
        float closest_dist = FLT_MAX;
        for (int k = 0; k < hit_face->vertexCount(); ++k) {
            float dist = (hit_face->getWorldVertexPos(k) - out_hit).length2();
            if (closest_dist > dist) {
                closest_dist = dist;
                closest_pt_idx = k;
            }
        }
        plane_origin = hit_face->getWorldVertexPos(closest_pt_idx);
    }

    // Find a face edge closest to the intersection
    // Make an orientation matrix based on that edge and the normal of the face
    {
        int closest_edge_idx = 0;
        float closest_dist = FLT_MAX;
        gfxm::vec3 projected_point = hit_face->getWorldVertexPos(0);
        for (int k = 0; k < hit_face->vertexCount(); ++k) {
            gfxm::vec3 A = hit_face->getWorldVertexPos(k);
            gfxm::vec3 B = hit_face->getWorldVertexPos((k + 1) % hit_face->vertexCount());
            gfxm::vec3 AB = B - A;
            gfxm::vec3 AP = out_hit - A;
            gfxm::vec3 pt = A + gfxm::dot(AP, AB) / gfxm::dot(AB, AB) * AB;
            float dist = (pt - out_hit).length2();
            if (closest_dist > dist) {
                closest_dist = dist;
                closest_edge_idx = k;
                projected_point = pt;
            }
        }
        
        const gfxm::vec3 A = hit_face->getWorldVertexPos(closest_edge_idx);
        const gfxm::vec3 B = hit_face->getWorldVertexPos((closest_edge_idx + 1) % hit_face->vertexCount());
        const gfxm::vec3 P = out_hit;
        gfxm::vec3 right = gfxm::normalize(B - A);
        gfxm::vec3 back = hit_face->N;
        gfxm::vec3 up = gfxm::cross(back, right);
        orient[0] = right;
        orient[1] = up;
        orient[2] = back;
    }

    return true;
}

bool csgScene::pickShape(const gfxm::vec3& from, const gfxm::vec3& to, csgBrushShape** out_shape) {
    float dist = INFINITY;
    gfxm::vec3 pt;
    for (auto& shape : shapes) {
        const auto& aabb = shape->aabb;
        if (!gfxm::intersect_ray_aabb(gfxm::ray(from, (to - from)), aabb)) {
            continue;
        }
        for (int i = 0; i < shape->faces.size(); ++i) {
            const auto& face = *shape->faces[i].get();
            float t = .0f;
            if (!gfxm::intersect_ray_plane_t(from, to, face.N, face.D, t)) {
                continue;
            }
            if (t >= dist) {
                continue;
            }
            gfxm::vec3 tmp_pt = from + (to - from) * t;
            if (!isPointInsideConvexFace(tmp_pt, &face)) {
                continue;
            }

            for (int j = 0; j < face.fragments.size(); ++j) {
                const auto& frag = face.fragments[j];
                if (frag.back_volume == frag.front_volume) {
                    continue;
                }
                gfxm::vec3 fragN = face.N * (frag.back_volume == VOLUME_EMPTY ? -1.f : 1.f);
                if (gfxm::dot(fragN, to - from) > .0f) {
                    continue;
                }
                if (!isPointInsideConvexFace(tmp_pt, frag.vertices)) {
                    continue;
                }

                dist = t;
                *out_shape = shape;
            }
        }
    }
    if (dist == INFINITY) {
        return false;
    }
    return true;
}
int csgScene::pickShapeFace(const gfxm::vec3& from, const gfxm::vec3& to, csgBrushShape* shape, gfxm::vec3* out_pos) {
    float dist = INFINITY;
    gfxm::vec3 pt;
    int face_id = -1;
    for (int i = 0; i < shape->faces.size(); ++i) {
        const auto& face = *shape->faces[i].get();
        float t = .0f;
        if (!gfxm::intersect_ray_plane_t(from, to, face.N, face.D, t)) {
            continue;
        }
        if (t >= dist) {
            continue;
        }

        gfxm::vec3 N = shape->faces[i]->N * (shape->volume_type == VOLUME_EMPTY ? -1.f : 1.f);
        if (gfxm::dot(N, to - from) > .0f) {
            continue;
        }

        gfxm::vec3 tmp_pt = from + (to - from) * t;
        if (!isPointInsideConvexFace(tmp_pt, &face)) {
            continue;
        }
        dist = t;
        face_id = i;
        if (out_pos) {
            *out_pos = from + (to - from) * t;
        }
    }
    return face_id;
}

void csgScene::serializeJson(nlohmann::json& json) {
    nlohmann::json& jmaterials = json["materials"];
    jmaterials = nlohmann::json::array();
    for (auto& m : materials) {
        nlohmann::json jmat;
        m->serializeJson(jmat);
        jmaterials.push_back(jmat);
    }
    nlohmann::json& jshapes = json["shapes"];
    jshapes = nlohmann::json::array();
    for (auto& shape : shape_vec) {
        nlohmann::json jshape;
        shape->serializeJson(jshape);
        jshapes.push_back(jshape);
    }
}
bool csgScene::deserializeJson(const nlohmann::json& json) {
    shapes.clear();
    invalidated_shapes.clear();
    shapes_to_rebuild.clear();
    shape_vec.clear();
    materials.clear();
    material_map.clear();

    const nlohmann::json& jmaterials = json["materials"];
    if (!jmaterials.is_array()) {
        return false;
    }
    int mat_count = jmaterials.size();
    for (int i = 0; i < mat_count; ++i) {
        const nlohmann::json& jmat = jmaterials[i];
        std::string name;
        type_read_json(jmat["name"], name);
        auto mat = createMaterial(name.c_str());
        mat->deserializeJson(jmat);
    }

    const nlohmann::json& jshapes = json["shapes"];
    if (!jshapes.is_array()) {
        return false;
    }
    int shape_count = jshapes.size();
    for (int i = 0; i < shape_count; ++i) {
        const nlohmann::json& jshape = jshapes[i];
        // TODO: Shape ownership
        csgBrushShape* shape = new csgBrushShape;
        shape->scene = this;
        shape->deserializeJson(jshape);
        addShape(shape);
    }
    return true;
}


uint32_t makeColor32(float R, float G, float B, float A) {
    uint32_t rc = std::min(255, int(255 * R));
    uint32_t gc = std::min(255, int(255 * G));
    uint32_t bc = std::min(255, int(255 * B));
    uint32_t ac = std::min(255, int(255 * A));
    uint32_t C = 0;
    C |= rc;
    C |= gc << 8;
    C |= bc << 16;
    C |= ac << 24;
    return C;
}

bool intersectPlanePlane(const csgPlane& A, const csgPlane& B, csgLine& out) {
    float d = gfxm::dot(A.N, B.N);
    if (fabsf(d) >= 1.f - FLT_EPSILON) {
        return false;
    }

    gfxm::vec3 CN = gfxm::cross(A.N, B.N);
    float det = CN.length2();
    gfxm::vec3 P = (gfxm::cross(B.N, CN) * A.D + gfxm::cross(CN, A.N) * B.D) / det;
    out.N = CN;
    out.P = P;
    return true;
}
bool intersectLinePlane(const csgLine& L, const csgPlane& P, gfxm::vec3& out) {
    float denom = gfxm::dot(P.N, L.N);
    if (abs(denom) <= 0.000001f) {
        return false;
    }
    gfxm::vec3 vec = P.N * P.D - L.P;
    float t = gfxm::dot(vec, P.N) / denom;
    out = L.P + L.N * t;
    return true;
}
bool makeVertexLocal(csgFace* FA, csgFace* FB, csgFace* FC, csgVertex* out) {
    std::array<csgFace*, 3> faces = { FA, FB, FC };
    std::sort(faces.begin(), faces.end());
    FA = FA;
    FB = FB;
    FC = FC;

    float d0 = gfxm::dot(FA->lclN, FB->lclN);
    float d1 = gfxm::dot(FA->lclN, FC->lclN);
    float d2 = gfxm::dot(FB->lclN, FC->lclN);
    if (fabsf(d0) >= 1.f
        || fabsf(d1) >= 1.f
        || fabsf(d2) >= 1.f
    ) {
        return false;
    }

    gfxm::vec3 lineN = gfxm::cross(FA->lclN, FB->lclN);
    float det = lineN.length2();
    gfxm::vec3 lineP = (gfxm::cross(FB->lclN, lineN) * FA->lclD + gfxm::cross(lineN, FA->lclN) * FB->lclD) / det;

    // C plane with intersection line intersection
    float denom = gfxm::dot(FC->lclN, lineN);
    if (abs(denom) <= 0.00000001f) {
        return false;
    }
    gfxm::vec3 vec = FC->lclN * FC->lclD - lineP;
    float t = gfxm::dot(vec, FC->lclN) / denom;
    out->position = lineP + lineN * t;
    out->faces.insert(FA);
    out->faces.insert(FB);
    out->faces.insert(FC);
    return true;
}
bool makeVertex(csgFace* FA, csgFace* FB, csgFace* FC, csgVertex* out) {
    std::array<csgFace*, 3> faces = { FA, FB, FC };
    std::sort(faces.begin(), faces.end());
    FA = FA;
    FB = FB;
    FC = FC;
    //csgPlane* A = FA->plane;
    //csgPlane* B = FB->plane;
    //csgPlane* C = FC->plane;

    float d0 = gfxm::dot(FA->N, FB->N);
    float d1 = gfxm::dot(FA->N, FC->N);
    float d2 = gfxm::dot(FB->N, FC->N);
    if (fabsf(d0) >= 1.f
        || fabsf(d1) >= 1.f
        || fabsf(d2) >= 1.f
    ) {
        return false;
    }

    gfxm::vec3 lineN = gfxm::cross(FA->N, FB->N);
    float det = lineN.length2();
    gfxm::vec3 lineP = (gfxm::cross(FB->N, lineN) * FA->D + gfxm::cross(lineN, FA->N) * FB->D) / det;

    // C plane with intersection line intersection
    float denom = gfxm::dot(FC->N, lineN);
    if (abs(denom) <= 0.00000001f) {
        return false;
    }
    gfxm::vec3 vec = FC->N * FC->D - lineP;
    float t = gfxm::dot(vec, FC->N) / denom;
    out->position = lineP + lineN * t;
    out->faces.insert(FA);
    out->faces.insert(FB);
    out->faces.insert(FC);
    return true;
}
bool isVertexInsidePlanes(const csgPlane* planes, int count, const gfxm::vec3& v) {
    for(int i = 0; i < count; ++i) {/*
        float d = gfxm::dot(gfxm::vec4(planes[i].N, planes[i].D), gfxm::vec4(v, 1.f));
        if (d > FLT_EPSILON) {
            return false;
        }*/
        
        gfxm::vec3 planePoint = planes[i].N * planes[i].D;
        gfxm::vec3 pt_dir = v - planePoint;
        float d = gfxm::dot(gfxm::normalize(pt_dir), planes[i].N);
        if (d > 0.f) {
            return false;
        }
    }
    return true;
}

bool findEdge(const csgVertex* a, const csgVertex* b, csgEdgeTmp* edge = 0) {
    std::vector<csgFace*> faces;
    auto it = std::set_intersection(
        std::begin(a->faces), std::end(a->faces),
        std::begin(b->faces), std::end(b->faces),
        std::back_inserter(faces)
    );
    
    if (faces.size() >= 2) {
        if (edge) {
            *edge = csgEdgeTmp{ { faces[0], faces[1] } };
        }
        return true;
    } else {
        return false;
    }
}
void findEdges(csgBrushShape* shape, csgFace* face) {
    face->control_points.clear();
    std::set<csgVertex*> unsorted = std::move(face->tmp_control_points);
    auto it = unsorted.begin();
    while (it != unsorted.end()) {
        csgVertex* vert = *it;
        face->control_points.push_back(vert);
        unsorted.erase(it);
        it = std::find_if(unsorted.begin(), unsorted.end(), [shape, &vert](csgVertex* other)->bool {
            return findEdge(vert, other);
        });
    }
    /*
    face->indices.clear();
    std::set<int> unsorted_indices = std::move(face->tmp_indices);
    auto it = unsorted_indices.begin();
    while (it != unsorted_indices.end()) {
        int idx = *it;
        face->indices.push_back(idx);
        unsorted_indices.erase(it);
        it = std::find_if(unsorted_indices.begin(), unsorted_indices.end(), [shape, &idx](const int& other)->bool {
            return findEdge(shape->control_points[idx].get(), shape->control_points[other].get());
        });
    }*/
}/*
void findEdges(csgFace* face) {
    std::vector<csgVertex> unsorted = std::move(face->vertices);
    auto it = unsorted.begin();
    while (it != unsorted.end()) {
        csgVertex v = *it;
        face->vertices.push_back(v);
        unsorted.erase(it);
        it = std::find_if(unsorted.begin(), unsorted.end(), [&v](const csgVertex& other)->bool {
            return findEdge(&v, &other);
        });
    }
}*/
void fixWindingLocal(csgBrushShape* shape, csgFace* face) {
    assert(face->vertexCount() >= 3);
    gfxm::vec3 cross;
    const auto& p0 = face->control_points[0]->position;
    const auto& p1 = face->control_points[1]->position;
    for (int i = 2; i < face->control_points.size(); ++i) {
        const auto& p2 = face->control_points[i]->position;
        cross = gfxm::cross(p1 - p0, p2 - p0);
        if (cross.length2() > FLT_EPSILON) {
            break;
        }
    }
    float d = gfxm::dot(face->lclN, cross);
    if (d < .0f) {
        std::reverse(face->control_points.begin(), face->control_points.end());
    }
    return;
    /*
    assert(face->vertexCount() >= 3);
    const auto& p0 = shape->control_points[face->indices[0]]->position;
    const auto& p1 = shape->control_points[face->indices[1]]->position;
    const auto& p2 = shape->control_points[face->indices[2]]->position;
    gfxm::vec3 cross = gfxm::cross(p1 - p0, p2 - p0);
    float d = gfxm::dot(face->lclN, cross);
    if (d < .0f) {
        std::reverse(face->indices.begin(), face->indices.end());
    }
    return;*/
}
void fixWinding(csgBrushShape* shape, csgFace* face) {
    // TODO: This only works when creating the shape from planes, since face->N is same as face->lclN at that point
    assert(face->vertexCount() >= 3);
    gfxm::vec3 cross;
    const auto& p0 = face->control_points[0]->position;
    const auto& p1 = face->control_points[1]->position;
    for (int i = 2; i < face->control_points.size(); ++i) {
        const auto& p2 = face->control_points[i]->position;
        cross = gfxm::cross(p1 - p0, p2 - p0);
        if (cross.length2() > FLT_EPSILON) {
            break;
        }
    }
    float d = gfxm::dot(face->N, gfxm::normalize(cross));
    if (d < .0f) {
        std::reverse(face->control_points.begin(), face->control_points.end());
    }
    return;
    /*
    assert(face->vertexCount() >= 3);
    const auto& p0 = shape->control_points[face->indices[0]]->position;
    const auto& p1 = shape->control_points[face->indices[1]]->position;
    const auto& p2 = shape->control_points[face->indices[2]]->position;
    gfxm::vec3 cross = gfxm::cross(p1 - p0, p2 - p0);
    float d = gfxm::dot(face->N, cross);
    if (d < .0f) {
        std::reverse(face->indices.begin(), face->indices.end());
    }
    return;*/
}

bool approxEqual(float a, float b) {
    return int(roundf(a * 1000)) == int(roundf(b * 1000));
}

RELATION_TYPE csgCheckVertexRelation(const gfxm::vec3& v, const gfxm::vec3& planeN, float planeD) {
    //float d = gfxm::dot(gfxm::vec4(face->plane->N, face->plane->D), gfxm::vec4(v->position, 1.f));
    
    gfxm::vec3 planePoint = planeN * planeD;
    gfxm::vec3 pt_dir = v - planePoint;
    float d = gfxm::dot(gfxm::normalize(pt_dir), planeN);

    //float d = gfxm::dot(face->plane->N, v->position) + face->plane->D;

    if (approxEqual(d, .0f)) {
        return RELATION_ALIGNED;
    } else if (d > .0f) {
        return RELATION_FRONT;
    } else {
        return RELATION_BACK;
    }
}
RELATION_TYPE csgCheckVertexRelation(const gfxm::vec3& v, csgFace* face) {
    //float d = gfxm::dot(gfxm::vec4(face->plane->N, face->plane->D), gfxm::vec4(v->position, 1.f));
    
    gfxm::vec3 planePoint = face->N * face->D;
    gfxm::vec3 pt_dir = v - planePoint;
    float d = gfxm::dot(gfxm::normalize(pt_dir), face->N);

    //float d = gfxm::dot(face->plane->N, v->position) + face->plane->D;

    if (approxEqual(d, .0f)) {
        return RELATION_ALIGNED;
    } else if (d > .0f) {
        return RELATION_FRONT;
    } else {
        return RELATION_BACK;
    }
}
RELATION_TYPE csgCheckVertexRelation(csgVertex* v, csgFace* face) {
    //float d = gfxm::dot(gfxm::vec4(face->plane->N, face->plane->D), gfxm::vec4(v->position, 1.f));
    
    gfxm::vec3 planePoint = face->N * face->D;
    gfxm::vec3 pt_dir = v->position - planePoint;
    float d = gfxm::dot(gfxm::normalize(pt_dir), face->N);

    //float d = gfxm::dot(face->plane->N, v->position) + face->plane->D;

    if (approxEqual(d, .0f)) {
        return RELATION_ALIGNED;
    } else if (d > .0f) {
        return RELATION_FRONT;
    } else {
        return RELATION_BACK;
    }
}
RELATION_TYPE csgCheckFragmentRelation(csgFragment* frag, csgFace* face) {
    std::unordered_map<RELATION_TYPE, int> count;
    count[RELATION_INSIDE] = 0;
    count[RELATION_ALIGNED] = 0;
    count[RELATION_OUTSIDE] = 0;
    for (csgVertex& v : frag->vertices) {
        count[csgCheckVertexRelation(&v, face)] += 1;
    }
    if (count[RELATION_OUTSIDE] > 0 &&
        count[RELATION_INSIDE] > 0) {
        return RELATION_SPLIT;
    }
    else if (count[RELATION_OUTSIDE] == 0 && count[RELATION_INSIDE] == 0) {
        float d = gfxm::dot(face->N, frag->face->N);
        if (d < .0f) {
            return RELATION_REVERSE_ALIGNED;
        }
        else {
            return RELATION_ALIGNED;
        }
    }
    else if (count[RELATION_INSIDE] > 0) {
        return RELATION_INSIDE;
    }
    else {
        return RELATION_OUTSIDE;
    }
}
RELATION_TYPE csgCheckVertexShapeRelation(csgVertex* v, csgBrushShape* shape) {
    int n = shape->faces.size();
    RELATION_TYPE rel = RELATION_INSIDE;
    for (int i = 0; i < n; ++i) {
        switch (csgCheckVertexRelation(v, shape->faces[i].get())) {
        case RELATION_FRONT:
            return RELATION_OUTSIDE;
        case RELATION_ALIGNED:
            rel = RELATION_ALIGNED;
        }
    }
    return rel;
}

void csgShapeInitFacesFromPlanes2(csgBrushShape* shape) {
    auto compareVertices = [](const gfxm::vec3& a, const gfxm::vec3& b)->bool {
        const float EPS = .001f;
        const float INV_EPS = 1.f / EPS;
        const int ix = round(a.x * INV_EPS);
        const int iy = round(a.y * INV_EPS);
        const int iz = round(a.z * INV_EPS);
        const int oix = round(b.x * INV_EPS);
        const int oiy = round(b.y * INV_EPS);
        const int oiz = round(b.z * INV_EPS);
        return ix == oix && iy == oiy && iz == oiz;
    };
    auto makeVertexHash = [](const gfxm::vec3& v)->uint64_t {
        const float EPS = .001f;
        const float INV_EPS = 1.f / EPS;
        uint64_t h = 0x778abe;
        h ^= std::hash<int>()(round((v.x + .5f) * INV_EPS)) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<int>()(round((v.y + .5f) * INV_EPS)) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<int>()(round((v.z + .5f) * INV_EPS)) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    };

    shape->control_points.clear();
    shape->faces.clear();

    struct Plane {
        gfxm::vec3 N;
        float D;
    };
    struct Edge {
        gfxm::vec3 P;
        gfxm::vec3 N;
        float ta = -INFINITY;
        float tb = INFINITY;
        bool discard = false;
    };

    std::vector<Plane> planes;
    planes.resize(shape->planes.size());
    for (int i = 0; i < shape->planes.size(); ++i) {
        auto& p = shape->planes[i];
        planes[i].N = p.N;
        planes[i].D = p.D;
    }
    const int n_planes = planes.size();

    const int n_edges = n_planes * (n_planes - 1) / 2;
    std::vector<Edge> edges(n_edges);
    int i_edge = 0;
    for (int i = 0; i < n_planes; ++i) {
        auto& plane_a = planes[i];
        if (plane_a.N.length() == .0f) {
            continue;
        }
        for (int j = i + 1; j < n_planes; ++j) {
            auto& plane_b = planes[j];
            if (plane_b.N.length() == .0f) {
                continue;
            }
            gfxm::vec3 C = gfxm::cross(plane_a.N, plane_b.N);
            float Clen = C.length();
            if (Clen <= FLT_EPSILON) {
                continue;
            }
            gfxm::vec3 N = C / Clen;
            gfxm::vec3 P = (gfxm::cross(plane_b.N, C) * plane_a.D + gfxm::cross(C, plane_a.N) * plane_b.D) / C.length2();
            edges[i_edge].P = P;
            edges[i_edge].N = N;
            ++i_edge;
        }
    }
    int edge_count = i_edge;
    
    i_edge = 0;
    for (int i = 0; i < n_planes; ++i) {
        Plane& plane = planes[i];
        for (int j = 0; j < edge_count; ++j) {
            Edge& edge = edges[j];
            if (edge.discard) {
                continue;
            }
            float denom = gfxm::dot(plane.N, edge.N);
            if (abs(denom) <= 0.00001f) {
                continue;
            }
            gfxm::vec3 vec = plane.N * plane.D - edge.P;
            float t = gfxm::dot(vec, plane.N) / denom;
            if (denom > .0f) {
                if (edge.tb > t) {
                    edge.tb = gfxm::_min(edge.tb, t);
                }
            } else {
                if (edge.ta < t) {
                    edge.ta = gfxm::_max(edge.ta, t);
                }
            }
        }
    }

    for (int i = 0; i < edge_count; ++i) {
        Edge& edge = edges[i];
        if (edge.ta == -INFINITY || edge.tb == INFINITY || edge.ta >= edge.tb) {
            edge.discard = true;
        }
    }

    // TODO: Find unique vertices
    struct Vert {
        gfxm::vec3 P;
    };
    std::unordered_map<uint64_t, Vert*> vertices;
    for (int i = 0; i < edge_count; ++i) {
        Edge& edge = edges[i];
        if (edge.discard) {
            continue;
        }
        gfxm::vec3 va = edge.P + edge.N * edge.ta;
        gfxm::vec3 vb = edge.P + edge.N * edge.tb;
        uint64_t ha = makeVertexHash(va);
        uint64_t hb = makeVertexHash(vb);
        auto it = vertices.find(ha);
        if (it == vertices.end()) {
            //vertices.insert(std::make_pair(ha, new Vert{ va }));
        }
        it = vertices.find(hb);
        if (it == vertices.end()) {
            //vertices.insert(std::make_pair(hb, new Vert{ vb }));
        }
    }
    // TODO:
    assert(false);
}
void csgShapeInitFacesFromPlanes_(csgBrushShape* shape) {
    std::unordered_map<std::size_t, int> vertex_hash_to_index;
    // TODO: Optimize
    // Init faces once in local space
    // Then transform resulting vertices, avoiding face recalculations upon transforming
    // Still need to reinit faces when planes are modified tho
    int plane_count = shape->planes.size();
    shape->control_points.clear();
    shape->faces.clear();
    shape->faces.resize(plane_count);
    for (int i = 0; i < plane_count; ++i) {
        shape->faces[i].reset(new csgFace);
        shape->faces[i]->shape = shape;
        shape->faces[i]->N = shape->planes[i].N;
        shape->faces[i]->D = shape->planes[i].D;
        shape->faces[i]->lclN = shape->planes[i].N;
        shape->faces[i]->lclD = shape->planes[i].D;
        shape->faces[i]->material = shape->planes[i].material;
        shape->faces[i]->uv_offset = shape->planes[i].uv_offset;
        shape->faces[i]->uv_scale = shape->planes[i].uv_scale;
    }
    bool aabb_initialized = false;
    
    for (int i = 0; i < plane_count; ++i) {
        for (int j = i + 1; j < plane_count - 1; ++j) {
            for (int k = j + 1; k < plane_count; ++k) {
                auto FA = shape->faces[i].get();
                auto FB = shape->faces[j].get();
                auto FC = shape->faces[k].get();
                csgVertex v;
                if (!makeVertex(FA, FB, FC, &v)) {
                    continue;
                }
                std::size_t hash = std::hash<csgVertex>()(v);
                int vertex_index = -1;
                csgVertex* p_vertex = 0;
                auto it = vertex_hash_to_index.find(hash);
                if (it == vertex_hash_to_index.end()
                    || !csgCompareVertices(*shape->control_points[it->second].get(), v)
                ) {
                    RELATION_TYPE rel = csgCheckVertexShapeRelation(&v, shape);
                    if (rel == RELATION_OUTSIDE) {
                        continue;
                    }
                    vertex_index = shape->_controlPointCount();
                    vertex_hash_to_index.insert(std::make_pair(hash, vertex_index));
                    p_vertex = shape->_createControlPoint(v);
                } else {
                    vertex_index = it->second;
                    p_vertex = shape->_getControlPoint(vertex_index);

                    p_vertex->faces.insert(FA);
                    p_vertex->faces.insert(FB);
                    p_vertex->faces.insert(FC);
                }


                if (aabb_initialized) {
                    gfxm::expand_aabb(shape->aabb, v.position);
                } else {
                    shape->aabb.from = v.position;
                    shape->aabb.to = v.position;
                    aabb_initialized = true;
                }

                FA->tmp_control_points.insert(p_vertex);
                FB->tmp_control_points.insert(p_vertex);
                FC->tmp_control_points.insert(p_vertex);
            }
        }
    }
    if (aabb_initialized) {
        shape->aabb.from.x -= 0.001f;
        shape->aabb.from.y -= 0.001f;
        shape->aabb.from.z -= 0.001f;
        shape->aabb.to.x += 0.001f;
        shape->aabb.to.y += 0.001f;
        shape->aabb.to.z += 0.001f;
    }
    shape->world_space_vertices.resize(shape->_controlPointCount());
    for (int i = 0; i < shape->_controlPointCount(); ++i) {
        shape->world_space_vertices[i] = shape->_getControlPoint(i)->position;
    }

    for (int i = 0; i < shape->faces.size(); ++i) {
        auto F = shape->faces[i].get();
        findEdges(shape, F);
        fixWinding(shape, F);
    }

    csgUpdateShapeNormals(shape);
}

void csgUpdateShapeWorldSpace(csgBrushShape* shape) {
    bool aabb_initialized = false;
    shape->world_space_vertices.resize(shape->control_points.size());
    gfxm::mat4 inv_transp = gfxm::transpose(gfxm::inverse(shape->transform));
    for (int i = 0; i < shape->control_points.size(); ++i) {
        auto& lclv = *shape->control_points[i].get();
        gfxm::vec3 world_v = shape->transform * gfxm::vec4(lclv.position, 1.f);
        shape->world_space_vertices[i] = world_v;
        
        if (aabb_initialized) {
            gfxm::expand_aabb(shape->aabb, world_v);
        } else {
            shape->aabb.from = world_v;
            shape->aabb.to = world_v;
            aabb_initialized = true;
        }

        for (auto& fptr : shape->faces) {
            auto& f = *fptr.get();
            f.normals.resize(f.lcl_normals.size());
            for (int j = 0; j < f.lcl_normals.size(); ++j) {
                auto& N = f.lcl_normals[j];
                f.normals[j] = inv_transp * gfxm::vec4(N, .0f);
                f.normals[j] = gfxm::normalize(f.normals[j]);
            }
        }
    }
    if (aabb_initialized) {
        shape->aabb.from -= gfxm::vec3(.001f, .001f, .001f);
        shape->aabb.to += gfxm::vec3(.001f, .001f, .001f);
    }

    for (auto& fptr : shape->faces) {
        auto& face = *fptr.get();
        assert(face.vertexCount());

        face.mid_point = gfxm::vec3(0, 0, 0);
        for (int j = 0; j < face.vertexCount(); ++j) {
            auto& v = face.getWorldVertexPos(j);
            face.mid_point += v;
        }
        face.mid_point /= (float)face.vertexCount();

        face.aabb_world.from = face.getWorldVertexPos(0);
        face.aabb_world.to = face.getWorldVertexPos(0);
        for (int j = 1; j < face.vertexCount(); ++j) {
            gfxm::expand_aabb(face.aabb_world, face.getWorldVertexPos(j));
        }
        face.aabb_world.from -= gfxm::vec3(.001f, .001f, .001f);
        face.aabb_world.to += gfxm::vec3(.001f, .001f, .001f);


        gfxm::mat4 transp_inv = gfxm::transpose(gfxm::inverse(shape->transform));
        face.N = transp_inv * gfxm::vec4(face.lclN, .0f);
        face.N = gfxm::normalize(face.N);
        gfxm::vec3 lclP = face.lclN * face.lclD;
        gfxm::vec3 P = shape->transform * gfxm::vec4(lclP, 1.f);
        face.D = gfxm::dot(face.N, P);

        if (shape->automatic_uv) {
            face.uvs.resize(face.control_points.size());

            gfxm::vec3 face_origin = face.lclN * face.lclD;
            gfxm::mat3 orient = csgMakeFaceLocalOrientationMatrix(&face, face_origin);
            gfxm::vec2& uv_scale = face.uv_scale;
            gfxm::vec2& uv_offset = face.uv_offset;
            for (int i = 0; i < face.control_points.size(); ++i) {
                gfxm::vec3 V = face.control_points[i]->position;
                gfxm::vec2 uv = projectVertexXY(orient, face_origin, V);
                uv.x += uv_offset.x;
                uv.y += uv_offset.y;
                uv.x *= 1.f / uv_scale.x;
                uv.y *= 1.f / uv_scale.y;
                face.uvs[i] = uv;
            }
        }
    }
}

void csgUpdateFaceNormals(csgFace* face) {
    auto shape = face->shape;
    face->lcl_normals.resize(face->control_points.size());
    for (int i = 0; i < face->control_points.size(); ++i) {
        auto& v = *face->control_points[i];
        gfxm::vec3 N;
        float fcount = .0f;
        for (auto f : v.faces) {
            if (fabsf(gfxm::dot(face->lclN, f->lclN)) < .707f) {
                continue;
            }
            N += f->lclN;
            fcount += 1.f;
        }
        //N /= fcount;
        face->lcl_normals[i] = gfxm::normalize(N);
    }
    if (face->uvs.size() != face->control_points.size()) {
        face->uvs.resize(face->control_points.size());

        gfxm::vec3 face_origin = face->lclN * face->lclD;
        gfxm::mat3 orient = csgMakeFaceLocalOrientationMatrix(face, face_origin);
        gfxm::vec2& uv_scale = face->uv_scale;
        gfxm::vec2& uv_offset = face->uv_offset;
        for (int i = 0; i < face->control_points.size(); ++i) {
            gfxm::vec3 V = face->control_points[i]->position;
            gfxm::vec2 uv = projectVertexXY(orient, face_origin, V);
            uv.x += uv_offset.x;
            uv.y += uv_offset.y;
            uv.x *= 1.f / uv_scale.x;
            uv.y *= 1.f / uv_scale.y;
            face->uvs[i] = uv;
        }
    }
}
void csgUpdateShapeNormals(csgBrushShape* shape) {
    for (auto& face : shape->faces) {
        csgUpdateFaceNormals(face.get());
    }
}

void csgTransformShape(csgBrushShape* shape, const gfxm::mat4& transform) {
    shape->transform = transform;
    shape->invalidate();/*
    for (int i = 0; i < shape->planes.size(); ++i) {
        auto& plane = shape->planes[i];
        gfxm::vec3 O = plane.untransformed.N * plane.untransformed.D;
        gfxm::vec3 N = plane.untransformed.N;
        N = transform * gfxm::vec4(N, .0f);
        O = transform * gfxm::vec4(O, 1.f);
        plane.N = gfxm::normalize(N);
        plane.D = gfxm::dot(O, plane.N = gfxm::normalize(N));
    }*/
}


void csgRecalculatePlanesFromFaceVertices(csgBrushShape* shape, int face_id) {
    // TODO: Don't recalculate unaffected planes
    /*
    for (auto& f : shape->faces) {
        assert(f.vertices.size() >= 3);
        const gfxm::vec3& p0 = shape->vertices[f.vertices[0].index].position;
        const gfxm::vec3& p1 = shape->vertices[f.vertices[1].index].position;
        const gfxm::vec3& p2 = shape->vertices[f.vertices[2].index].position;
        gfxm::vec3 N = gfxm::normalize(gfxm::cross(p1 - p0, p1 - p2));
        if (gfxm::dot(f.N, N) < .0f) {
            N *= -1.f;
        }
        gfxm::vec3 P = f.mid_point;
        N = gfxm::inverse(shape->transform) * gfxm::vec4(N, .0f);
        P = gfxm::inverse(shape->transform) * gfxm::vec4(P, 1.f);
        float D = gfxm::dot(N, P);
        f.plane->untransformed.N = N;
        f.plane->untransformed.D = D;
    }*/
    shape->invalidate();
}

void csgMakeTestShape(csgBrushShape* shape, const gfxm::mat4& transform) {
    shape->planes.clear();
    shape->planes.resize(7);
    shape->planes[0] = { gfxm::vec3(-1,  0,  0), 1.f };
    shape->planes[1] = { gfxm::vec3(1,  0,  0), .0f };
    shape->planes[2] = { gfxm::vec3(0, -1,  0), -.0f };
    shape->planes[3] = { gfxm::vec3(0,  1,  0), 1.f };
    shape->planes[4] = { gfxm::vec3(0,  0, -1), 1.f };
    shape->planes[5] = { gfxm::vec3(0,  0,  1), .0f };
    shape->planes[6] = { gfxm::normalize(gfxm::vec3(1,  0, -1)), .2f };
    shape->transform = transform;

    csgTransformShape(shape, transform);
    csgShapeInitFacesFromPlanes_(shape);
    csgUpdateShapeWorldSpace(shape);
}
void csgMakeCube(csgBrushShape* shape, float width, float height, float depth, const gfxm::mat4& transform) {
    shape->planes.clear();
    shape->planes.resize(6);
    shape->planes[0] = { gfxm::vec3(-1,  0,  0), width * .5f };
    shape->planes[1] = { gfxm::vec3( 1,  0,  0), width * .5f };
    shape->planes[2] = { gfxm::vec3( 0, -1,  0), height * .5f };
    shape->planes[3] = { gfxm::vec3( 0,  1,  0), height * .5f };
    shape->planes[4] = { gfxm::vec3( 0,  0, -1), depth * .5f };
    shape->planes[5] = { gfxm::vec3( 0,  0,  1), depth * .5f };
    shape->transform = transform;

    csgTransformShape(shape, transform);
    csgShapeInitFacesFromPlanes_(shape);
    csgUpdateShapeWorldSpace(shape);
}
void csgMakeBox(csgBrushShape* shape, float width, float height, float depth, const gfxm::mat4& transform) {
    const float hw = fabsf(width) * .5f;
    const float hh = fabsf(height) * .5f;
    const float hd = depth * .5f;
    shape->planes.clear();
    shape->planes.resize(6);/*
    shape->planes[0] = { gfxm::vec3(-1,  0,  0), hw };
    shape->planes[1] = { gfxm::vec3( 1,  0,  0), hw };
    shape->planes[2] = { gfxm::vec3( 0, -1,  0), hh };
    shape->planes[3] = { gfxm::vec3( 0,  1,  0), hh };*/
    shape->planes[0] = { gfxm::vec3(-1,  0,  0), width < .0f ? fabsf(width) : .0f };
    shape->planes[1] = { gfxm::vec3(1,  0,  0), width < .0f ? .0f : fabsf(width) };
    shape->planes[2] = { gfxm::vec3(0, -1,  0), height < .0f ? fabsf(height) : .0f };
    shape->planes[3] = { gfxm::vec3(0,  1,  0), height < .0f ? .0f : fabsf(height) };
    shape->planes[4] = { gfxm::vec3( 0,  0, -1), depth < .0f ? fabsf(depth) : .0f };
    shape->planes[5] = { gfxm::vec3( 0,  0,  1), depth < .0f ? .0f : fabsf(depth) };
    shape->transform = transform;

    csgTransformShape(shape, transform);
    csgShapeInitFacesFromPlanes_(shape);
    csgUpdateShapeWorldSpace(shape);
}
void csgMakeCylinder(csgBrushShape* shape, float height, float radius, int segments, const gfxm::mat4& transform) {
    assert(height != .0f);
    assert(segments >= 3);
    assert(radius > 0.001f);

    std::vector<gfxm::vec2> uvs;
    float inv_2pi = 1.f / (gfxm::pi * 2.f);
    float inv_pi = 1.f / gfxm::pi;
    //float diameter = radius * 2.f;
    float approx_pi = segments * sinf(2.f * gfxm::pi / segments);
    float circum_step = 2.f * radius * sinf(gfxm::pi / segments);
    float circumference = segments * circum_step;

    shape->automatic_uv = false;
    shape->planes.clear();
    shape->faces.clear();
    shape->control_points.clear();

    for (int i = 0; i < segments; ++i) {
        float a = i / (float)segments * gfxm::pi * 2.f;
        float x = sinf(a) * radius;
        float z = cosf(a) * radius;
        shape->_createControlPoint(gfxm::vec3(x, height > .0f ? .0f : height, z));
        shape->_createControlPoint(gfxm::vec3(x, height > .0f ? height : .0f, z));
        //uvs.push_back(gfxm::vec2(a * inv_2pi * diameter, .0f));
        //uvs.push_back(gfxm::vec2(a * inv_2pi * diameter, 1.f));
        uvs.push_back(gfxm::vec2(a * inv_2pi * circumference, .0f));
        uvs.push_back(gfxm::vec2(a * inv_2pi * circumference, fabsf(height)));
    }

    shape->faces.resize(segments + 2);
    for (int i = 0; i < segments; ++i) {
        const int a = i * 2;
        const int b = i * 2 + 1;
        const int c = (i * 2 + 2) % shape->_controlPointCount();
        const int d = (i * 2 + 3) % shape->_controlPointCount();
        gfxm::vec2 uv_seam_fix = gfxm::vec2(1.f, .0f) * float((i + 1) / segments);

        auto cpa = shape->_getControlPoint(a);
        auto cpb = shape->_getControlPoint(b);
        auto cpc = shape->_getControlPoint(c);
        auto cpd = shape->_getControlPoint(d);

        shape->faces[i].reset(new csgFace);
        auto& face = *shape->faces[i].get();
        face.shape = shape;
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
    }

    {
        shape->faces[segments].reset(new csgFace);
        auto& face = *shape->faces[segments].get();
        face.shape = shape;
        for (int i = segments - 1; i >= 0; --i) {
            const int a = i * 2;
            auto cp = shape->_getControlPoint(a);
            face.control_points.push_back(shape->_getControlPoint(a));
            face.uvs.push_back(gfxm::vec2(cp->position.x, cp->position.z));
            cp->faces.insert(&face);
        }
        auto cpa = face.control_points[0];
        auto cpb = face.control_points[1];
        auto cpc = face.control_points[2];
        gfxm::vec3 N = gfxm::normalize(gfxm::cross(cpb->position - cpc->position, cpb->position - cpa->position));
        float D = gfxm::dot(N, cpa->position);
        face.lclN = N;
        face.lclD = D;
    }

    {
        shape->faces[segments + 1].reset(new csgFace);
        auto& face = *shape->faces[segments + 1].get();
        face.shape = shape;
        for (int i = 0; i < segments; ++i) {
            const int a = i * 2 + 1;
            auto cp = shape->_getControlPoint(a);
            face.control_points.push_back(shape->_getControlPoint(a));
            face.uvs.push_back(gfxm::vec2(cp->position.x, cp->position.z));
            cp->faces.insert(&face);
        }
        auto cpa = face.control_points[0];
        auto cpb = face.control_points[1];
        auto cpc = face.control_points[2];
        gfxm::vec3 N = gfxm::normalize(gfxm::cross(cpb->position - cpc->position, cpb->position - cpa->position));
        float D = gfxm::dot(N, cpa->position);
        face.lclN = N;
        face.lclD = D;
    }

    shape->transform = transform;

    csgTransformShape(shape, transform);
    csgUpdateShapeWorldSpace(shape);
    csgUpdateShapeNormals(shape);
    /*
    shape->planes.clear();
    shape->planes.resize(segments + 2);
    shape->planes[0] = { gfxm::vec3( 0, -1,  0), .0f };
    for (int i = 0; i < segments; ++i) {
        float step = gfxm::pi * 2 / (float)segments;
        float a = step * (float)i;
        gfxm::vec3 n = gfxm::vec3(cosf(a), .0f, sinf(a));
        shape->planes[i + 1] = { gfxm::normalize(n), radius };
    }
    shape->planes[segments + 1] = { gfxm::vec3(0,  1,  0), height };
    shape->transform = transform;

    csgTransformShape(shape, transform);
    csgShapeInitFacesFromPlanes_(shape);
    csgUpdateShapeWorldSpace(shape);*/
}
void csgMakeSphere(csgBrushShape* shape, int segments, float radius, const gfxm::mat4& transform) {
    assert(radius > 0.001f);

    shape->automatic_uv = false;

    segments = std::max(4, segments - (segments % 2));

    std::vector<gfxm::vec2> uvs;

    {
        shape->_createControlPoint(gfxm::vec3(0, radius, 0));
        uvs.push_back(gfxm::vec2(.5f, 1.0f));
    }

    float inv_2pi = 1.f / (gfxm::pi * 2.f);
    float inv_pi = 1.f / gfxm::pi;
    for (int i = 0; i < segments - 1; ++i) {
        for (int j = 0; j < segments; ++j) {
            float lat = (i + 1) / (float)segments * gfxm::pi * 2.f;
            float lon = j / (float)segments * gfxm::pi * 2.f;
            float z = radius * cosf(lon) * sinf(lat);
            float y = radius * cosf(lat);
            float x = radius * sinf(lon) * sinf(lat);
            gfxm::vec3 P = gfxm::vec3(x, y, z);
            gfxm::vec3 N = gfxm::normalize(P);

            shape->_createControlPoint(P);
            uvs.push_back(gfxm::vec2(lon * inv_2pi, 1.f - lat * inv_pi));
        }
    }
    
    {
        shape->_createControlPoint(gfxm::vec3(0, -radius, 0));
        uvs.push_back(gfxm::vec2(.5f, .0f));
    }

    int levels = segments / 2 - 2;
    shape->faces.resize(segments * levels + segments * 2);

    // Cap
    for (int i = 0; i < segments; ++i) {
        int a = 0;
        int b = i % (segments) + 1;
        int c = (i + 1) % (segments) + 1;

        gfxm::vec2 uv_seam_fix = gfxm::vec2(1.f, .0f) * float((i + 1) / segments);

        shape->faces[i].reset(new csgFace);
        auto& face = *shape->faces[i].get();
        face.shape = shape;
        face.control_points.push_back(shape->_getControlPoint(a));
        face.control_points.push_back(shape->_getControlPoint(b));
        face.control_points.push_back(shape->_getControlPoint(c));
        face.uvs.push_back(uvs[a]);
        face.uvs.push_back(uvs[b]);
        face.uvs.push_back(uvs[c] + uv_seam_fix);

        auto& p0 = *shape->_getControlPoint(a);
        auto& p1 = *shape->_getControlPoint(b);
        auto& p2 = *shape->_getControlPoint(c);
        p0.faces.insert(&face);
        p1.faces.insert(&face);
        p2.faces.insert(&face);
        gfxm::vec3 N = gfxm::normalize(gfxm::cross(p1.position - p2.position, p1.position - p0.position));
        float D = gfxm::dot(N, p0.position);
        face.lclN = N;
        face.lclD = D;
    }
    
    for (int lv = 0; lv < levels; ++lv) {
        for (int i = 0; i < segments; ++i) {
            int a = (i + 1) % segments + lv * segments + 1;
            int b = i + lv * segments + 1;
            int c = b + segments;
            int d = a + segments;

            gfxm::vec2 uv_seam_fix = gfxm::vec2(1.f, .0f) * float((i + 1) / segments);

            shape->faces[i + lv * segments + segments].reset(new csgFace);
            csgFace& face = *shape->faces[i + lv * segments + segments].get();
            face.shape = shape;
            face.control_points.push_back(shape->_getControlPoint(a));
            face.control_points.push_back(shape->_getControlPoint(b));
            face.control_points.push_back(shape->_getControlPoint(c));
            face.control_points.push_back(shape->_getControlPoint(d));
            face.uvs.push_back(uvs[a] + uv_seam_fix);
            face.uvs.push_back(uvs[b]);
            face.uvs.push_back(uvs[c]);
            face.uvs.push_back(uvs[d] + uv_seam_fix);

            auto& p0 = *shape->_getControlPoint(a);
            auto& p1 = *shape->_getControlPoint(b);
            auto& p2 = *shape->_getControlPoint(c);
            auto& p3 = *shape->_getControlPoint(d);
            p0.faces.insert(&face);
            p1.faces.insert(&face);
            p2.faces.insert(&face);
            p3.faces.insert(&face);
            gfxm::vec3 N = gfxm::normalize(gfxm::cross(p1.position - p2.position, p1.position - p0.position));
            float D = gfxm::dot(N, p0.position);
            face.lclN = N;
            face.lclD = D;
        }
    }

    // Cap
    for (int i = 0; i < segments; ++i) {
        int a = shape->_controlPointCount() - 1;
        int b = (i + 1) % (segments)+1 + levels * segments;
        int c = i % (segments)+1 + levels * segments;

        gfxm::vec2 uv_seam_fix = gfxm::vec2(1.f, .0f) * float((i + 1) / segments);


        shape->faces[i + levels * segments + segments].reset(new csgFace);
        csgFace& face = *shape->faces[i + levels * segments + segments].get();
        face.shape = shape;
        face.control_points.push_back(shape->_getControlPoint(a));
        face.control_points.push_back(shape->_getControlPoint(b));
        face.control_points.push_back(shape->_getControlPoint(c));
        face.uvs.push_back(uvs[a]);
        face.uvs.push_back(uvs[b] + uv_seam_fix);
        face.uvs.push_back(uvs[c]);

        auto& p0 = *shape->_getControlPoint(a);
        auto& p1 = *shape->_getControlPoint(b);
        auto& p2 = *shape->_getControlPoint(c);
        p0.faces.insert(&face);
        p1.faces.insert(&face);
        p2.faces.insert(&face);
        gfxm::vec3 N = gfxm::normalize(gfxm::cross(p1.position - p2.position, p1.position - p0.position));
        float D = gfxm::dot(N, p0.position);
        face.lclN = N;
        face.lclD = D;
    }

    shape->transform = transform;

    csgTransformShape(shape, transform);
    csgUpdateShapeWorldSpace(shape);
    csgUpdateShapeNormals(shape);
}


bool csgIntersectLineFace(const gfxm::vec3& e0, const gfxm::vec3& e1, csgFace* face, gfxm::vec3& out) {
    assert(face->vertexCount() >= 3);

    //const auto& p0 = face->vertices[0].position;
    //const auto& p1 = face->vertices[1].position;
    //const auto& p2 = face->vertices[2].position;
    const auto& p0 = face->getWorldVertexPos(0);
    const auto& p1 = face->getWorldVertexPos(1);
    const auto& p2 = face->getWorldVertexPos(2);
    gfxm::vec3 N = face->N;

    gfxm::vec3 line_plane_intersection;
    gfxm::vec3 edge_V = (e1 - e0);
    float dot_test = (gfxm::dot(N, edge_V));
    float t;
    if (fabsf(dot_test) > FLT_EPSILON) {
        t = gfxm::dot(N, (p0 - e0) / dot_test);
        line_plane_intersection = e0 + edge_V * t;
        if (t < .0f || t > 1.0f) {
            return false;
        }
    } else {
        return false;
    }

    for (int i = 1; i < face->vertexCount(); ++i) {
        const auto& p0 = face->getWorldVertexPos(i - 1);
        const auto& p1 = face->getWorldVertexPos(i);
        gfxm::vec3 c = gfxm::cross(line_plane_intersection - p0, p1 - p0);
        bool inside = gfxm::dot(c, N) <= 0;
        if (!inside) {
            return false;
        }
    }
    {
        const auto& p0 = face->getWorldVertexPos(face->vertexCount() - 1);
        const auto& p1 = face->getWorldVertexPos(0);
        gfxm::vec3 c = gfxm::cross(line_plane_intersection - p0, p1 - p0);
        bool inside = gfxm::dot(c, N) <= 0;
        if (!inside) {
            return false;
        }
    }

    out = line_plane_intersection;

    return true;
}
bool csgIntersectFaceFace(csgFace* face_a, csgFace* face_b, std::vector<csgVertex>& out_vertices) {
    gfxm::vec3 p;
    std::vector<csgVertex> new_vertices;
    for (int k = 1; k < face_b->vertexCount(); ++k) {
        auto& p0 = face_b->getWorldVertexPos(k - 1);
        auto& p1 = face_b->getWorldVertexPos(k);
        if (!csgIntersectLineFace(p0, p1, face_a, p)) {
            continue;
        }
        csgVertex v;
        v.position = p;
        v.faces.insert(face_a);
        v.faces.insert(face_b);
        out_vertices.push_back(v);
    }
    auto& p0 = face_b->getWorldVertexPos(face_b->vertexCount() - 1);
    auto& p1 = face_b->getWorldVertexPos(0);
    if (csgIntersectLineFace(p0, p1, face_a, p)) {
        csgVertex v;
        v.position = p;
        v.faces.insert(face_a);
        v.faces.insert(face_b);
        out_vertices.push_back(v);
    }

    return true;
}

void csgPrepareCut(csgBrushShape* shape, const gfxm::vec3& cut_plane_N, float cut_plane_D, csgShapeCutData& out) {
    out.clear();
    out.shape = shape;
    out.cut_plane_N = cut_plane_N;
    out.cut_plane_D = cut_plane_D;

    for (int i = 0; i < shape->control_points.size(); ++i) {
        auto cp = shape->control_points[i].get();
        cp->tmp_relation = csgCheckVertexRelation(shape->world_space_vertices[cp->index], cut_plane_N, cut_plane_D);
        if (cp->tmp_relation == RELATION_FRONT) {
            out.front_control_points.push_back(cp);
        } else if(cp->tmp_relation == RELATION_BACK) {
            out.back_control_points.push_back(cp);
        } else {
            out.aligned_control_points.push_back(cp);
        }
    }

    for (int i = 0; i < shape->faces.size(); ++i) {
        int intersection_count = 0;
        csgFace* face = shape->faces[i].get();

        static_assert(RELATION_OUTSIDE == 0 && RELATION_INSIDE == 1 && RELATION_ALIGNED == 2, "Relation enum - unexpected values");
        int count[3];
        count[RELATION_INSIDE] = 0;
        count[RELATION_ALIGNED] = 0;
        count[RELATION_OUTSIDE] = 0;
        for (int k = 0; k < face->vertexCount(); ++k) {
            auto cp = face->control_points[k];
            count[cp->tmp_relation] += 1;
        }

        if (count[RELATION_INSIDE] == 0) {
            out.discarded_faces.push_back(i);
            continue;
        } else if(count[RELATION_INSIDE] < face->control_points.size()) {
            auto& face_cut_data = out.cut_faces[i];
            face_cut_data.face_id = i;
        } else {
            continue;
        }

        gfxm::vec3 p;
        for (int k = 0; k < face->vertexCount(); ++k) {
            int j = (k + 1) % face->vertexCount();
            auto& p0 = face->getWorldVertexPos(k);
            auto& p1 = face->getWorldVertexPos(j);
            if (!gfxm::intersect_ray_plane_point(p0, p1, cut_plane_N, cut_plane_D, p)) {
                continue;
            }
            out.preview_lines.push_back(p);

            ++intersection_count;
            if (intersection_count == 2) {
                break;
            }
        }
    }
}
void csgPerformCut(csgShapeCutData& data) {
    csgBrushShape* shape = data.shape;
    assert(shape);

    // TODO
    std::vector<csgVertex*> *discarded_control_points = &data.front_control_points;

    shape->faces.push_back(std::unique_ptr<csgFace>(new csgFace));
    csgFace* new_face = shape->faces.back().get();

    gfxm::mat4 inv_transform = gfxm::inverse(shape->transform);

    new_face->shape = shape;
    new_face->N = data.cut_plane_N;
    new_face->D = data.cut_plane_D;
    new_face->lclN =  inv_transform * gfxm::vec4(data.cut_plane_N, .0f);
    gfxm::vec3 lclP = inv_transform * gfxm::vec4(data.cut_plane_N * data.cut_plane_D, 1.f);
    new_face->lclD = gfxm::dot(new_face->lclN, lclP);
    new_face->material = 0;
    new_face->uv_offset = gfxm::vec2(0,0);
    new_face->uv_scale = gfxm::vec2(1,1);

    // TODO:
    // First find all outside vertices
    // Memorize all faces that use those vertices
    // Check if those faces need to be discarded or cut
    // As a result, faces that are inside do not need to be processed at all

    std::vector<csgFace*> intersected_faces;
    for (auto& kv : data.cut_faces) {
        intersected_faces.push_back(shape->faces[kv.first].get());
    }
    for (int i = 0; i < intersected_faces.size(); ++i) {
        for (int j = i; j < intersected_faces.size(); ++j) {
            auto face_b = intersected_faces[i];
            auto face_c = intersected_faces[j];
            csgVertex v;
            if (!makeVertexLocal(new_face, face_b, face_c, &v)) {
                continue;
            }
            // NOTE: No need to handle duplicates, they are not possible in this case I'm pretty sure

            RELATION_TYPE rel = RELATION_BACK;
            for (int k = 0; k < intersected_faces.size(); ++k) {
                auto& face = intersected_faces[k];
                rel = csgCheckVertexRelation(v.position, face->lclN, face->lclD);
                if (rel == RELATION_FRONT) {
                    break;
                }
            }
            if (rel == RELATION_FRONT) {
                continue;
            }

            csgVertex* p_vertex = shape->_createControlPoint(v);

            new_face->tmp_control_points.insert(p_vertex);
            face_b->tmp_control_points.insert(p_vertex);
            face_c->tmp_control_points.insert(p_vertex);
        }
    }

    findEdges(shape, new_face);
    fixWindingLocal(shape, new_face);

    csgUpdateFaceNormals(new_face);

    for (auto face : intersected_faces) {
        face->tmp_control_points.insert(face->control_points.begin(), face->control_points.end());
        for (auto cp : *discarded_control_points) {
            face->tmp_control_points.erase(cp);
        }
        for (auto cp : data.aligned_control_points) {
            face->tmp_control_points.erase(cp);
        }
        findEdges(shape, face);
        fixWindingLocal(shape, face);
        csgUpdateFaceNormals(face);
    }

    std::sort(data.discarded_faces.begin(), data.discarded_faces.end(), [](int a, int b)->bool {
        return a > b;
    });
    for (auto fidx : data.discarded_faces) {
        auto pface = shape->faces[fidx].get();
        for (auto cp : pface->control_points) {
            cp->faces.erase(pface);
        }
        shape->faces.erase(shape->faces.begin() + fidx);
    }

    for (auto cp : *discarded_control_points) {
        cp->faces.clear();
    }
    for (auto cp : data.aligned_control_points) {
        cp->faces.clear();
    }
    for (int i = 0; i < shape->control_points.size(); ++i) {
        if (shape->control_points[i]->faces.empty()) {
            shape->control_points.erase(shape->control_points.begin() + i);
            --i;
            continue;
        }
    }
    for (int i = 0; i < shape->control_points.size(); ++i) {
        shape->control_points[i]->index = i;
    }

    data.shape->invalidate();
}

/*
void csgIntersectShapes(csgBrushShape* shape_a, csgBrushShape* shape_b) {
    //auto face_a = &shape_a->faces[6];
    //auto face_b = &shape_b->faces[4];
    //csgIntersectFaceFace(face_a, face_b);
    //csgIntersectFaceFace(face_b, face_a);

    std::unordered_map<csgFace*, std::vector<csgVertex>> new_vertices_per_face;
    for (int i = 0; i < shape_a->faces.size(); ++i) {
        auto face_a = &shape_a->faces[i];
        for (int j = 0; j < shape_b->faces.size(); ++j) {
            auto face_b = &shape_b->faces[j];
            std::vector<csgVertex> new_vertices;
            csgIntersectFaceFace(face_a, face_b, new_vertices);
            csgIntersectFaceFace(face_b, face_a, new_vertices);

            if (!new_vertices.empty()) {
                assert(new_vertices.size() == 2);
                auto& a = new_vertices_per_face[face_a];
                auto& b = new_vertices_per_face[face_b];
                a.insert(a.end(), new_vertices.begin(), new_vertices.end());
                b.insert(b.end(), new_vertices.begin(), new_vertices.end());
            }
        }
    }
    for (auto& kv : new_vertices_per_face) {
        kv.first->vertices.insert(kv.first->vertices.end(), kv.second.begin(), kv.second.end());
    }
}*/


void csgSplit(csgFragment* frag, csgFace* face, csgFragment* front, csgFragment* back) {
    std::unordered_map<RELATION_TYPE, csgFragment*> pieces;
    pieces[RELATION_FRONT] = front;
    pieces[RELATION_BACK] = back;
    for (auto& kv : pieces) {
        csgFragment* piece = kv.second;
        piece->face = frag->face;
        piece->vertices.clear();
        piece->front_volume = frag->front_volume;
        piece->back_volume = frag->back_volume;
    }

    int vertex_count = frag->vertices.size();
    for (int i = 0; i < vertex_count; ++i) {
        size_t j = (i + 1) % vertex_count;
        csgVertex v0 = frag->vertices[i];
        csgVertex v1 = frag->vertices[j];
        RELATION_TYPE c0 = csgCheckVertexRelation(&v0, face);
        RELATION_TYPE c1 = csgCheckVertexRelation(&v1, face);
        if (c0 != c1) {
            csgEdgeTmp edge;
            if (!findEdge(&v0, &v1, &edge)) {
                // TODO: not sure how to handle
                //assert(false);
                //pieces[c0]->vertices.push_back(v0);
                continue;
            }
            csgVertex v;
            if (!makeVertex(edge.faces[0], edge.faces[1], face, &v)) {
                // TODO: not sure how to handle
                //assert(false);
                //pieces[c0]->vertices.push_back(v0);
                continue;
            }
            float t = gfxm::dot(gfxm::normalize(v1.position - v0.position), v.position - v0.position) / (v1.position - v0.position).length();
            v.normal = (gfxm::slerp(v0.normal, v1.normal, t));
            v.uv = gfxm::lerp(v0.uv, v1.uv, t);

            if (c0 == RELATION_ALIGNED) {
                pieces[c1]->vertices.push_back(v);
            } else if(c1 == RELATION_ALIGNED) {
                pieces[c0]->vertices.push_back(v0);
                pieces[c0]->vertices.push_back(v);
            } else {
                pieces[c0]->vertices.push_back(v0);
                pieces[c0]->vertices.push_back(v);
                pieces[c1]->vertices.push_back(v);
            }
        } else {
            // TODO: not sure how to handle
            if (c0 == RELATION_ALIGNED) {
                //pieces[RELATION_BACK]->vertices.push_back(v0);
            } else {
                pieces[c0]->vertices.push_back(v0);
            }
        }
    }
}
std::vector<csgFragment> csgCarve(csgFragment frag, csgBrushShape* shape, int face_idx) {
    std::vector<csgFragment> pieces;

    if (face_idx >= shape->faces.size()) {
        return { frag };
    }

    csgFace* face = shape->faces[face_idx].get();
    RELATION_TYPE rel = csgCheckFragmentRelation(&frag, face);
    switch (rel) {
    case RELATION_FRONT:
        frag.rel_type = RELATION_OUTSIDE;
        return { frag };
    case RELATION_ALIGNED:
    case RELATION_REVERSE_ALIGNED:
        frag.rel_type = rel;
    case RELATION_BACK:
        return csgCarve(std::move(frag), shape, face_idx + 1);
    case RELATION_SPLIT: {
        csgFragment front;
        csgFragment back;
        front.N = frag.N;
        back.N = frag.N;
        csgSplit(&frag, face, &front, &back);
        back.rel_type = frag.rel_type;
        auto rest = csgCarve(std::move(back), shape, face_idx + 1);

        if (rest.size() == 1 && rest[0].rel_type == RELATION_OUTSIDE) {
            frag.rel_type = RELATION_OUTSIDE;
            return { frag };
        }

        front.rel_type = RELATION_OUTSIDE;
        rest.push_back(std::move(front));
        return rest;
    }
    }
    
    assert(false);
    return {};
}
bool csgCheckShapeOrder(const csgBrushShape* shape_a, const csgBrushShape* shape_b) {
    return shape_a->uid < shape_b->uid;
}
void csgRebuildFragments(csgBrushShape* shape) {
    for (auto& fptr : shape->faces) {
        auto& face = *fptr.get();
        face.fragments.clear();
        face.fragments.emplace_back();
        {
            csgFragment* fragment = &face.fragments.back();
            fragment->face = fptr.get();
            fragment->N = face.N;
            fragment->vertices.resize(face.control_points.size());
            for (int i = 0; i < face.control_points.size(); ++i) {
                fragment->vertices[i] = *face.control_points[i];
                fragment->vertices[i].position = shape->world_space_vertices[face.control_points[i]->index];
                fragment->vertices[i].normal = face.normals[i];
                fragment->vertices[i].uv = face.uvs[i];
            }
            //fragment->vertices = face.vertices;
            fragment->front_volume = VOLUME_SOLID;
            fragment->back_volume = shape->volume_type;
            fragment->rgba = shape->rgba;
        }

        for(auto other : shape->intersecting_sorted) {
            if (!intersectAabb(face.aabb_world, other->aabb)) {
                continue;
            }

            int frag_count = face.fragments.size();
            for (int frag_idx = frag_count - 1; frag_idx >= 0; --frag_idx) {
                bool is_first = csgCheckShapeOrder(shape, other); // TODO
                csgFragment frag = std::move(face.fragments[frag_idx]);
                face.fragments.erase(face.fragments.begin() + frag_idx);

                frag.rel_type = RELATION_INSIDE;
                std::vector<csgFragment> frags = csgCarve(std::move(frag), other, 0);
                for (auto& frag : frags) {
                    bool keep_frag = true;
                    switch (frag.rel_type) {
                    case RELATION_INSIDE:
                        if (is_first) {
                            frag.back_volume = other->volume_type;
                        }
                        frag.front_volume = other->volume_type;
                        break;
                    case RELATION_ALIGNED:
                        if (is_first) {
                            keep_frag = false;
                        }
                        break;
                    case RELATION_REVERSE_ALIGNED:
                        if (is_first) {
                            keep_frag = false;
                        } else {
                            frag.front_volume = other->volume_type;
                        }
                        break;
                    }
                    if (keep_frag) {
                        face.fragments.emplace_back(std::move(frag));
                    }
                }
            }
        }
    }
}
void csgTriangulateFragment(const csgFragment* frag, int base_index, std::vector<uint32_t>& out_indices) {
    int vertex_count = frag->vertices.size();
    std::vector<uint32_t> indices;
    indices.resize(vertex_count);
    for (int i = 0; i < vertex_count; ++i) {
        indices[i] = base_index + i;
    }
    for (;;) {
        int n = indices.size();
        if (n < 3) {
            break;
        }
        if (n == 3) {
            out_indices.push_back(indices[0]);
            out_indices.push_back(indices[1]);
            out_indices.push_back(indices[2]);
            break;
        }
        out_indices.push_back(indices[0]);
        out_indices.push_back(indices[1]);
        out_indices.push_back(indices[2]);

        out_indices.push_back(indices[2]);
        out_indices.push_back(indices[3]);
        out_indices.push_back(indices[4 % n]);
        indices.erase(indices.begin() + 3);
        indices.erase(indices.begin() + 1);
    }
}

gfxm::mat3 csgMakeFaceLocalOrientationMatrix(csgFace* face, gfxm::vec3& origin) {
    origin = face->lclN * face->lclD;
    gfxm::vec3 up = gfxm::vec3(.0f, 1.f, .0f);
    gfxm::vec3 right = gfxm::vec3(1.f, .0f, .0f);
    gfxm::vec3 back = gfxm::vec3(.0f, .0f, 1.f);
    gfxm::vec3 planeN = face->lclN;
    gfxm::mat3 m;
    if (fabsf(gfxm::dot(up, planeN)) >= (1.f - FLT_EPSILON)) {
        back = gfxm::cross(planeN, right);
        right = gfxm::cross(back, up);
        up = planeN;

        m[0] = right;
        m[1] = back;
        m[2] = up;
    } else {
        right = gfxm::cross(up, planeN);
        up = gfxm::cross(planeN, right);
        back = planeN;

        m[0] = right;
        m[1] = up;
        m[2] = back;
    }

    return m;
}
gfxm::mat3 csgMakeFaceOrientationMatrix(csgFace* face, gfxm::vec3& origin) {
    origin = face->N * face->D;
    gfxm::vec3 up = gfxm::vec3(.0f, 1.f, .0f);
    gfxm::vec3 right = gfxm::vec3(1.f, .0f, .0f);
    gfxm::vec3 back = gfxm::vec3(.0f, .0f, 1.f);
    gfxm::vec3 planeN = face->N;
    gfxm::mat3 m;
    if (fabsf(gfxm::dot(up, planeN)) >= (1.f - FLT_EPSILON)) {
        back = gfxm::cross(planeN, right);
        right = gfxm::cross(back, up);
        up = planeN;

        m[0] = right;
        m[1] = back;
        m[2] = up;
    } else {
        right = gfxm::cross(up, planeN);
        up = gfxm::cross(planeN, right);
        back = planeN;

        m[0] = right;
        m[1] = up;
        m[2] = back;
    }

    return m;
}


void csgMakeShapeTriangles(
    csgBrushShape* shape,
    std::unordered_map<csgMaterial*, csgMeshData>& mesh_data
) {
    for (int i = 0; i < shape->faces.size(); ++i) {
        csgMaterial* mat = shape->material;
        if (shape->faces[i]->material) {
            mat = shape->faces[i]->material;
        }
        auto& mesh = mesh_data[mat];
        mesh.material = mat;

        int base_index = mesh.vertices.size();
        gfxm::vec3 face_origin;
        gfxm::mat3 orient = csgMakeFaceOrientationMatrix(shape->faces[i].get(), face_origin);

        for (int j = 0; j < shape->faces[i]->fragments.size(); ++j) {
            auto& frag = shape->faces[i]->fragments[j];
            if (frag.back_volume == frag.front_volume) {
                continue;
            }
            float normal_mul = 1.f;
            bool inverse_winding = false;
            if (frag.front_volume == VOLUME_EMPTY) {

            } else if(frag.back_volume == VOLUME_EMPTY) {
                normal_mul = -1.f;
                inverse_winding = true;
            }
            for (int k = 0; k < frag.vertices.size(); ++k) {
                mesh.vertices.push_back(frag.vertices[k].position);
                //mesh.normals.push_back(normal_mul * frag.face->N);
                mesh.normals.push_back(normal_mul * frag.vertices[k].normal);
                mesh.colors.push_back(shape->rgba);
                mesh.uvs.push_back(frag.vertices[k].uv);

                /*
                gfxm::vec2& uv_scale = shape->faces[i].uv_scale;
                gfxm::vec2& uv_offset = shape->faces[i].uv_offset;
                gfxm::vec2 uv = projectVertexXY(orient, face_origin, frag.vertices[k].position);
                uv.x += uv_offset.x;
                uv.y += uv_offset.y;
                uv.x *= 1.f / uv_scale.x;
                uv.y *= 1.f / uv_scale.y;
                mesh.uvs.push_back(uv);*/
            }
            std::vector<uint32_t> indices_frag;
            csgTriangulateFragment(&frag, base_index, indices_frag);
            if (inverse_winding) {
                std::reverse(indices_frag.begin(), indices_frag.end());
            }
            mesh.indices.insert(mesh.indices.end(), indices_frag.begin(), indices_frag.end());
            base_index = mesh.vertices.size();
        }
    }

    for (auto& kv : mesh_data) {
        auto& mesh = kv.second;
        assert(mesh.indices.size() % 3 == 0);
        mesh.tangents.resize(mesh.vertices.size());
        mesh.bitangents.resize(mesh.vertices.size());
        for (int l = 0; l < mesh.indices.size(); l += 3) {
            uint32_t a = mesh.indices[l];
            uint32_t b = mesh.indices[l + 1];
            uint32_t c = mesh.indices[l + 2];

            gfxm::vec3 Va = mesh.vertices[a];
            gfxm::vec3 Vb = mesh.vertices[b];
            gfxm::vec3 Vc = mesh.vertices[c];
            gfxm::vec3 Na = mesh.normals[a];
            gfxm::vec3 Nb = mesh.normals[b];
            gfxm::vec3 Nc = mesh.normals[c];
            gfxm::vec2 UVa = mesh.uvs[a];
            gfxm::vec2 UVb = mesh.uvs[b];
            gfxm::vec2 UVc = mesh.uvs[c];

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

            mesh.tangents[a] += sdir;
            mesh.tangents[b] += sdir;
            mesh.tangents[c] += sdir;
            mesh.bitangents[a] += tdir;
            mesh.bitangents[b] += tdir;
            mesh.bitangents[c] += tdir;
        }
        for (int k = 0; k < mesh.vertices.size(); ++k) {
            mesh.tangents[k] = gfxm::normalize(mesh.tangents[k]);
            mesh.bitangents[k] = gfxm::normalize(mesh.bitangents[k]);
        }
    }
}

void csgInit() {
    assert(csg_data == 0);
    csg_data = new csgData;

    csgMaterial* mat_floor;
    csgMaterial* mat_floor2;
    csgMaterial* mat_wall;
    csgMaterial* mat_wall2;
    csgMaterial* mat_ceiling;
    {
        ktImage img_floor;
        if (!loadImage(&img_floor, "floor_checker0.png")) {
            assert(false);
        }
        ktImage img_floor2;
        if (!loadImage(&img_floor2, "floor_wood0.jpg")) {
            assert(false);
        }
        ktImage img_wall;
        if (!loadImage(&img_wall, "wall_industry0.jpg")) {
            assert(false);
        }
        ktImage img_wall2;
        if (!loadImage(&img_wall2, "wall_concrete1.png")) {
            assert(false);
        }
        ktImage img_ceiling;
        if (!loadImage(&img_ceiling, "ceiling0.png")) {
            assert(false);
        }
        std::unique_ptr<csgMaterial> mat_floor_(new csgMaterial);
        std::unique_ptr<csgMaterial> mat_floor2_(new csgMaterial);
        std::unique_ptr<csgMaterial> mat_wall_(new csgMaterial);
        std::unique_ptr<csgMaterial> mat_wall2_(new csgMaterial);
        std::unique_ptr<csgMaterial> mat_ceiling_(new csgMaterial);
        mat_floor = mat_floor_.get();
        mat_floor2 = mat_floor2_.get();
        mat_wall = mat_wall_.get();
        mat_wall2 = mat_wall2_.get();
        mat_ceiling = mat_ceiling_.get();
        mat_floor->texture.reset(new gpuTexture2d);
        mat_floor2->texture.reset(new gpuTexture2d);
        mat_wall->texture.reset(new gpuTexture2d);
        mat_wall2->texture.reset(new gpuTexture2d);
        mat_ceiling->texture.reset(new gpuTexture2d);
        mat_floor->texture->setData(&img_floor);
        mat_floor2->texture->setData(&img_floor2);
        mat_wall->texture->setData(&img_wall);
        mat_wall2->texture->setData(&img_wall2);
        mat_ceiling->texture->setData(&img_ceiling);

        csg_data->materials.push_back(std::move(mat_floor_));
        csg_data->materials.push_back(std::move(mat_floor2_));
        csg_data->materials.push_back(std::move(mat_wall_));
        csg_data->materials.push_back(std::move(mat_wall2_));
        csg_data->materials.push_back(std::move(mat_ceiling_));
    }

    {
        const char* vs_src = R"(
            #version 450 
            in vec3 inPosition;
            in vec3 inNormal;
            in vec4 inColorRGBA;
            in vec2 inUV;

            uniform mat4 matProjection;
            uniform mat4 matView;
            uniform mat4 matModel;

            out vec3 frag_normal;
            out vec3 frag_pos;
            out vec4 frag_rgba;
            out vec2 frag_uv;        

            void main(){
                frag_uv = inUV;
                frag_rgba = inColorRGBA;
                frag_normal = inNormal;        
	            vec4 pos = matProjection * matView * matModel * vec4(inPosition, 1);
                frag_pos = inPosition.xyz;
                gl_Position = pos;
            })";
        const char* fs_src = R"(
            #version 450
            uniform sampler2D tex;
            uniform mat4 matView;
            out vec4 outAlbedo;
            in vec3 frag_normal;
            in vec3 frag_pos;
            in vec4 frag_rgba;
            in vec2 frag_uv;

            vec3 calcPointLightness(vec3 frag_pos, vec3 normal, vec3 light_pos, float radius, vec3 L_col) {
	            vec3 light_vec = light_pos - frag_pos;
	            float dist = length(light_vec);
	            vec3 light_dir = normalize(light_vec);
	            float lightness = max(dot(normal, light_dir), .0);
	            float attenuation = clamp(1.0 - dist*dist/(radius*radius), 0.0, 1.0);
	            attenuation *= attenuation;
	            return L_col * lightness * attenuation;
            }

            vec3 calcDirLight(vec3 N, vec3 L_dir, vec3 L_col) {
	            float L = max(dot(N, -L_dir), .0f);
	            return L * L_col;
            }

            void main(){
	            vec3 N = frag_normal;
	            if(!gl_FrontFacing) {
		            N *= -1;
	            }
                vec3 camPos = inverse(matView)[3].xyz;
	            vec3 V = inverse(matView)[2].xyz;
	            float vdn = 1.0 - max(dot(V, N), 0.0);
	            vec3 rimcolor = vec3(smoothstep(0.6, 1.0, vdn));
	
	            vec3 L = calcPointLightness(frag_pos, N, camPos, 15, vec3(1,1,1));/*
		            + calcPointLightness(frag_pos, N, vec3(4, 3, 1), 10, vec3(0.2,0.5,1))
		            + calcPointLightness(frag_pos, N, vec3(-4, 3, 1), 10, vec3(1,0.5,0.2))
		            + calcDirLight(N, vec3(0, -1, 0), vec3(.3,.3,.3));*/
	            vec3 color = /*frag_rgba.rgb * */texture(tex, frag_uv).rgb * L;
                outAlbedo = vec4(color, 1);
            })";
        csg_data->prog.reset(new gpuShaderProgram(vs_src, fs_src));
        csg_data->prog->setUniform1i("tex", 0);
    }
    {
        const char* vs_src = R"(
            #version 450 
            in vec3 inPosition;

            uniform mat4 matProjection;
            uniform mat4 matView;
            uniform mat4 matModel;

            out float frag_vertex_id;
        
            void main(){         
                float frag_vertex_id =  float(gl_VertexID); 
	            vec4 pos = matProjection * matView * matModel * vec4(inPosition, 1);
                gl_Position = pos;
            })";
        const char* fs_src = R"(
            #version 450
            out vec4 outAlbedo;
            in float frag_vertex_id;

            void main(){
                float vidx = frag_vertex_id;
                vec3 col3 = vec3(1,1,1);
                outAlbedo = vec4(col3, 1);
            })";
        csg_data->prog_line.reset(new gpuShaderProgram(vs_src, fs_src));
    }

    std::vector<gfxm::vec3> lines;
    std::vector<gfxm::vec3> points;
    {
        csgScene scene;
        csgBrushShape shape_room;
        csgBrushShape shape_pillar;
        csgBrushShape shape_pillar2;
        //csgMakeTestShape(&shape_a, gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(-10, 1, 0)));
        csgMakeCube(&shape_room, 10.f, 4.f, 10.f, gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(-9, 2, 0 + 32)));
        csgMakeCylinder(&shape_pillar, 5.f, .5f, 16, gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(-10.5f, 0, 5 + 32)));
        csgMakeCylinder(&shape_pillar2, 5.f, .5f, 16, gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(-7.5f, 0, 5 + 32)));
        shape_room.volume_type = VOLUME_EMPTY;
        shape_room.rgba = makeColor32(0.7, .4f, .6f, 1.f);
        shape_room.planes[0].uv_scale = gfxm::vec2(5.f, 5.f * .62f);
        shape_room.planes[0].uv_offset = gfxm::vec2(.0f, -3.2f);
        shape_room.planes[0].material = mat_wall;
        shape_room.planes[1].uv_scale = gfxm::vec2(5.f, 5.f * .62f);
        shape_room.planes[1].uv_offset = gfxm::vec2(.0f, -3.2f);
        shape_room.planes[1].material = mat_wall;
        shape_room.planes[4].uv_scale = gfxm::vec2(5.f, 5.f * .62f);
        shape_room.planes[4].uv_offset = gfxm::vec2(1.0f, -3.2f);
        shape_room.planes[4].material = mat_wall;
        shape_room.planes[5].uv_scale = gfxm::vec2(5.f, 5.f * .62f);
        shape_room.planes[5].uv_offset = gfxm::vec2(-1.0f, -3.2f);
        shape_room.planes[5].material = mat_wall;

        shape_room.planes[2].uv_scale = gfxm::vec2(5.f, 5.f);
        shape_room.planes[2].uv_offset = gfxm::vec2(.0f, .0f);
        shape_room.planes[2].material = mat_floor;
        shape_room.planes[3].uv_scale = gfxm::vec2(3.f, 3.f);
        shape_room.planes[3].uv_offset = gfxm::vec2(.0f, .0f);
        shape_room.planes[3].material = mat_ceiling;

        shape_pillar.volume_type = VOLUME_SOLID;
        shape_pillar.rgba = makeColor32(.5f, .7f, .0f, 1.f);
        shape_pillar.material = mat_wall2;
        shape_pillar2.volume_type = VOLUME_SOLID;
        shape_pillar2.rgba = makeColor32(.5f, .7f, .0f, 1.f);
        shape_pillar2.material = mat_wall2;

        csgBrushShape shape_doorway;
        csgMakeCube(&shape_doorway, 2, 2.5, 1, gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(-9, 1.25, 5.5 + 32)));
        shape_doorway.volume_type = VOLUME_EMPTY;
        shape_doorway.rgba = makeColor32(.0f, .5f, 1.f, 1.f);
        shape_doorway.material = mat_wall;

        csgBrushShape shape_arch_part;
        csgMakeCylinder(&shape_arch_part, 1, 1, 32,
            gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(-9, 2.5, 6 + 32))
            * gfxm::to_mat4(gfxm::angle_axis(gfxm::radian(-90), gfxm::vec3(1, 0, 0)))
        );
        shape_arch_part.volume_type = VOLUME_EMPTY;
        shape_arch_part.rgba = makeColor32(.5f, 1.f, .0f, 1.f);
        shape_arch_part.material = mat_wall;

        csgBrushShape shape_room2;
        csgMakeCube(&shape_room2, 10.f, 4.f, 10.f, gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(-10.5, 2, 11. + 32)));
        shape_room2.volume_type = VOLUME_EMPTY;
        shape_room2.rgba = makeColor32(.0f, 1.f, .5f, 1.f);
        shape_room2.material = mat_floor2;
        shape_room2.faces[2]->uv_scale = gfxm::vec2(5, 5);

        csgBrushShape shape_window;
        csgMakeCube(&shape_window, 2.5, 2.5, 1, gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(-12.5, 2., 5.5 + 32)));
        shape_window.volume_type = VOLUME_EMPTY;
        shape_window.rgba = makeColor32(.5f, 1.f, .0f, 1.f);
        shape_window.material = mat_wall2;

        scene.addShape(&shape_room);
        scene.addShape(&shape_pillar);
        scene.addShape(&shape_pillar2);
        scene.addShape(&shape_room2);
        scene.addShape(&shape_doorway);
        scene.addShape(&shape_arch_part);
        scene.addShape(&shape_window);
        scene.update();

        //csgIntersectShapes(&shape_a, &shape_b);
        //csgRebuildFragments(&shape_a, &shape_b);
        //csgRebuildFragments(&shape_b, &shape_a);
        /*
        for (int i = 0; i < shape_a.faces.size(); ++i) {
            for (int j = 0; j < shape_a.faces[i].fragments.size(); ++j) {
                auto& frag = shape_a.faces[i].fragments[j];
                for (int k = 0; k < frag.vertices.size(); ++k) {
                    int next = (k + 1) % frag.vertices.size();
                    lines.push_back(frag.vertices[k].position);
                    lines.push_back(frag.vertices[next].position);
                }
            }
        }
        for (int i = 0; i < shape_b.faces.size(); ++i) {
            for (int j = 0; j < shape_b.faces[i].fragments.size(); ++j) {
                auto& frag = shape_b.faces[i].fragments[j];
                for (int k = 0; k < frag.vertices.size(); ++k) {
                    int next = (k + 1) % frag.vertices.size();
                    lines.push_back(frag.vertices[k].position);
                    lines.push_back(frag.vertices[next].position);
                }
            }
        }

        for (int i = 0; i < shape_a.faces.size(); ++i) {
            auto f = &shape_a.faces[i];
            for (int j = 0; j < f->vertices.size(); ++j) {
                points.push_back(f->vertices[j].position);
            }
        }*/


        std::unordered_map<csgMaterial*, csgMeshData> mesh_data;
        csgMakeShapeTriangles(&shape_room, mesh_data);
        csgMakeShapeTriangles(&shape_pillar, mesh_data);
        csgMakeShapeTriangles(&shape_pillar2, mesh_data);
        csgMakeShapeTriangles(&shape_doorway, mesh_data);
        csgMakeShapeTriangles(&shape_arch_part, mesh_data);
        csgMakeShapeTriangles(&shape_room2, mesh_data);
        csgMakeShapeTriangles(&shape_window, mesh_data);

        for (auto& kv : mesh_data) {
            auto material = kv.first;
            auto& mesh = kv.second;

            auto ptr = new csgData::Mesh;
            ptr->vertex_buffer.setArrayData(mesh.vertices.data(), mesh.vertices.size() * sizeof(mesh.vertices[0]));
            ptr->normal_buffer.setArrayData(mesh.normals.data(), mesh.normals.size() * sizeof(mesh.normals[0]));
            ptr->color_buffer.setArrayData(mesh.colors.data(), mesh.colors.size() * sizeof(mesh.colors[0]));
            ptr->uv_buffer.setArrayData(mesh.uvs.data(), mesh.uvs.size() * sizeof(mesh.uvs[0]));
            ptr->index_buffer.setArrayData(mesh.indices.data(), mesh.indices.size() * sizeof(mesh.indices[0]));
            ptr->mesh_desc.reset(new gpuMeshDesc);
            ptr->mesh_desc->setDrawMode(MESH_DRAW_MODE::MESH_DRAW_TRIANGLES);
            ptr->mesh_desc->setAttribArray(VFMT::Position_GUID, &ptr->vertex_buffer, 0);
            ptr->mesh_desc->setAttribArray(VFMT::Normal_GUID, &ptr->normal_buffer, 0);
            ptr->mesh_desc->setAttribArray(VFMT::ColorRGBA_GUID, &ptr->color_buffer, 0);
            ptr->mesh_desc->setAttribArray(VFMT::UV_GUID, &ptr->uv_buffer, 0);
            ptr->mesh_desc->setIndexArray(&ptr->index_buffer);
            gpuMakeMeshShaderBinding(&ptr->mesh_binding, csg_data->prog.get(), ptr->mesh_desc.get(), 0);
            ptr->material = mesh.material;

            csg_data->meshes.push_back(std::unique_ptr<csgData::Mesh>(ptr));
        }
    }


    {
        float width = .03f;
        float height = .03f;
        float depth = .03f;
        float w = width * .5f;
        float h = height * .5f;
        float d = depth * .5f;

        float vertices[] = {
            -w, -h, -d,  w, -h, -d,
            -w, -h,  d,  w, -h,  d,
            -w,  h, -d,  w,  h, -d,
            -w,  h,  d,  w,  h,  d,
            -w, -h, -d, -w, -h,  d,
             w, -h, -d,  w, -h,  d,
            -w,  h, -d, -w,  h,  d,
             w,  h, -d,  w,  h,  d,
            -w, -h, -d, -w,  h, -d,
             w, -h, -d,  w,  h, -d,
             w, -h,  d,  w,  h,  d,
            -w, -h,  d, -w,  h,  d
        };
        csg_data->point.vertex_buffer.reset(new gpuBuffer);
        csg_data->point.vertex_buffer->setArrayData(vertices, sizeof(vertices));
        gpuMeshDesc mesh_desc;
        mesh_desc.setDrawMode(MESH_DRAW_MODE::MESH_DRAW_LINES);
        mesh_desc.setAttribArray(VFMT::Position_GUID, csg_data->point.vertex_buffer.get(), 0);
        mesh_desc.setVertexCount(sizeof(vertices) * 24);
        gpuMakeMeshShaderBinding(&csg_data->point.mesh_binding, csg_data->prog.get(), &mesh_desc, 0);
    }

    float width = 1.f;
    float height = 1.f;
    float depth = 1.f;
    float w = width * .5f;
    float h = height * .5f;
    float d = depth * .5f;

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

    //csg_data->vertex_buffer->setArrayData(vertices, sizeof(vertices));
    //csg_data->normal_buffer->setArrayData(normals, sizeof(normals));
    //csg_data->index_buffer->setArrayData(indices, sizeof(indices));
    /*
    gpuMeshDesc mesh_desc;
    mesh_desc.setDrawMode(MESH_DRAW_MODE::MESH_DRAW_TRIANGLES);
    mesh_desc.setAttribArray(VFMT::Position_GUID, csg_data->vertex_buffer.get(), 0);
    mesh_desc.setAttribArray(VFMT::Normal_GUID, csg_data->normal_buffer.get(), 0);
    mesh_desc.setIndexArray(csg_data->index_buffer.get());

    csg_data->mesh_binding = csg_data->prog->getMeshBinding(&mesh_desc, 0);
    */
    struct Edge {
        int idx_a;
        int idx_b;
        int face_a;
        int face_b;
    };
    struct SimpleEdge {
        int va;
        int vb;
        bool remove = false;
    };
    struct Vertex {
        gfxm::vec3 pos;
        std::list<int> edges;
    };
    struct Face {
        std::vector<SimpleEdge> simple_edges;
        std::vector<int> vertices;
        gfxm::vec3 normal;
        std::vector<std::unique_ptr<Face>> intersection_faces;

        // Only used when first adding vertices of a new intersection face
        Face* intersection_face = 0;

        //
        std::vector<uint32_t> triangulated_indices;
        std::vector<gfxm::vec3> triangulated_vertices;
        std::vector<gfxm::vec3> triangulated_normals;

        void reverse_edges() {
            for (int i = 0; i < simple_edges.size(); ++i) {
                auto& e = simple_edges[i];
                std::swap(e.va, e.vb);
            }
            std::reverse(simple_edges.begin(), simple_edges.end());
        }
    };
    struct Shape {
        std::vector<Vertex> vertices;
        std::vector<Face> faces;
        std::vector<Edge> edges;
    };

    auto recalcFaceNormal = [](Shape* shape, Face* face) {
        assert(face->simple_edges.size() >= 3);
        face->normal = gfxm::cross(
            shape->vertices[face->simple_edges[0].vb].pos - shape->vertices[face->simple_edges[0].va].pos,
            shape->vertices[face->simple_edges[1].vb].pos - shape->vertices[face->simple_edges[1].va].pos
        );
    };

    auto makeBox = [](float width, float height, float depth, const gfxm::mat4& tr, Shape& shape) {
        float hw = width * .5f;
        float hh = height * .5f;
        float hd = depth * .5f;
        shape.vertices.resize(8);
        shape.vertices[0].pos = gfxm::vec3{ -hw, -hh, -hd };
        shape.vertices[1].pos = gfxm::vec3{  hw, -hh, -hd };
        shape.vertices[2].pos = gfxm::vec3{  hw, -hh,  hd };
        shape.vertices[3].pos = gfxm::vec3{ -hw, -hh,  hd };
        shape.vertices[4].pos = gfxm::vec3{ -hw,  hh, -hd };
        shape.vertices[5].pos = gfxm::vec3{  hw,  hh, -hd };
        shape.vertices[6].pos = gfxm::vec3{  hw,  hh,  hd };
        shape.vertices[7].pos = gfxm::vec3{ -hw,  hh,  hd };
        for (int i = 0; i < shape.vertices.size(); ++i) {
            shape.vertices[i].pos = tr * gfxm::vec4(shape.vertices[i].pos, 1.f);
        }
        shape.faces.resize(6);
        shape.faces[0].vertices.resize(4);
        shape.faces[0].vertices[0] = 0;
        shape.faces[0].vertices[1] = 1;
        shape.faces[0].vertices[2] = 2;
        shape.faces[0].vertices[3] = 3;
        shape.faces[0].simple_edges.resize(4);
        shape.faces[0].simple_edges[0] = { 0, 1 };
        shape.faces[0].simple_edges[1] = { 1, 2 };
        shape.faces[0].simple_edges[2] = { 2, 3 };
        shape.faces[0].simple_edges[3] = { 3, 0 };
        shape.faces[0].normal = tr * gfxm::vec4(.0f, -1.f, .0f, .0f);
        shape.faces[1].vertices.resize(4);
        shape.faces[1].vertices[0] = 4;
        shape.faces[1].vertices[1] = 7;
        shape.faces[1].vertices[2] = 6;
        shape.faces[1].vertices[3] = 5;
        shape.faces[1].simple_edges.resize(4);
        shape.faces[1].simple_edges[0] = { 4, 7 };
        shape.faces[1].simple_edges[1] = { 7, 6 };
        shape.faces[1].simple_edges[2] = { 6, 5 };
        shape.faces[1].simple_edges[3] = { 5, 4 };
        shape.faces[1].normal = tr * gfxm::vec4(.0f, 1.f, .0f, .0f);
        shape.faces[2].vertices.resize(4);
        shape.faces[2].vertices[0] = 0;
        shape.faces[2].vertices[1] = 4;
        shape.faces[2].vertices[2] = 5;
        shape.faces[2].vertices[3] = 1;
        shape.faces[2].simple_edges.resize(4);
        shape.faces[2].simple_edges[0] = { 0, 4 };
        shape.faces[2].simple_edges[1] = { 4, 5 };
        shape.faces[2].simple_edges[2] = { 5, 1 };
        shape.faces[2].simple_edges[3] = { 1, 0 };
        shape.faces[2].normal = tr * gfxm::vec4(.0f, .0f, -1.f, .0f);
        shape.faces[3].vertices.resize(4);
        shape.faces[3].vertices[0] = 3;
        shape.faces[3].vertices[1] = 2;
        shape.faces[3].vertices[2] = 6;
        shape.faces[3].vertices[3] = 7;
        shape.faces[3].simple_edges.resize(4);
        shape.faces[3].simple_edges[0] = { 3, 2 };
        shape.faces[3].simple_edges[1] = { 2, 6 };
        shape.faces[3].simple_edges[2] = { 6, 7 };
        shape.faces[3].simple_edges[3] = { 7, 3 };
        shape.faces[3].normal = tr * gfxm::vec4(.0f, .0f, 1.f, .0f);
        shape.faces[4].vertices.resize(4);
        shape.faces[4].vertices[0] = 0;
        shape.faces[4].vertices[1] = 3;
        shape.faces[4].vertices[2] = 7;
        shape.faces[4].vertices[3] = 4;
        shape.faces[4].simple_edges.resize(4);
        shape.faces[4].simple_edges[0] = { 0, 3 };
        shape.faces[4].simple_edges[1] = { 3, 7 };
        shape.faces[4].simple_edges[2] = { 7, 4 };
        shape.faces[4].simple_edges[3] = { 4, 0 };
        shape.faces[4].normal = tr * gfxm::vec4(-1.f, .0f, .0f, .0f);
        shape.faces[5].vertices.resize(4);
        shape.faces[5].vertices[0] = 2;
        shape.faces[5].vertices[1] = 1;
        shape.faces[5].vertices[2] = 5;
        shape.faces[5].vertices[3] = 6;
        shape.faces[5].simple_edges.resize(4);
        shape.faces[5].simple_edges[0] = { 2, 1 };
        shape.faces[5].simple_edges[1] = { 1, 5 };
        shape.faces[5].simple_edges[2] = { 5, 6 };
        shape.faces[5].simple_edges[3] = { 6, 2 };
        shape.faces[5].normal = tr * gfxm::vec4(1.f, .0f, .0f, .0f);

        shape.edges.resize(12);
        shape.edges[0].idx_a = 0;
        shape.edges[0].idx_b = 1;
        shape.edges[0].face_a = 0;
        shape.edges[0].face_b = 2;
        shape.edges[1].idx_a = 3;
        shape.edges[1].idx_b = 2;
        shape.edges[1].face_a = 3;
        shape.edges[1].face_b = 0;
        shape.edges[2].idx_a = 4;
        shape.edges[2].idx_b = 5;
        shape.edges[2].face_a = 2;
        shape.edges[2].face_b = 1;
        shape.edges[3].idx_a = 7;
        shape.edges[3].idx_b = 6;
        shape.edges[3].face_a = 1;
        shape.edges[3].face_b = 3;
        shape.edges[4].idx_a = 0;
        shape.edges[4].idx_b = 4;
        shape.edges[4].face_a = 2;
        shape.edges[4].face_b = 4;
        shape.edges[5].idx_a = 1;
        shape.edges[5].idx_b = 5;
        shape.edges[5].face_a = 5;
        shape.edges[5].face_b = 2;
        shape.edges[6].idx_a = 2;
        shape.edges[6].idx_b = 6;
        shape.edges[6].face_a = 3;
        shape.edges[6].face_b = 5;
        shape.edges[7].idx_a = 3;
        shape.edges[7].idx_b = 7;
        shape.edges[7].face_a = 4;
        shape.edges[7].face_b = 3;
        shape.edges[8].idx_a = 0;
        shape.edges[8].idx_b = 3;
        shape.edges[8].face_a = 4;
        shape.edges[8].face_b = 0;
        shape.edges[9].idx_a = 4;
        shape.edges[9].idx_b = 7;
        shape.edges[9].face_a = 1;
        shape.edges[9].face_b = 4;
        shape.edges[10].idx_a = 5;
        shape.edges[10].idx_b = 6;
        shape.edges[10].face_a = 5;
        shape.edges[10].face_b = 1;
        shape.edges[11].idx_a = 1;
        shape.edges[11].idx_b = 2;
        shape.edges[11].face_a = 0;
        shape.edges[11].face_b = 5;

        for (int i = 0; i < shape.edges.size(); ++i) {
            shape.vertices[shape.edges[i].idx_a].edges.push_back(i);
            shape.vertices[shape.edges[i].idx_b].edges.push_back(i);
        }
    };
    struct CreatedVertex {
        gfxm::vec3 pos;
        // If the vertex was created as a copy of an inside vertex
        // the owner will be non zero
        // and the owner_index would be the index of the original vertex.
        const Shape* owner = 0;
        int owner_index = 0;
    };
    struct Edge2 {
        int a = -1;
        int b = -1;
        const Shape* shape = 0;
        int face = 0;
        bool intersection_edge = false;
    };
    struct Edge_ {
        struct {
            int va = -1;
            int vb = -1;
        } on_shape_a;
        struct {
            int va = -1;
            int vb = -1;
        } on_shape_b;
    };

    auto findVertices = [](
        Shape* shape_a,
        Shape* shape_b,
        std::unordered_map<FacePairKey, Edge_>& edge_map,
        std::vector<Edge2>& out_edges        
    ) {
        std::vector<CreatedVertex> out_verts;

        const int vert_count = shape_a->vertices.size();
        const int edge_count = shape_a->edges.size();
        const int face_count = shape_a->faces.size();
        int shape_a_vidx = shape_a->vertices.size();
        int shape_b_vidx = shape_b->vertices.size();
        for (int fi = 0; fi < face_count; ++fi) {
            for (int ei = 0; ei < edge_count; ++ei) {
                gfxm::vec3 result;
                if (intersectEdgeFace(
                    shape_b->vertices[shape_b->edges[ei].idx_a].pos,
                    shape_b->vertices[shape_b->edges[ei].idx_b].pos,
                    shape_a->vertices[shape_a->faces[fi].vertices[0]].pos,
                    shape_a->vertices[shape_a->faces[fi].vertices[1]].pos,
                    shape_a->vertices[shape_a->faces[fi].vertices[2]].pos,
                    shape_a->vertices[shape_a->faces[fi].vertices[3]].pos,
                    result
                )) {
                    {
                        FacePairKey key((uint64_t)shape_a, (uint64_t)shape_b, fi, shape_b->edges[ei].face_a);
                        auto& edge = edge_map[key];
                        if (edge.on_shape_a.va == -1) {
                            edge.on_shape_a.va = shape_a_vidx;
                            edge.on_shape_b.va = shape_b_vidx;
                        } else {
                            edge.on_shape_a.vb = shape_a_vidx;
                            edge.on_shape_b.vb = shape_b_vidx;
                        }
                    }
                    {
                        FacePairKey key((uint64_t)shape_a, (uint64_t)shape_b, fi, shape_b->edges[ei].face_b);
                        auto& edge = edge_map[key];
                        if (edge.on_shape_a.va == -1) {
                            edge.on_shape_a.va = shape_a_vidx;
                            edge.on_shape_b.va = shape_b_vidx;
                        } else {
                            edge.on_shape_a.vb = shape_a_vidx;
                            edge.on_shape_b.vb = shape_b_vidx;
                        }
                    }
                    {
                        FacePairKey key((uint64_t)shape_b, (uint64_t)shape_b, shape_b->edges[ei].face_a, shape_b->edges[ei].face_b);
                        auto& edge = edge_map[key];
                        if (edge.on_shape_a.va == -1) {
                            edge.on_shape_a.va = shape_a_vidx;
                            edge.on_shape_b.va = shape_b_vidx;
                        } else {
                            edge.on_shape_a.vb = shape_a_vidx;
                            edge.on_shape_b.vb = shape_b_vidx;
                        }
                    }
                    CreatedVertex vert;
                    vert.pos = result;
                    out_verts.push_back(vert);
                    ++shape_a_vidx;
                    ++shape_b_vidx;
                }
            }
        }
        for (int vi = 0; vi < vert_count; ++vi) {
            bool inside = true;
            for (int fi = 0; fi < face_count; ++fi) {
                if (!isVertInsidePlane(
                    shape_b->vertices[vi].pos,
                    shape_a->vertices[shape_a->faces[fi].vertices[0]].pos,
                    shape_a->vertices[shape_a->faces[fi].vertices[1]].pos,
                    shape_a->vertices[shape_a->faces[fi].vertices[2]].pos
                )) {
                    inside = false;
                    break;
                }
            }
            if (inside) {
                for (auto& e : shape_b->vertices[vi].edges) {
                    {
                        FacePairKey key((uint64_t)shape_b, (uint64_t)shape_b, shape_b->edges[e].face_a, shape_b->edges[e].face_b);
                        auto& edge = edge_map[key];
                        if (edge.on_shape_a.va == -1) {
                            edge.on_shape_a.va = vi;
                            edge.on_shape_b.va = vi;
                        } else {
                            edge.on_shape_a.vb = vi;
                            edge.on_shape_b.vb = vi;
                        }
                    }
                }/*
                CreatedVertex vert;
                vert.pos = shape_b->vertices[vi].pos;
                vert.owner = shape_b;
                vert.owner_index = vi;
                out_verts.emplace_back(vert);*/
            }
        }
        for (int i = 0; i < out_verts.size(); ++i) {
            Vertex v;
            v.pos = out_verts[i].pos;
            shape_a->vertices.push_back(v);
            shape_b->vertices.push_back(v);
        }
    };
    auto finalizeFaceVertices = [](Shape* shape) {
        for (auto& f : shape->faces) {
            if (f.intersection_face == 0) {
                continue;
            }

            auto int_face = f.intersection_face;
            auto edge_list = int_face->simple_edges;
            int head = edge_list.front().va;
            int tail = edge_list.front().vb;
            edge_list.erase(edge_list.begin());
            int_face->vertices.push_back(head);
            auto it_edge = edge_list.begin();
            while (it_edge != edge_list.end() && head != tail) {
                if (tail == it_edge->va) {
                    tail = it_edge->vb;
                    int_face->vertices.push_back(it_edge->va);
                    edge_list.erase(it_edge);
                    it_edge = edge_list.begin();
                    continue;
                } else if (tail == it_edge->vb) {
                    tail = it_edge->va;
                    int_face->vertices.push_back(it_edge->vb);
                    edge_list.erase(it_edge);
                    it_edge = edge_list.begin();
                    continue;
                }
                ++it_edge;
            }

            int_face->simple_edges.clear();
            for (int i = 1; i < int_face->vertices.size(); ++i) {
                SimpleEdge se;
                se.va = int_face->vertices[i - 1];
                se.vb = int_face->vertices[i];
                int_face->simple_edges.push_back(se);
            }
            SimpleEdge se;
            se.va = int_face->vertices.back();
            se.vb = int_face->vertices.front();
            int_face->simple_edges.push_back(se);
            
            f.intersection_faces.emplace_back(std::unique_ptr<Face>(f.intersection_face));
            f.intersection_face = 0;
        }
    };
    auto cutHoles2 = [&recalcFaceNormal](Shape* shape) {
        for (auto& f : shape->faces) {
            if (f.intersection_faces.empty()) {
                continue;
            }

            auto* hole_face = f.intersection_faces[0].get();
            recalcFaceNormal(shape, hole_face);
            if (gfxm::dot(hole_face->normal, f.normal) > .0f) {
                hole_face->reverse_edges();
            }

            struct Split {
                float t;
                int splitting_vertex;
                int target_edge;
            };
            std::list<Split> splits;
            for (int j = 0; j < f.simple_edges.size(); ++j) {
                int v0 = f.simple_edges[j].va;
                int v1 = f.simple_edges[j].vb;

                for (int i = 0; i < hole_face->simple_edges.size(); ++i) {
                    int vh0 = hole_face->simple_edges[i].va;
                    int vh1 = hole_face->simple_edges[i].vb;

                    // TODO: Find a more precise way
                    // Maybe can determine which vertices are splitters in an earlier stage
                    float t = .0f;
                    if (!isPointOnLineSegment(
                        shape->vertices[vh0].pos,
                        shape->vertices[v0].pos,
                        shape->vertices[v1].pos,
                        t
                    )) {
                        continue;
                    }

                    Split split;
                    split.t = t;
                    split.splitting_vertex = vh0;
                    split.target_edge = j;
                    splits.push_back(split);
                }
            }
            if (!splits.empty()) {
                splits.sort([](const Split& a, const Split& b)->bool {
                    return a.t > b.t;
                });
                splits.sort([](const Split& a, const Split& b)->bool {
                    return a.target_edge > b.target_edge;
                });
                for (auto& s : splits) {
                    auto& edge = f.simple_edges[s.target_edge];
                    int edge_old_va = edge.va;
                    edge.va = s.splitting_vertex;
                    SimpleEdge new_edge;
                    new_edge.va = edge_old_va;
                    new_edge.vb = s.splitting_vertex;
                    f.simple_edges.insert(f.simple_edges.begin() + s.target_edge, new_edge);
                }
            }
        }
    };
    auto cutHoles = [&recalcFaceNormal](Shape* shape) {
        for (auto& f : shape->faces) {
            // TODO: Merge intersection polys into a single hole first
            // working with one poly for now
            if (f.intersection_faces.empty()) {
                continue;
            }
            auto* hole_face = f.intersection_faces[0].get();
            recalcFaceNormal(shape, hole_face);
            if (gfxm::dot(hole_face->normal, f.normal) > .0f) {
                hole_face->reverse_edges();
            }
            //hole_face->reverse_edges();
            //int base_index = shape->vertices.size();
            // TODO: Face should have its own vertices

            std::unordered_set<int> intersection_vertices;
            std::unordered_map<int, int> intersection_vertex_to_hole_edge;
            enum REMOVAL_FLAG {
                REMOVAL_FRONT,
                REMOVAL_BACK,
                REMOVAL_FLAG_COUNT
            };
            struct Split {
                float t;
                int splitting_vertex;
                int target_edge;
                REMOVAL_FLAG rem_flag;
            };
            std::list<Split> splits;
            for (int j = 0; j < f.simple_edges.size(); ++j) {
                int v0 = f.simple_edges[j].va;
                int v1 = f.simple_edges[j].vb;

                for (int i = 0; i < hole_face->simple_edges.size(); ++i) {
                    int vh0 = hole_face->simple_edges[i].va;
                    int vh1 = hole_face->simple_edges[i].vb;

                    // TODO: Find a more precise way
                    // Maybe can determine which vertices are splitters in an earlier stage
                    float t = .0f;
                    if (!isPointOnLineSegment(
                        shape->vertices[vh0].pos,
                        shape->vertices[v0].pos,
                        shape->vertices[v1].pos,
                        t
                    )) {
                        continue;
                    }
                    REMOVAL_FLAG rem_flag = REMOVAL_FRONT;
                    float t2 = .0f;
                    if (isPointOnLineSegment(
                        shape->vertices[vh1].pos,
                        shape->vertices[v0].pos,
                        shape->vertices[v1].pos,
                        t2
                    )) {
                        rem_flag = REMOVAL_BACK;
                    }
                    intersection_vertices.insert(vh0);
                    intersection_vertex_to_hole_edge[vh0] = i;

                    Split split;
                    split.t = t;
                    split.splitting_vertex = vh0;
                    split.target_edge = j;
                    splits.push_back(split);

                    printf("Edge split\n");
                }
            }
            REMOVAL_FLAG removal_flag = REMOVAL_BACK;
            if (!splits.empty()) {
                splits.sort([](const Split& a, const Split& b)->bool {
                    return a.t > b.t;
                });
                splits.sort([](const Split& a, const Split& b)->bool {
                    return a.target_edge > b.target_edge;
                });
                for (auto& s : splits) {
                    printf("s.edge == %i, s.t == %.3f\n", s.target_edge, s.t);
                }
                removal_flag = splits.front().rem_flag;
                for (auto& s : splits) {
                    auto& edge = f.simple_edges[s.target_edge];
                    int edge_old_va = edge.va;
                    edge.va = s.splitting_vertex;
                    SimpleEdge new_edge;
                    new_edge.va = edge_old_va;
                    new_edge.vb = s.splitting_vertex;
                    f.simple_edges.insert(f.simple_edges.begin() + s.target_edge, new_edge);

                    if (removal_flag == REMOVAL_FRONT) {
                        f.simple_edges[s.target_edge + 1].remove = true;
                    } else if(removal_flag == REMOVAL_BACK) {
                        f.simple_edges[s.target_edge].remove = true;
                    }
                    removal_flag = (REMOVAL_FLAG)(((int)removal_flag + 1) % REMOVAL_FLAG_COUNT);
                }
                printf("total resulting edges: %i\n", f.simple_edges.size());
            }
            for(int i = f.simple_edges.size() - 1; i >= 0; --i) {
                auto e = f.simple_edges[i];
                if (e.remove) {
                    f.simple_edges.erase(f.simple_edges.begin() + i);
                    /*
                    auto it = intersection_vertex_to_hole_edge.find(e.va);
                    if (it == intersection_vertex_to_hole_edge.end()) {
                        assert(false);
                        continue;
                    }
                    int first_hole_edge = it->second;
                    for (int j = first_hole_edge; j < hole_face->simple_edges.size(); ++j) {
                        auto hole_edge = hole_face->simple_edges[j];
                        f.simple_edges.insert(f.simple_edges.begin() + i, hole_edge);
                        if (intersection_vertices.find(hole_edge.vb) != intersection_vertices.end()) {
                            break;
                        }
                    }*/
                }
            }
            // TODO: HANDLE ISOLATED HOLES

            /*
            assert(f.simple_edges.size() >= 3);
            gfxm::vec3 N = gfxm::cross(
                shape->vertices[f.simple_edges[0].vb].pos - shape->vertices[f.simple_edges[0].va].pos,
                shape->vertices[f.simple_edges[1].vb].pos - shape->vertices[f.simple_edges[1].va].pos
            );
            if (gfxm::dot(N, f.normal) > .0f) {
                f.reverse_edges();
            }

            hole_face->reverse_edges();*/
        }
    };
    auto cdtFlattenVertex = [](const gfxm::vec3& v, const gfxm::vec3& origin, const gfxm::vec3& axisX, const gfxm::vec3& axisY)->CDT::V2d<float> {
        float x2d = gfxm::dot(axisX, v - origin);
        float y2d = gfxm::dot(axisY, v - origin);
        CDT::V2d<float> v2d;
        v2d.x = x2d;
        v2d.y = y2d;
        return v2d;
    };
    auto cdtUnflattenVertex = [](const CDT::V2d<float>& v2d, const gfxm::vec3& origin, const gfxm::vec3& axisX, const gfxm::vec3& axisY)->gfxm::vec3 {
        return origin + axisX * v2d.x + axisY * v2d.y;
    };
    auto triangulateFaces = [&cdtFlattenVertex, &cdtUnflattenVertex ](Shape* shape) {
        for (auto& f : shape->faces) {
            //auto& f = shape->faces[5];
            CDT::Triangulation<float> cdt;

            std::vector<gfxm::vec3> new_vertices;
            gfxm::vec3 axisZ = f.normal;
            gfxm::vec3 origin = shape->vertices[f.simple_edges[0].va].pos;
            gfxm::vec3 axisX = gfxm::normalize(shape->vertices[f.simple_edges[0].vb].pos - origin);
            gfxm::vec3 axisY = gfxm::cross(axisZ, axisX);
            std::vector<CDT::V2d<float>> cdt_vertices;
            std::unordered_map<uint32_t, uint32_t> index_remap;
            for (int i = 0; i < f.simple_edges.size(); ++i) {
                gfxm::vec3 v = shape->vertices[f.simple_edges[i].va].pos;
                index_remap[f.simple_edges[i].va] = cdt_vertices.size();
                new_vertices.push_back(v);
                cdt_vertices.push_back(cdtFlattenVertex(v, origin, axisX, axisY));
            }
            if (!f.intersection_faces.empty()) {
                auto int_face = f.intersection_faces[0].get();
                for (int i = 0; i < int_face->simple_edges.size(); ++i) {
                    {
                        int idx = int_face->simple_edges[i].vb;
                        auto it = index_remap.find(idx);
                        if (it != index_remap.end()) {
                            continue;
                        }
                        gfxm::vec3 v = shape->vertices[idx].pos;
                        index_remap[idx] = cdt_vertices.size();
                        new_vertices.push_back(v);
                        cdt_vertices.push_back(cdtFlattenVertex(v, origin, axisX, axisY));
                    }
                }
            }
            cdt.insertVertices(cdt_vertices);
            
            std::vector<CDT::Edge> cdt_edges;
            for (int i = 0; i < f.simple_edges.size(); ++i) {
                CDT::Edge edge(index_remap[f.simple_edges[i].va], index_remap[f.simple_edges[i].vb]);
                cdt_edges.push_back(edge);
            }
            std::vector<CDT::Edge> cdt_edges_hole;
            if (!f.intersection_faces.empty()) {
                auto int_face = f.intersection_faces[0].get();
                for (int i = 0; i < int_face->simple_edges.size(); ++i) {
                    CDT::Edge edge(index_remap[int_face->simple_edges[i].va], index_remap[int_face->simple_edges[i].vb]);
                    cdt_edges.push_back(edge);
                }
            }
            cdt.insertEdges(cdt_edges);
            //cdt.insertEdges(cdt_edges_hole);
            
            cdt.eraseOuterTrianglesAndHoles();

            f.triangulated_indices.resize(cdt.triangles.size() * 3);
            for (int i = 0; i < cdt.triangles.size(); ++i) {
                f.triangulated_indices[i * 3] = cdt.triangles[i].vertices[0];
                f.triangulated_indices[i * 3 + 1] = cdt.triangles[i].vertices[1];
                f.triangulated_indices[i * 3 + 2] = cdt.triangles[i].vertices[2];
            }

            f.triangulated_vertices = new_vertices;
            /*
            f.triangulated_vertices.resize(cdt.vertices.size());
            for (int i = 0; i < cdt.vertices.size(); ++i) {
                f.triangulated_vertices[i] = cdtUnflattenVertex(cdt.vertices[i], origin, axisX, axisY);
            }*/
            f.triangulated_normals.resize(cdt.vertices.size());
            for (int i = 0; i < cdt.vertices.size(); ++i) {
                f.triangulated_normals[i] = axisZ;
            }
        }
    };
    auto findIntersections = [&findVertices, &finalizeFaceVertices, &cutHoles, &cutHoles2, &triangulateFaces](
        Shape* shape_a,
        Shape* shape_b,
        std::vector<Edge2>& out_edges
    ) {
        std::unordered_map<FacePairKey, Edge_> edge_map;
        findVertices(shape_a, shape_b, edge_map, out_edges);
        findVertices(shape_b, shape_a, edge_map, out_edges);

        for (auto& kv : edge_map) {
            Edge2 e;
            e.a = kv.second.on_shape_a.va;
            e.b = kv.second.on_shape_a.vb;
            e.shape = (const Shape*)kv.first.shape_a;
            e.face = kv.first.face_a;
            e.intersection_edge = kv.first.shape_a != kv.first.shape_b;
            out_edges.push_back(e);

            e.a = kv.second.on_shape_b.va;
            e.b = kv.second.on_shape_b.vb;
            e.shape = (const Shape*)kv.first.shape_b;
            e.face = kv.first.face_b;
            out_edges.push_back(e);
        }

        for (auto& e : out_edges) {
            if (!e.intersection_edge) {
                //continue;
            }
            Shape* shape = (Shape*)e.shape;
            int face = e.face;
            
            if (shape->faces[face].intersection_face == 0) {
                shape->faces[face].intersection_face = new Face;
            }
            shape->faces[face].intersection_face->simple_edges.push_back(SimpleEdge{ e.a, e.b });
        }

        finalizeFaceVertices(shape_a);
        finalizeFaceVertices(shape_b);

        cutHoles2(shape_a);
        cutHoles2(shape_b);

        triangulateFaces(shape_a);
        triangulateFaces(shape_b);
    };

    Shape shape_a;
    Shape shape_b;
    makeBox(2, 2, 2, gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(-8, 1, 5)), shape_a);
    /*makeBox(2, 2, 2, 
        gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(-8, 1, 5) + gfxm::vec3(1, 1, 1))
        * gfxm::to_mat4(
            gfxm::angle_axis(.3f, gfxm::vec3(0,1,0))
            * gfxm::angle_axis(.3f, gfxm::vec3(1, 0, 0))
        ),
        shape_b
    );*/
    makeBox(4, 1, 1, 
        gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(-7.f, 1.f, 5.5f))
        * gfxm::to_mat4(
            gfxm::angle_axis(.3f, gfxm::vec3(0,1,0))
            * gfxm::angle_axis(.3f, gfxm::vec3(1, 0, 0))
        ),
        shape_b
    );

    std::vector<Edge2> new_edges;
    {
        findIntersections(&shape_a, &shape_b, new_edges);
    }
    for (int i = 0; i < shape_a.faces.size(); ++i) {
        auto& f = shape_a.faces[i];
        for (int j = 0; j < f.simple_edges.size(); ++j) {
            const auto& e = f.simple_edges[j];
            points.push_back(shape_a.vertices[e.va].pos);
        }
    }
    for (int i = 0; i < shape_b.faces.size(); ++i) {
        auto& f = shape_b.faces[i];
        for (int j = 0; j < f.simple_edges.size(); ++j) {
            const auto& e = f.simple_edges[j];
            points.push_back(shape_b.vertices[e.va].pos);
        }
    }


    struct ShapeData {
        std::unordered_map<int, std::list<Edge2>> edges_per_face;
    };
    std::unordered_map<const Shape*, ShapeData> shape_data;
    for (auto& e : new_edges) {
        const Shape* shape = e.shape;
        int face = e.face;
        shape_data[shape].edges_per_face[face].emplace_back(e);
    }


    /*
    for (auto& e : new_edges) {
        if (e.a == -1 || e.b == -1) {
            assert(false);
            continue;
        }
        lines.push_back(e.shape->vertices[e.a].pos);
        lines.push_back(e.shape->vertices[e.b].pos);
    }*//*
    for (auto& it : shape_a.edges) {
        lines.push_back(shape_a.vertices[it.idx_a].pos);
        lines.push_back(shape_a.vertices[it.idx_b].pos);
    }*//*
    for (auto& f : shape_a.faces) {
        for (auto& e : f.simple_edges) {
            lines.push_back(shape_a.vertices[e.va].pos);
            lines.push_back(shape_a.vertices[e.vb].pos);
        }
    }
    for (auto& f : shape_b.faces) {
        for (auto& e : f.simple_edges) {
            lines.push_back(shape_b.vertices[e.va].pos);
            lines.push_back(shape_b.vertices[e.vb].pos);
        }
    }*//*
    for (auto& f : shape_a.faces) {
        for (int i = 1; i < f.vertices.size(); ++i) {
            lines.push_back(shape_a.vertices[f.vertices[i - 1]].pos);
            lines.push_back(shape_a.vertices[f.vertices[i]].pos);
        }
    }
    for (auto& f : shape_b.faces) {
        for (int i = 1; i < f.vertices.size(); ++i) {
            lines.push_back(shape_b.vertices[f.vertices[i - 1]].pos);
            lines.push_back(shape_b.vertices[f.vertices[i]].pos);
        }
    }*/
    {
        csg_data->lines.vertex_buffer.reset(new gpuBuffer);
        csg_data->lines.vertex_buffer->setArrayData(lines.data(), lines.size() * sizeof(lines[0]));
        gpuMeshDesc mesh_desc;
        mesh_desc.setDrawMode(MESH_DRAW_MODE::MESH_DRAW_LINES);
        mesh_desc.setAttribArray(VFMT::Position_GUID, csg_data->lines.vertex_buffer.get(), 0);
        mesh_desc.setVertexCount(lines.size());
        gpuMakeMeshShaderBinding(&csg_data->lines.mesh_binding, csg_data->prog_line.get(), &mesh_desc, 0);
    }

    struct Polygon {
        Shape* shape = 0;
        std::vector<uint32_t> indices;
        gfxm::vec3 N;
    };
    std::vector<Polygon> polygons;
    /*for (auto& it_shape : shape_data)*/ {
        //Shape* pshape = (Shape*)it_shape.first;
        //auto& edges_per_face = it_shape.second.edges_per_face;
        Shape* pshape = &shape_b;
        auto& edges_per_face = shape_data[pshape].edges_per_face;

        for(auto& it_face : edges_per_face) {
            polygons.emplace_back();
            Polygon& poly = polygons.back();
            poly.shape = pshape;

            auto face_id = it_face.first;
            auto edge_list = it_face.second;

            Edge2 edge = *edge_list.begin();
            edge_list.erase(edge_list.begin());
            int head = edge.a;
            int tail = edge.b;
            poly.indices.push_back(head);
            auto it_edge = edge_list.begin();
            while (it_edge != edge_list.end() && head != tail) {
                if (tail == it_edge->a) {
                    tail = it_edge->b;
                    poly.indices.push_back(it_edge->a);
                    edge_list.erase(it_edge);
                    it_edge = edge_list.begin();
                    continue;
                } else if (tail == it_edge->b) {
                    tail = it_edge->a;
                    poly.indices.push_back(it_edge->b);
                    edge_list.erase(it_edge);
                    it_edge = edge_list.begin();
                    continue;
                }
                ++it_edge;
            }
            assert(poly.indices.size() >= 3);
            Shape* shape_ptr = pshape;
            gfxm::vec3 faceN = shape_ptr->faces[face_id].normal;
            gfxm::vec3 N = gfxm::normalize(
                gfxm::cross(pshape->vertices[poly.indices[1]].pos - pshape->vertices[poly.indices[0]].pos, pshape->vertices[poly.indices[2]].pos - pshape->vertices[poly.indices[0]].pos)
            );
            if (gfxm::dot(faceN, N) > 0) {
                std::reverse(poly.indices.begin(), poly.indices.end());
            }
            poly.N = -faceN;
        }
    }

    std::vector<gfxm::vec3> poly_vertices;
    std::vector<gfxm::vec3> poly_normals;
    std::vector<uint32_t> poly_indices;
    for (auto& p : polygons) {
        assert(p.indices.size() >= 3);
        std::vector<uint32_t> tri_indices;
        gfxm::vec3 N = p.N;
        int base_index = poly_vertices.size();
        for (int i = 0; i < p.indices.size(); ++i) {
            poly_vertices.push_back(p.shape->vertices[p.indices[i]].pos);
            poly_normals.push_back(N);
            p.indices[i] = i;
        }

        while (true) {
            if (p.indices.size() == 3) {
                tri_indices.push_back(base_index + p.indices[0]);
                tri_indices.push_back(base_index + p.indices[1]);
                tri_indices.push_back(base_index + p.indices[2]);
                p.indices.clear();
                break;
            }
            uint32_t idx0 = base_index + p.indices[0];
            uint32_t idx1 = base_index + p.indices[1];
            uint32_t idx2 = base_index + p.indices[2];
            tri_indices.push_back(idx0);
            tri_indices.push_back(idx1);
            tri_indices.push_back(idx2);
            p.indices.erase(p.indices.begin() + 1);
        }      

        poly_indices.insert(poly_indices.end(), tri_indices.begin(), tri_indices.end());
    }

    /*
    csg_data->vertex_buffer->setArrayData(poly_vertices.data(), poly_vertices.size() * sizeof(poly_vertices[0]));
    csg_data->normal_buffer->setArrayData(poly_normals.data(), poly_normals.size() * sizeof(poly_normals[0]));
    csg_data->index_buffer->setArrayData(poly_indices.data(), poly_indices.size() * sizeof(poly_indices[0]));
    gpuMeshDesc mesh_desc;
    mesh_desc.setDrawMode(MESH_DRAW_MODE::MESH_DRAW_TRIANGLES);
    mesh_desc.setAttribArray(VFMT::Position_GUID, csg_data->vertex_buffer.get(), 0);
    mesh_desc.setAttribArray(VFMT::Normal_GUID, csg_data->normal_buffer.get(), 0);
    //mesh_desc.setVertexCount(lines.size());
    mesh_desc.setIndexArray(csg_data->index_buffer.get());

    csg_data->mesh_binding = csg_data->prog->getMeshBinding(&mesh_desc, 0);*/

    {
        csg_data->point.points = points;
    }

    {
        std::vector<uint32_t> indices;
        std::vector<gfxm::vec3> vertices;
        std::vector<gfxm::vec3> normals;
        int base_index = vertices.size();
        for (auto& f : shape_a.faces) {
            vertices.insert(vertices.end(), f.triangulated_vertices.begin(), f.triangulated_vertices.end());
            for (int i = 0; i < f.triangulated_normals.size(); ++i) {
                normals.push_back(f.triangulated_normals[i]);
            }
            //std::reverse(f.triangulated_indices.begin(), f.triangulated_indices.end());
            for (int i = 0; i < f.triangulated_indices.size(); ++i) {
                indices.push_back(base_index + f.triangulated_indices[i]);
            }
            base_index = vertices.size();
        }
        for (auto& f : shape_b.faces) {
            vertices.insert(vertices.end(), f.triangulated_vertices.begin(), f.triangulated_vertices.end());
            for (int i = 0; i < f.triangulated_normals.size(); ++i) {
                normals.push_back(-f.triangulated_normals[i]);
            }
            std::reverse(f.triangulated_indices.begin(), f.triangulated_indices.end());
            for (int i = 0; i < f.triangulated_indices.size(); ++i) {
                indices.push_back(base_index + f.triangulated_indices[i]);
            }
            base_index = vertices.size();
        }
        for (auto& idx : poly_indices) {
            indices.push_back(base_index + idx);
        }
        vertices.insert(vertices.end(), poly_vertices.begin(), poly_vertices.end());
        normals.insert(normals.end(), poly_normals.begin(), poly_normals.end());
        /*
        csg_data->vertex_buffer->setArrayData(vertices.data(), vertices.size() * sizeof(vertices[0]));
        csg_data->normal_buffer->setArrayData(normals.data(), normals.size() * sizeof(normals[0]));
        csg_data->index_buffer->setArrayData(indices.data(), indices.size() * sizeof(indices[0]));
        gpuMeshDesc mesh_desc;
        mesh_desc.setDrawMode(MESH_DRAW_MODE::MESH_DRAW_TRIANGLES);
        mesh_desc.setAttribArray(VFMT::Position_GUID, csg_data->vertex_buffer.get(), 0);
        mesh_desc.setAttribArray(VFMT::Normal_GUID, csg_data->normal_buffer.get(), 0);
        //mesh_desc.setVertexCount(lines.size());
        mesh_desc.setIndexArray(csg_data->index_buffer.get());

        csg_data->mesh_binding = csg_data->prog->getMeshBinding(&mesh_desc, 0);*/
    }
}
void csgCleanup() {
    assert(csg_data);
    delete csg_data;
}

void csgDraw(const gfxm::mat4& view, const gfxm::mat4& proj) {
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    //glBindFramebuffer(GL_FRAMEBUFFER, 0);
    //glClear(GL_DEPTH_BUFFER_BIT);

    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glUseProgram(csg_data->prog->getId());
    gfxm::mat4 model = gfxm::mat4(1.f);
    csg_data->prog->setUniformMatrix4("matProjection", proj);
    csg_data->prog->setUniformMatrix4("matModel", model);
    csg_data->prog->setUniformMatrix4("matView", view);

    for (int i = 0; i < csg_data->meshes.size(); ++i) {
        auto& mesh = csg_data->meshes[i];
        gpuBindMeshBinding(&mesh->mesh_binding);

        if (mesh->material == 0) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, 0);
        } else {
            auto tex = mesh->material->texture.get();
            if (tex) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, tex->getId());
            } else {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, 0);
            }
        }

        gpuDrawMeshBinding(&mesh->mesh_binding);
    }
    /*
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glUseProgram(csg_data->prog_line->getId());
    csg_data->prog_line->setUniformMatrix4("matProjection", proj);
    csg_data->prog_line->setUniformMatrix4("matModel", model);
    csg_data->prog_line->setUniformMatrix4("matView", view);
    gpuBindMeshBinding(csg_data->mesh_binding);
    gpuDrawMeshBinding(csg_data->mesh_binding);
    */
    /*
    glUseProgram(csg_data->prog_line->getId());
    csg_data->prog_line->setUniformMatrix4("matProjection", proj);
    csg_data->prog_line->setUniformMatrix4("matView", view);
    gpuBindMeshBinding(csg_data->point.mesh_binding);
    for (int i = 0; i < csg_data->point.points.size(); ++i) {
        auto& pt = csg_data->point.points[i];
        gfxm::mat4 model = gfxm::translate(gfxm::mat4(1.f), pt);
        csg_data->prog_line->setUniformMatrix4("matModel", model);
        gpuDrawMeshBinding(csg_data->point.mesh_binding);
    }*/
    
    {
        gfxm::mat4 model = gfxm::mat4(1.f);
        csg_data->prog_line->setUniformMatrix4("matModel", model);
        gpuBindMeshBinding(&csg_data->lines.mesh_binding);
        gpuDrawMeshBinding(&csg_data->lines.mesh_binding);
    }

    glDeleteVertexArrays(1, &vao);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}