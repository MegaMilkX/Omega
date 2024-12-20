#pragma once

#include <unordered_set>
#include "csg_common.hpp"
#include "csg_brush_shape.hpp"
#include "object/csg_object.hpp"
#include "object/csg_group_object.hpp"
#include "object/csg_custom_shape_object.hpp"


class csgScene {
    int next_uid = 0; // replace with uint64_t
    std::unordered_set<csgBrushShape*> invalidated_shapes;
    std::unordered_set<csgBrushShape*> shapes_to_rebuild;

    std::vector<csgBrushShape*> retriangulated_shapes;

    std::unordered_set<csgBrushShape*> shapes;
    std::vector<std::unique_ptr<csgBrushShape>> shape_vec;
    
    std::vector<std::unique_ptr<csgObject>> objects;

    std::map<std::string, csgMaterial*> material_map;
    std::vector<std::unique_ptr<csgMaterial>> materials;

    void updateShapeIntersections(csgBrushShape* shape);
public:

    void            addShape(csgBrushShape* shape, bool keep_uid = false);
    void            removeShape(csgBrushShape* shape);
    int             shapeCount() const;
    csgBrushShape*  getShape(int i);

    void addObject(csgObject* object, bool keep_uid = false);
    void removeObject(csgObject* object);
    int objectCount() const;
    csgObject* getObject(int i);

    csgMaterial*    createMaterial(const char* name);
    void            destroyMaterial(csgMaterial* mat);
    void            destroyMaterial(int i);
    csgMaterial*    getMaterial(const char* name);
    csgMaterial*    getMaterial(int i);

    void invalidateShape(csgBrushShape* shape);
    void markForRebuild(csgBrushShape* shape);
    void update();

    int             retriangulatedShapesCount() const;
    csgBrushShape*  getRetriangulatedShape(int i);
    void            clearRetriangulatedShapes();

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
    int pickFace(
        const gfxm::vec3& from, const gfxm::vec3& to,
        csgBrushShape** out_shape,
        gfxm::vec3& out_hit, gfxm::vec3& out_normal,
        gfxm::vec3& plane_origin, gfxm::mat3& orient
    );

    void serializeJson(nlohmann::json& json);
    bool deserializeJson(const nlohmann::json& json);
};

