#include "sat_convexmesh_triangle.hpp"


static void project(const gfxm::vec3* verts, int count, const gfxm::vec3& axis, float& out_min, float& out_max) {
    out_min = out_max = gfxm::dot(verts[0], axis);
    for (int i = 1; i < count; ++i) {
        float p = gfxm::dot(verts[i], axis);
        if(p < out_min) out_min = p;
        if(p > out_max) out_max = p;
    }
}

static bool overlap(float mina, float maxa, float minb, float maxb, float& out_depth) {
    if(maxa < minb || maxb < mina) return false;
    out_depth = gfxm::_min(maxa - minb, maxb - mina);
    return true;
}

bool SAT_ConvexMeshTriangle(const gfxm::mat4& mesh_transform, const phyConvexMesh* mesh, const gfxm::vec3* triangle, phyContactPoint& out_contact) {
    enum AXIS_TYPE {
        TRI_FACE,
        CONVEX_FACE,
        EDGE_EDGE
    };

    float best_depth = std::numeric_limits<float>::infinity();
    gfxm::vec3 best_axis;
    AXIS_TYPE axis_type;
    int feature_a = -1;
    int feature_b = -1;

    const gfxm::vec3* convex_vertices = mesh->getVertexData();
    int vertex_count = mesh->vertexCount();

    auto inverse_mesh_transform = gfxm::inverse(mesh_transform);
    const gfxm::vec3 tri[3] = {
        inverse_mesh_transform * gfxm::vec4(triangle[0], 1.0f),
        inverse_mesh_transform * gfxm::vec4(triangle[1], 1.0f),
        inverse_mesh_transform * gfxm::vec4(triangle[2], 1.0f)
    };
    const gfxm::vec3 tri_centroid = (tri[0] + tri[1] + tri[2]) / 3.f;

    auto fnTestAxis = [&](const gfxm::vec3& axis, int fa, int fb, AXIS_TYPE axis_type, AXIS_TYPE& out_axis_type, bool blank = false)->bool {
        float mina, maxa, minb, maxb, depth;

        project(convex_vertices, vertex_count, axis, mina, maxa);
        project(tri, 3, axis, minb, maxb);

        if (!overlap(mina, maxa, minb, maxb, depth)) {
            return false;
        }

        if (!blank && depth < best_depth) {
            best_depth = depth;
            best_axis = axis;
            out_axis_type = axis_type;
            feature_a = fa;
            feature_b = fb;
        }
        return true;
    };

    // Triangle normal axis
    gfxm::vec3 Nt = gfxm::normalize(gfxm::cross(tri[1] - tri[0], tri[2] - tri[1]));
    if (gfxm::dot(Nt, mesh->getCentroid() - tri_centroid) < .0f) {
        Nt = -Nt;
    }
    if(!fnTestAxis(Nt, 0, 0, TRI_FACE, axis_type, false)) {
        return false;
    }
    
    // Mesh face normal axes
    for (int i = 0; i < mesh->faceCount(); ++i) {
        gfxm::vec3 N = mesh->getFaceNormalData()[i];
        if (gfxm::dot(N, mesh->getCentroid() - tri_centroid) < .0f) {
            N = -N;
        }
        if(!fnTestAxis(N, i, 0, CONVEX_FACE, axis_type, false)) {
            return false;
        }
    }
    
    // mesh edge x triangle edge axes
    const gfxm::vec3 tri_edges[3] = {
        tri[1] - tri[0],
        tri[2] - tri[1],
        tri[0] - tri[2]
    };
    for (int i = 0; i < mesh->uniqueEdgeCount(); ++i) {
        phyConvexMesh::Edge e = mesh->getUniqueEdgeData()[i];
        const gfxm::vec3& mesh_edge = mesh->getVertexData()[e.b] - mesh->getVertexData()[e.a];
        for (int j = 0; j < 3; ++j) {
            gfxm::vec3 axis = gfxm::cross(mesh_edge, tri_edges[j]);
            if(axis.length2() < 1e-8f) continue;
            if (gfxm::dot(axis, mesh->getCentroid() - tri_centroid) < .0f) {
                axis = -axis;
            }
            axis = gfxm::normalize(axis);
            if(!fnTestAxis(axis, i, j, EDGE_EDGE, axis_type)) {
                return false;
            }
        }
    }
    
    // Contact points
    gfxm::vec3 pt_on_mesh;
    gfxm::vec3 pt_on_triangle;
    switch (axis_type) {
    case TRI_FACE: {
        const gfxm::vec3& axis = (best_axis);
        float min_proj = std::numeric_limits<float>::infinity();
        for (int i = 0; i < mesh->vertexCount(); ++i) {
            const gfxm::vec3& v = mesh->getVertexData()[i];
            float p = gfxm::dot(v, axis);
            if (p < min_proj) {
                min_proj = p;
                pt_on_mesh = v;
            }
        }
        float d = gfxm::dot(pt_on_mesh - tri[0], axis);
        pt_on_triangle = pt_on_mesh - axis * d;
        break;
    }
    case CONVEX_FACE: {
        float maxd = -std::numeric_limits<float>::infinity();
        int best_face = -1;
        for (int i = 0; i < mesh->faceCount(); ++i) {
            uint32_t i0 = mesh->getIndexData()[i * 3];
            uint32_t i1 = mesh->getIndexData()[i * 3 + 1];
            uint32_t i2 = mesh->getIndexData()[i * 3 + 2];
            const gfxm::vec3& a = mesh->getVertexData()[i0];
            const gfxm::vec3& b = mesh->getVertexData()[i1];
            const gfxm::vec3& c = mesh->getVertexData()[i2];

            float da = gfxm::dot(a, -best_axis);
            float db = gfxm::dot(b, -best_axis);
            float dc = gfxm::dot(c, -best_axis);
            if (da > maxd && db > maxd && dc > maxd) {
                maxd = gfxm::_min(da, gfxm::_min(db, dc));
                best_face = i;
            }
        }

        int convex_face_idx = best_face;
        const gfxm::vec3& axis = best_axis;
        float max_proj = -std::numeric_limits<float>::infinity();
        for (int i = 0; i < 3; ++i) {
            float p = gfxm::dot(tri[i], axis);
            if (p > max_proj) {
                max_proj = p;
                pt_on_triangle = tri[i];
            }
        }
        const gfxm::vec3& pt_convex_plane = mesh->getVertexData()[convex_face_idx * 3];
        float d = gfxm::dot(pt_on_triangle - pt_convex_plane, axis);
        pt_on_mesh = pt_on_triangle - axis * d;
        break;
    }
    case EDGE_EDGE: {
        float maxd = -std::numeric_limits<float>::infinity();
        int best_edge = -1;
        for (int i = 0; i < mesh->uniqueEdgeCount(); ++i) {
            const auto& edge = mesh->getUniqueEdgeData()[i];
            const gfxm::vec3& a = mesh->getVertexData()[edge.a];
            const gfxm::vec3& b = mesh->getVertexData()[edge.b];
            float da = gfxm::dot(a, -best_axis);
            float db = gfxm::dot(b, -best_axis);
            if (da > maxd && db > maxd) {
                maxd = gfxm::_min(da, db);
                best_edge = i;
            }
        }

        int convex_edge_idx = best_edge;
        int tri_edge_idx = feature_b;
        uint32_t cei0 = mesh->getUniqueEdgeData()[convex_edge_idx].a;
        uint32_t cei1 = mesh->getUniqueEdgeData()[convex_edge_idx].b;
        const gfxm::vec3& A0 = mesh->getVertexData()[cei0];
        const gfxm::vec3& B0 = mesh->getVertexData()[cei1];
        const gfxm::vec3& A1 = tri[tri_edge_idx];
        const gfxm::vec3& B1 = tri[(tri_edge_idx + 1) % 3];
        closestPointSegmentSegment(A0, B0, A1, B1, pt_on_mesh, pt_on_triangle);
        break;
    }
    }

    float axis_len = best_axis.length();
    best_depth /= axis_len;
    const gfxm::vec3& normal = mesh_transform * gfxm::vec4(best_axis / axis_len, .0f);
    pt_on_mesh = mesh_transform * gfxm::vec4(pt_on_mesh, 1.0f);
    pt_on_triangle = mesh_transform * gfxm::vec4(pt_on_triangle, 1.0f);

    //dbgDrawText(pt_on_mesh, std::format("depth: {}", best_depth), 0xFFFFFFFF);

    float depth = gfxm::dot(normal, pt_on_mesh) - gfxm::dot(normal, pt_on_triangle);

    out_contact.depth = depth;
    out_contact.normal_a = normal;
    out_contact.normal_b = -normal;
    out_contact.point_a = pt_on_triangle;
    out_contact.point_b = pt_on_mesh;
    return true;
}

