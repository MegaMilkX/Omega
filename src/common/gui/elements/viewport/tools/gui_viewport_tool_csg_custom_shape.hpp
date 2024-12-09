#pragma once

#include "gui/elements/viewport/gui_viewport.hpp"
#include "csg/csg.hpp"
#include "tools_common.hpp"


class GuiViewportToolCsgCreateCustomShape : public GuiViewportToolBase {
    csgScene* csg_scene = 0;

    enum STATE {
        STATE_DRAW,
        STATE_HEIGHT
    };
    STATE state = STATE_DRAW;

    const float snap_step = .125f;

    gfxm::vec3 cursor3d_pos = gfxm::vec3(0, 0, 0);
    gfxm::vec3 cursor3d_normal = gfxm::vec3(0, 1, 0);
    gfxm::mat3 cursor3d_orient = gfxm::mat3(1.f);

    gfxm::vec3 plane_normal = gfxm::vec3(0, 1, 0);
    float plane_offset = .0f;
    gfxm::mat3 plane_orient = gfxm::mat3(1.f);
    gfxm::vec3 plane_origin = gfxm::vec3(0, 0, 0);

    std::vector<gfxm::vec3> points;
    float height = 1.f;

    bool is_ok_to_draw = true;

    bool isCCW(const gfxm::vec3* points, int count, const gfxm::vec3& refN, const gfxm::mat3& refOrient) {
        int idx = 0;
        gfxm::vec2 corner = gfxm::vec2(0, 0); // First point is always at 0, 0 since we take it as origin
        for (int i = 1; i < count; ++i) {
            gfxm::vec2 pt = gfxm::project_point_xy(refOrient, points[0], points[i]);
            if (pt.y > corner.y) {
                continue;
            }
            if (pt.y < corner.y) {
                corner = pt;
                idx = i;
                continue;
            }
            if (pt.x > corner.x) {
                corner = pt;
                idx = i;
                continue;
            }
        }

        gfxm::vec3 A = points[(idx - 1) % count];
        gfxm::vec3 B = points[idx];
        gfxm::vec3 C = points[(idx + 1) % count];

        gfxm::vec3 edgeAB = (B - A);
        gfxm::vec3 edgeBC = (C - B);
        gfxm::vec3 cross = gfxm::cross(edgeAB, edgeBC);
        float d = gfxm::dot(cross, refN);
        if (d > .0f) {
            LOG_WARN("CCW");
            return true;
        } else if(d < .0f) {
            LOG_WARN("CW");
            return false;
        } else {
            LOG_ERR("d == .0f");
            assert(false);
            return false;
        }
    }

    // Returns first concave corner that is followed by a convex one, or the first one if none of them are concave
    int findConcaveCorner(const gfxm::vec3* points, int count, const gfxm::vec3& refN, bool is_ccw, bool* out_is_convex) {
        gfxm::vec3 N = is_ccw ? refN : -refN;
        gfxm::vec3 A = points[count - 1];
        int prev_concave_index = -1;
        for (int i = 0; i < count + 1; ++i) {
            gfxm::vec3 B = points[i % count];
            gfxm::vec3 C = points[(i + 1) % count];

            gfxm::vec3 edgeAB = (B - A);
            gfxm::vec3 edgeBC = (C - B);

            gfxm::vec3 cross = gfxm::cross(edgeAB, edgeBC);
            float len = cross.length();
            float d = gfxm::dot(cross, N);

            if (d > FLT_EPSILON) {
                // Convex corner
                LOG_DBG("Convex d: " << d << ", l: " << len);
                if (prev_concave_index >= 0) {
                    LOG_WARN("Next concave: " << prev_concave_index);
                    if (out_is_convex) {
                        *out_is_convex = false;
                    }
                    return prev_concave_index;
                }
            } else {
                // Not convex or nearly coplanar, which we should split too
                LOG_ERR("CONCAVE d: " << d << ", l: " << len);
                prev_concave_index = i % count;
                //return i;
            }

            A = B;
        }
        LOG_WARN("Next concave (failed to find): " << 0);
        if (out_is_convex) {
            *out_is_convex = true;
        }
        return 0;
    }

    std::vector<int> eatConvexPolygon(std::vector<int>& index_list, const gfxm::vec3* points, int count, const gfxm::vec3& refN, bool is_ccw) {
        assert(count >= 3);
        assert(index_list.size() >= 3);
        const gfxm::vec3 N = is_ccw ? refN : -refN;
        const gfxm::vec3 firstPoint = points[index_list[0]];
        const gfxm::vec3 firstEdge = points[index_list[1]] - firstPoint;
        gfxm::vec3 lastEdge = gfxm::vec3(0,0,0);

        std::vector<int> out;
        out.push_back(index_list[0]);

        int triangle_count = 0;
        while(index_list.size() >= 3) {
            LOG("Triangle count: " << triangle_count);
            int ia = index_list[0];
            int ib = index_list[1];
            int ic = index_list[2];
            const gfxm::vec3 A = points[ia];
            const gfxm::vec3 B = points[ib];
            const gfxm::vec3 C = points[ic];
            const gfxm::vec3 edgeAB = B - A;
            const gfxm::vec3 edgeBC = C - B;
            const gfxm::vec3 edgeCA = A - C;
            const gfxm::vec3 cross = gfxm::cross(edgeAB, edgeBC);
            float d = gfxm::dot(cross, N);

            float d2 = gfxm::dot(gfxm::cross(firstEdge, C - firstPoint), N);
            if (d2 <= .0f) {
                LOG("Convexness broken");
                break;
            }
            if (triangle_count > 0) {
                float d3 = gfxm::dot(gfxm::cross(lastEdge, edgeBC), N);
                if (d3 <= .0f) {
                    LOG("Convexness broken 2");
                    break;
                }
            }

            if (d > FLT_EPSILON) {
                for (int i = 3; i < index_list.size(); ++i) {
                    int idx = index_list[i];
                    gfxm::vec3 P = points[idx];
                    gfxm::vec3 AP = P - A;
                    gfxm::vec3 BP = P - B;
                    gfxm::vec3 CP = P - C;
                    float d0 = gfxm::dot(gfxm::cross(edgeAB, AP), N);
                    float d1 = gfxm::dot(gfxm::cross(edgeBC, BP), N);
                    float d2 = gfxm::dot(gfxm::cross(edgeCA, CP), N);
                    if (d0 >= .0f && d1 >= .0f && d2 >= .0f) {
                        LOG_WARN("Point " << idx << " is inside");
                        if (triangle_count > 0) {
                            out.push_back(index_list[1]);
                        }
                        return out;
                    }
                }
                LOG("Triangle accepted");
                LOG(index_list[0] << ", " << index_list[1] << ", " << index_list[2]);
                ++triangle_count;
                lastEdge = points[index_list[2]] - points[index_list[1]];
                out.push_back(index_list[1]);
                index_list.erase(index_list.begin() + 1);
                if (triangle_count == 2) {
                    //break;
                }
                continue;
            } else {
                LOG("Triangle concave");
                break;
            }
        }
        if (triangle_count > 0) {
            out.push_back(index_list[1]);
        }
        return out;
    }

    void finalizeShape() {
        LOG("Finalizing shape...");
        bool is_ccw = isCCW(points.data(), points.size() - 1, plane_normal, plane_orient);
        //findConcaveCorner(points.data(), points.size() - 1, plane_normal, is_ccw);
        std::vector<gfxm::vec3> points_copy(points.begin(), points.begin() + points.size() - 1);
        std::vector<int> index_list;
        bool is_convex = false;
        int first_idx = findConcaveCorner(points_copy.data(), points_copy.size(), plane_normal, is_ccw, &is_convex);
        for (int i = 0; i < points_copy.size(); ++i) {
            index_list.push_back((first_idx + i) % points_copy.size());
        }

        while (index_list.size() >= 3) {
            if (is_convex) {
                std::vector<gfxm::vec3> poly;
                for (int i = 0; i < index_list.size(); ++i) {
                    poly.push_back(points_copy[index_list[i]]);
                }
                auto ptr = new csgBrushShape;
                csgMakeConvexShape(ptr, poly.data(), poly.size(), height, plane_normal);
                notifyOwner(GUI_NOTIFY::CSG_SHAPE_CREATED, ptr);
                break;
            }

            std::vector<int> result = eatConvexPolygon(index_list, points_copy.data(), points_copy.size(), plane_normal, is_ccw);
            if (result.size() >= 3) {
                std::vector<gfxm::vec3> poly;
                for (int i = 0; i < result.size(); ++i) {
                    poly.push_back(points_copy[result[i]]);
                }

                auto ptr = new csgBrushShape;
                csgMakeConvexShape(ptr, poly.data(), poly.size(), height, plane_normal);
                notifyOwner(GUI_NOTIFY::CSG_SHAPE_CREATED, ptr);

                std::vector<gfxm::vec3> tmp_points_copy = points_copy;
                points_copy.resize(index_list.size());
                for (int i = 0; i < index_list.size(); ++i) {
                    points_copy[i] = tmp_points_copy[index_list[i]];
                }
                index_list.clear();
                first_idx = findConcaveCorner(points_copy.data(), points_copy.size(), plane_normal, is_ccw, &is_convex);
                for (int i = 0; i < points_copy.size(); ++i) {
                    index_list.push_back((first_idx + i) % points_copy.size());
                }
            } else {
                int idx = index_list.front();
                index_list.erase(index_list.begin());
                index_list.push_back(idx);
            }
        }

        state = STATE_DRAW;
        //auto ptr = new csgBrushShape;
        //csgMakeConvexShape(ptr, points.data(), points.size() - 1, height, plane_normal);
        //notifyOwner(GUI_NOTIFY::CSG_SHAPE_CREATED, ptr);
        notifyOwner(GUI_NOTIFY::VIEWPORT_TOOL_DONE, (GuiViewportToolBase*)this);
        points.clear();
    }

public:
    GuiViewportToolCsgCreateCustomShape(csgScene* scene)
    : GuiViewportToolBase("Create custom shape"),
    csg_scene(scene) {}
    

    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::FOCUS:
            return true;
        case GUI_MSG::UNFOCUS:
            return true;
        case GUI_MSG::LCLICK:
        case GUI_MSG::DBL_LCLICK:
            if(state == STATE_DRAW) {
                if(is_ok_to_draw) {
                    if (points.size() >= 3) {
                        gfxm::ray R = viewport->makeRayFromMousePos();
                        if (gfxm::intersect_line_sphere(R.origin, R.origin + R.direction * R.length, points[0], .3f)) {
                            points.push_back(points[0]);
                            state = STATE_HEIGHT;
                        } else {
                            points.push_back(cursor3d_pos);
                        }
                    } else {
                        points.push_back(cursor3d_pos);
                    }
                }
            } else if(state == STATE_HEIGHT) {
                finalizeShape();
            }
            return true;
        case GUI_MSG::RCLICK:
        case GUI_MSG::DBL_RCLICK:
            if (!points.empty()) {
                state = STATE_DRAW;
                points.pop_back();
            } else {
                state = STATE_DRAW;
                notifyOwner(GUI_NOTIFY::VIEWPORT_TOOL_DONE, (GuiViewportToolBase*)this);
            }
            return true;
        }
        return GuiViewportToolBase::onMessage(msg, params);
    };
    
    void onDrawTool(const gfxm::rect& client_area, const gfxm::mat4& proj, const gfxm::mat4& view) override {
        if (points.empty()) {
            CSG_PICK_PARAMS params = { 0 };
            params.csg_scene = csg_scene;
            params.snap_step = snap_step;
            params.viewport = viewport;
            params.out_position = &cursor3d_pos;
            params.out_normal = &cursor3d_normal;
            params.out_orient = &cursor3d_orient;
            guiViewportToolCsgPickSurface(params);

            plane_normal = cursor3d_normal;
            plane_offset = gfxm::dot(cursor3d_normal, cursor3d_pos);
            plane_orient = cursor3d_orient;
            plane_origin = cursor3d_pos;
        } else {
            gfxm::ray R = viewport->makeRayFromMousePos();
            gfxm::intersect_line_plane_point(R.origin, R.direction, plane_normal, plane_offset, cursor3d_pos);

            gfxm::vec2 pos2d = gfxm::project_point_xy(plane_orient, plane_origin, cursor3d_pos);
            const float inv_snap_step = 1.f / snap_step;
            pos2d.x = roundf(pos2d.x * inv_snap_step) * snap_step;
            pos2d.y = roundf(pos2d.y * inv_snap_step) * snap_step;
            if (guiIsModifierKeyPressed(GUI_KEY_SHIFT)) {
                gfxm::vec2 last_pos2d = gfxm::project_point_xy(plane_orient, plane_origin, points.back());
                gfxm::vec2 p2d = pos2d - last_pos2d;
                if (fabsf(p2d.x) > fabsf(p2d.y)) {
                    pos2d.y = last_pos2d.y;
                } else if(fabsf(p2d.y) > fabsf(p2d.x)) {
                    pos2d.x = last_pos2d.x;
                }
            }
            cursor3d_pos = gfxm::unproject_point_xy(pos2d, plane_origin, plane_orient[0], plane_orient[1]);
        }

        // TODO: check if next line intersects any other
        // set is_ok_to_draw to false if it does

        if (state == STATE_HEIGHT) {
            gfxm::ray R = viewport->makeRayFromMousePos();
            gfxm::vec3 A;
            gfxm::vec3 B;
            gfxm::closest_point_line_line(
                R.origin, R.origin + R.direction,
                points.back(), points.back() + plane_normal,
                A, B
            );
            float d = gfxm::dot(B - points.back(), plane_normal);
            const float inv_snap_step = 1.f / snap_step;
            d = roundf(d * inv_snap_step) * snap_step;
            height = d;
        }

        guiPushViewportRect(client_area); // TODO: Do this automatically
        guiPushProjection(proj);
        guiPushViewTransform(view);

        guiViewportToolCsgDrawCursor3d(cursor3d_pos, cursor3d_orient);

        for (int i = 0; i < int(points.size()) - 1; ++i) {
            guiDrawLine3(points[i], points[i + 1], 0xFFFFFFFF);
        }
        if (!points.empty() && state != STATE_HEIGHT) {
            guiDrawLine3(points.back(), cursor3d_pos, is_ok_to_draw ? 0xFFFFFFFF : 0xFF0000FF);
        }
        if (state == STATE_HEIGHT) {
            gfxm::vec3 offset = plane_normal * height;
            for (int i = 0; i < int(points.size()) - 1; ++i) {
                guiDrawLine3(points[i] + offset, points[i + 1] + offset, 0xFFFFFFFF);
            }
            for (int i = 0; i < points.size(); ++i) {
                guiDrawLine3(points[i], points[i] + offset, 0xFFFFFFFF);
            }
        }

        guiPopViewTransform();
        guiPopProjection();
        guiPopViewportRect();

        if (state == STATE_DRAW && points.size() >= 3) {
            gfxm::ray R = viewport->makeRayFromMousePos();
            if (gfxm::intersect_line_sphere(R.origin, R.origin + R.direction * R.length, points[0], .3f)) {
                guiDrawCircle(viewport->getClientArea().min + viewport->worldToClientArea(points[0]), 10.f, false, 0xFF00FF00);
            } else {
                guiDrawCircle(viewport->getClientArea().min + viewport->worldToClientArea(points[0]), 7.f, false, 0xFF00FFFF);
            }
        }
    }
};
