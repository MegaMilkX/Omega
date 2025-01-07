#include "collision_util.hpp"



void fixEdgeCollisionNormal(ContactPoint& cp, int tri, const gfxm::mat4& mesh_transform, const CollisionTriangleMesh* mesh) {
    if (cp.type != CONTACT_POINT_TYPE::TRIANGLE_EDGE) {
        return;
    }

    auto neighbors = mesh->getEdgeNeighbors(tri, cp.edge_idx);
    auto vertex_data = mesh->getVertexData();
    auto index_data = mesh->getIndexData();

    gfxm::vec3 A = mesh_transform * gfxm::vec4(vertex_data[index_data[neighbors.a * 3]], 1.0f);
    gfxm::vec3 B = mesh_transform * gfxm::vec4(vertex_data[index_data[neighbors.a * 3 + 1]], 1.0f);
    gfxm::vec3 C = mesh_transform * gfxm::vec4(vertex_data[index_data[neighbors.a * 3 + 2]], 1.0f);
    gfxm::vec3 Na = gfxm::normalize(gfxm::cross(B - A, C - B));

    A = mesh_transform * gfxm::vec4(vertex_data[index_data[neighbors.b * 3]], 1.0f);
    B = mesh_transform * gfxm::vec4(vertex_data[index_data[neighbors.b * 3 + 1]], 1.0f);
    C = mesh_transform * gfxm::vec4(vertex_data[index_data[neighbors.b * 3 + 2]], 1.0f);
    gfxm::vec3 Nb = gfxm::normalize(gfxm::cross(B - A, C - B));
    if (neighbors.a == neighbors.b) {
        Nb = -Nb;
    }

    dbgDrawArrow(cp.point_b, Na, DBG_COLOR_RED);
    dbgDrawArrow(cp.point_b, Nb, DBG_COLOR_GREEN);
    dbgDrawArrow(cp.point_b, cp.normal_b, DBG_COLOR_BLUE);

    const gfxm::vec3 Ca = gfxm::cross(cp.normal_b, Na);
    const gfxm::vec3 Cb = gfxm::cross(cp.normal_b, Nb);
    if (Ca.length2() > FLT_EPSILON && Cb.length2() > FLT_EPSILON) {
        const gfxm::vec3 Ae = mesh_transform * gfxm::vec4(vertex_data[index_data[tri * 3 + cp.edge_idx]], 1.0f);
        const gfxm::vec3 Be = mesh_transform * gfxm::vec4(vertex_data[index_data[tri * 3 + (cp.edge_idx + 1) % 3]], 1.0f);
        const gfxm::vec3 Ne = gfxm::normalize(Be - Ae);
        float d = gfxm::dot(Ca, Ne);
        float a = acosf(gfxm::dot(Na, cp.normal_b));
        float ab = acosf(gfxm::dot(Na, Nb));
        if (d < .0f) {
            a = -a;
            ab = -ab;
        }
        a = gfxm::clamp(a, gfxm::_min(.0f, ab), gfxm::_max(.0f, ab));
                        
        //dbgDrawText(cp.point_b, std::format("{}", a), DBG_COLOR_BLACK);

        gfxm::vec2 v2(cosf(a), sinf(a));
        cp.normal_b = gfxm::unproject_point_xy(v2, gfxm::vec3(0,0,0), Na, gfxm::normalize(gfxm::cross(Na, Ne)));
        cp.normal_a = -cp.normal_b;
        dbgDrawArrow(cp.point_b + gfxm::vec3(.0f, .1f, .0f), cp.normal_b, DBG_COLOR_BLUE | DBG_COLOR_RED);
    }
}

