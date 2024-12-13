#include "csg_scene.hpp"

#include "math/intersection.hpp"
#include "csg_core.hpp"

void csgScene::updateShapeIntersections(csgBrushShape* shape) {
    std::unordered_set<csgBrushShape*> diff;
    std::unordered_set<csgBrushShape*> new_intersections;
    for (auto other : shapes) {
        if (other == shape) {
            continue;
        }
        if (csgIntersectAabb(shape->aabb, other->aabb)) {
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

void csgScene::addShape(csgBrushShape* shape, bool keep_uid) {
    if (shapes.count(shape)) {
        assert(false);
        return;
    }
    shape->scene = this;
    if (!keep_uid) {
        shape->uid = next_uid++;
    } else {
        next_uid = std::max(next_uid, shape->uid);
    }
    invalidated_shapes.insert(shape);
    shapes.insert(shape);
    shape->index = shape_vec.size();
    shape_vec.push_back(std::unique_ptr<csgBrushShape>(shape));
}
void csgScene::removeShape(csgBrushShape* shape) {
    if (shapes.count(shape) == 0) {
        assert(false);
        return;
    }
    for (auto s : shape->intersecting_shapes) {
        s->intersecting_shapes.erase(shape);
        shapes_to_rebuild.insert(s);
        // vvv This causes all the other touching shapes to rebuild, not good
        //invalidated_shapes.insert(s);
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

void csgScene::addObject(csgObject* object, bool keep_uid) {
    assert(object);

    csgBrushShape* shape = dynamic_cast<csgBrushShape*>(object);
    if (shape) {
        addShape(shape, keep_uid);
        return;
    }

    auto it = std::find_if(
        objects.begin(), objects.end(), [object](const std::unique_ptr<csgObject>& ptr) -> bool {
            return ptr.get() == object;
        }
    );
    if (it != objects.end()) {
        assert(false);
        return;
    }
    objects.push_back(std::unique_ptr<csgObject>(object));
    object->onAdd(this);
    object->scene = this;
    if (!keep_uid) {
        object->uid = next_uid++;
    } else {
        next_uid = std::max(next_uid, object->uid);
    }
}
void csgScene::removeObject(csgObject* object) {
    csgBrushShape* shape = dynamic_cast<csgBrushShape*>(object);
    if (shape) {
        removeShape(shape);
        return;
    }
    
    auto it = std::find_if(
        objects.begin(), objects.end(), [object](const std::unique_ptr<csgObject>& ptr) -> bool {
            return ptr.get() == object;
        }
    );
    if (it == objects.end()) {
        assert(false);
        return;
    }
    std::unique_ptr<csgObject> obj = std::move(*it);
    objects.erase(it);

    obj->onRemove(this);
    obj->scene = 0;
}
int csgScene::objectCount() const {
    return objects.size();
}
csgObject* csgScene::getObject(int i) {
    return objects[i].get();
}

csgMaterial* csgScene::createMaterial(const char* name) {
    auto it = material_map.find(name);
    if (it != material_map.end()) {
        return it->second;
    }
    csgMaterial* m = new csgMaterial;
    m->name = name;
    LOG_DBG("Material: " << name);
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
    invalidated_shapes.insert(shape);
}
void csgScene::markForRebuild(csgBrushShape* shape) {
    shapes_to_rebuild.insert(shape);
}


static const const csgObject* findLCAImpl(const csgObject* object, std::set<const csgObject*>& out) {
    if (object->owner == nullptr) {
        return nullptr;
    }
    if (out.count(object->owner) > 0) {
        return object->owner;
    }
    out.insert(object->owner);
    return findLCAImpl(object->owner, out);
}
static const csgObject* findLCA(const csgObject* a, const csgObject* b) {
    std::set<const csgObject*> owners;
    const csgObject* lca = 0;
    lca = findLCAImpl(a, owners);
    lca = findLCAImpl(b, owners);
    return lca;
}
typedef std::pair<const csgObject*, const csgObject*> object_pair_t;
static object_pair_t findTopmostWithCommonParent(const csgObject* a, const csgObject* b, const csgObject* lca) {
    if (a->owner == lca && b->owner == lca) {
        return std::make_pair(a, b);
    }
    if (a->owner != lca) {
        a = a->owner;
    }
    if (b->owner != lca) {
        b = b->owner;
    }
    return findTopmostWithCommonParent(a, b, lca);
}
static bool compareCsgObjectUids(const csgObject* a, const csgObject* b) {
    if (a->owner == nullptr && b->owner == nullptr) {
        return a->uid < b->uid;
    }
    const csgObject* lca = findLCA(a, b);
    object_pair_t pair = findTopmostWithCommonParent(a, b, lca);

    return pair.first->uid < pair.second->uid;
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
        return compareCsgObjectUids(a, b);
    });
    for (auto shape : shape_vec) {
        shape->intersecting_sorted.clear();
        shape->intersecting_sorted.insert(
            shape->intersecting_sorted.end(), shape->intersecting_shapes.begin(), shape->intersecting_shapes.end()
        );
        std::sort(shape->intersecting_sorted.begin(), shape->intersecting_sorted.end(), [](const csgBrushShape* a, const csgBrushShape* b)->bool {
            return compareCsgObjectUids(a, b);
        });
    }
    for (auto shape : shape_vec) {
        csgRebuildFragments(shape);
    }

    clearRetriangulatedShapes();
    for (auto shape : shape_vec) {
        shape->triangulated_meshes.clear();
        std::vector<std::unique_ptr<csgMeshData>> meshes;
        csgTriangulateShape(shape, meshes);
        shape->triangulated_meshes = std::move(meshes);

        retriangulated_shapes.push_back(shape);
    }

    shapes_to_rebuild.clear();
}

int csgScene::retriangulatedShapesCount() const {
    return retriangulated_shapes.size();
}
csgBrushShape* csgScene::getRetriangulatedShape(int i) {
    return retriangulated_shapes[i];
}
void csgScene::clearRetriangulatedShapes() {
    retriangulated_shapes.clear();
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
                gfxm::vec3 fragN = face.N * (frag.back_volume == CSG_VOLUME_EMPTY ? -1.f : 1.f);
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
                gfxm::vec3 fragN = face.N * (frag.back_volume == CSG_VOLUME_EMPTY ? -1.f : 1.f);
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

        gfxm::vec3 N = shape->faces[i]->N * (shape->volume_type == CSG_VOLUME_EMPTY ? -1.f : 1.f);
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

int csgScene::pickFace(
    const gfxm::vec3& from, const gfxm::vec3& to,
    csgBrushShape** out_shape,
    gfxm::vec3& out_hit, gfxm::vec3& out_normal,
    gfxm::vec3& plane_origin, gfxm::mat3& orient
) {
    csgBrushShape* shape = 0;
    csgFace* hit_face = 0;
    if (!pickShape(from, to, &shape)) {
        return -1;
    }

    *out_shape = shape;
    
    float dist = INFINITY;
    gfxm::vec3 pt;
    int face_id = -1;
    for (int i = 0; i < shape->faces.size(); ++i) {
        auto& face = *shape->faces[i].get();
        float t = .0f;
        if (!gfxm::intersect_ray_plane_t(from, to, face.N, face.D, t)) {
            continue;
        }
        if (t >= dist) {
            continue;
        }

        gfxm::vec3 N = shape->faces[i]->N * (shape->volume_type == CSG_VOLUME_EMPTY ? -1.f : 1.f);
        if (gfxm::dot(N, to - from) > .0f) {
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
            gfxm::vec3 fragN = face.N * (frag.back_volume == CSG_VOLUME_EMPTY ? -1.f : 1.f);
            if (gfxm::dot(fragN, to - from) > .0f) {
                continue;
            }
            if (!isPointInsideConvexFace(tmp_pt, frag.vertices)) {
                continue;
            }

            dist = t;
            out_hit = tmp_pt;
            out_normal = fragN;
            face_id = i;
        }

        hit_face = shape->faces[face_id].get();
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
        if (shape->owner) {
            continue;
        }
        nlohmann::json jshape;
        shape->serializeJson(jshape);
        jshapes.push_back(jshape);
    }

    nlohmann::json& jobjects = json["objects"];
    jobjects = nlohmann::json::array();
    for (auto& object : objects) {
        if (object->owner) {
            continue;
        }
        nlohmann::json jobject;
        object->serializeJson(jobject);
        jobjects.push_back(jobject);
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

    auto itjshapes = json.find("shapes");
    if (itjshapes != json.end()) {
        const nlohmann::json& jshapes = itjshapes.value();
        if (!jshapes.is_array()) {
            assert(false);
            return false;
        }
        int shape_count = jshapes.size();
        for (int i = 0; i < shape_count; ++i) {
            const nlohmann::json& jshape = jshapes[i];
            // TODO: Shape ownership
            csgBrushShape* shape = new csgBrushShape;
            shape->scene = this;
            shape->deserializeJson(jshape);
            addShape(shape, true);
        }
    }

    auto itjobjects = json.find("objects");
    if (itjobjects != json.end()) {
        const nlohmann::json& jobjects = itjobjects.value();
        if (!jobjects.is_array()) {
            assert(false);
            return false;
        }
        int object_count = jobjects.size();
        for (int i = 0; i < object_count; ++i) {
            const nlohmann::json& jobject = jobjects[i];
            std::string type;
            type_read_json(jobject["type"], type);
            csgObject* object = csgCreateObjectFromTypeName(type);
            if (!object) {
                continue;
            }
            object->scene = this;
            object->deserializeJson(jobject);
            object->scene = 0;  // TODO: hack, shapes get materials from scene on deserialization
                                // But groups must not be 'in scene' before being added
            addObject(object, true);
        }
    }

    return true;
}

