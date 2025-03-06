#pragma once

#include "gui/gui.hpp"
#include "gui/elements/viewport/tools/gui_viewport_tool_base.hpp"
#include "world/world.hpp"

struct GameRenderInstance {
    RuntimeWorld world;
    gpuRenderTarget* render_target;
    gpuRenderBucket* render_bucket;
    gfxm::mat4 view_transform;
    gfxm::mat4 projection;
};


#include "math/intersection.hpp"


class GuiViewport : public GuiElement {
    std::list<GuiViewportToolBase*> tools;
    bool hide_tools = false;
    bool drag_drop_highlight = false;
    gfxm::mat4 view_transform = gfxm::mat4(1.f);
public:
    float fov = 65.f;
    gfxm::mat4 projection = gfxm::perspective(gfxm::radian(65.0f), 16.f / 9.f, 0.01f, 1000.0f);

    bool is_ortho = false;

    gfxm::vec2 last_mouse_pos;
    float cam_angle_y = .0f;
    float cam_angle_x = .0f;
    float zoom = 2.f;
    gfxm::vec3 cam_pivot = gfxm::vec3(0, 1, 0);
    bool cam_dragging = false;

    GameRenderInstance* render_instance = 0;

    GuiViewport() {
        setSize(gui::perc(100), gui::perc(100));
    }

    void addTool(GuiViewportToolBase* tool) {
        tool->setViewport(this);
        tool->setParent(this);
        tools.push_front(tool);
        guiSetFocusedWindow(tool);
    }
    void removeTool(GuiViewportToolBase* tool) {
        for (auto it = tools.begin(); it != tools.end(); ++it) {
            if ((*it) == tool) {
                tools.erase(it);
                if (guiGetFocusedWindow() == tool) {
                    guiSetFocusedWindow(this);
                }
                tool->setParent(0);
                tool->setViewport(0);
                break;
            }
        }
    }
    void clearTools() {
        for (auto& tool : tools) {
            tool->setParent(0);
            tool->setViewport(0);
        }
        tools.clear();
    }

    void setCameraPivot(const gfxm::vec3& new_pivot, float new_zoom) {
        cam_pivot = new_pivot;
        zoom = new_zoom;
    }

    gfxm::ray makeRayFromMousePos() {
        gfxm::mat4& proj = projection;
        gfxm::vec2 mouse = last_mouse_pos - client_area.min;
        gfxm::ray R = gfxm::ray_viewport_to_world(
            client_area.max - client_area.min, gfxm::vec2(mouse.x, (client_area.max.y - client_area.min.y) - mouse.y),
            proj, render_instance->view_transform
        );
        return R;
    }
    gfxm::vec2 worldToClientArea(const gfxm::vec3& world) {
        gfxm::vec4 screen4 = projection * view_transform * gfxm::vec4(world, 1.f);
        if (screen4.w != .0f) {
            screen4 /= screen4.w;
        }
        gfxm::vec2 screen2 = (gfxm::vec2(screen4.x, -screen4.y) + gfxm::vec2(1.f, 1.f)) * .5f;
        screen2 *= gfxm::vec2(client_area.max.x - client_area.min.x, client_area.max.y - client_area.min.y);
        return screen2;
    }

    const gfxm::mat4& getViewTransform() const {
        return view_transform;
    }

    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::DRAG_START:
            if (guiDragGetPayload()->type == GUI_DRAG_FILE) {
                std::filesystem::path path = *(std::string*)guiDragGetPayload()->payload_ptr;

                drag_drop_highlight = true;
                hide_tools = true;
                return true;
                /*if (path.extension().string() == ".mat") {
                    drag_drop_highlight = true;
                    hide_tools = true;
                    return true;
                }*/
            }
            break;
        case GUI_MSG::DRAG_DROP:
            if (drag_drop_highlight) {
                notifyOwner(GUI_NOTIFY::VIEWPORT_DRAG_DROP,
                    (int)(last_mouse_pos.x - client_area.min.x),
                    (int)(last_mouse_pos.y - client_area.min.y)
                );
            }
            return true;
        case GUI_MSG::DRAG_STOP:
            drag_drop_highlight = false;
            hide_tools = false;
            return true;
        case GUI_MSG::FOCUS:
            return true;
        case GUI_MSG::UNFOCUS:
            return true;
        case GUI_MSG::MOUSE_MOVE: {
            gfxm::vec2 mouse_pos = gfxm::vec2(params.getA<int32_t>(), params.getB<int32_t>());
            mouse_pos -= getGlobalPosition();
            float dx = (mouse_pos.x - last_mouse_pos.x);
            float dy = (mouse_pos.y - last_mouse_pos.y);
            if (cam_dragging) {
                if (guiIsModifierKeyPressed(GUI_KEY_SHIFT)) {
                    cam_angle_x -= dy * .35f;
                    cam_angle_y -= dx * .35f;
                } else {
                    gfxm::mat4 m = gfxm::inverse(render_instance->view_transform);
                    cam_pivot += gfxm::vec3(m[0]) * -dx * .01f * (zoom + 1.f) * .20f;
                    cam_pivot += gfxm::vec3(m[1]) * dy * .01f * (zoom + 1.f) * .20f;
                }
            }
            last_mouse_pos = mouse_pos;
            notifyOwner(
                GUI_NOTIFY::VIEWPORT_MOUSE_MOVE,
                (int)(last_mouse_pos.x - client_area.min.x),
                (int)(last_mouse_pos.y - client_area.min.y)
            );
            if (drag_drop_highlight) {
                notifyOwner(
                    GUI_NOTIFY::VIEWPORT_DRAG_DROP_HOVER,
                    (int)(last_mouse_pos.x - client_area.min.x),
                    (int)(last_mouse_pos.y - client_area.min.y)
                );
            }
            /*
            for (auto& tool : tools) {
                tool->onMouseMove(last_mouse_pos - client_area.min);
            }*/
            } return true;
        case GUI_MSG::MOUSE_SCROLL: {
            float dz = (zoom + 1.f) * .2f;
            zoom -= params.getA<int32_t>() * dz * 0.01f;
            zoom = gfxm::_max(.0f, zoom);
            return true;
        }
        case GUI_MSG::MBUTTON_DOWN:
            cam_dragging = true;
            guiCaptureMouse(this);
            return true;
        case GUI_MSG::MBUTTON_UP:
            cam_dragging = false;
            guiCaptureMouse(0);
            return true;
        case GUI_MSG::LCLICK:
            notifyOwner(
                GUI_NOTIFY::VIEWPORT_LCLICK,
                (int)(last_mouse_pos.x - client_area.min.x),
                (int)(last_mouse_pos.y - client_area.min.y)
            );
            /*
            for (auto& tool : tools) {
                tool->onLClick(last_mouse_pos);
            }*/
            return true;
        case GUI_MSG::RCLICK:
            notifyOwner(
                GUI_NOTIFY::VIEWPORT_RCLICK,
                (int)(last_mouse_pos.x - client_area.min.x),
                (int)(last_mouse_pos.y - client_area.min.y)
            );
            /*
            for (auto& tool : tools) {
                tool->onRClick(last_mouse_pos);
            }*/
            return true;
        case GUI_MSG::KEYDOWN: {
            switch (params.getA<uint16_t>()) {
            case 90: // Z key
                setCameraPivot(gfxm::vec3(0,0,0), 2.f);
                return true;
            }
            break;
        }
        }
        return false;
    }

    void onHitTest(GuiHitResult& hit, int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return;
        }

        if (!hide_tools) {
            for (auto& tool : tools) {
                tool->hitTest(hit, x, y);
                if (hit.hasHit()) {
                    return;
                }
            }
        }

        hit.add(GUI_HIT::CLIENT, this);
        return;
    }
    void onLayout(const gfxm::vec2& extents, uint64_t flags) override {
        rc_bounds = gfxm::rect(gfxm::vec2(0, 0), extents);
        client_area = rc_bounds;
        if (render_instance) {
            gfxm::vec2 vpsz = client_area.max - client_area.min;
            if (render_instance->render_target->getWidth() != vpsz.x
                || render_instance->render_target->getHeight() != vpsz.y) {
                render_instance->render_target->setSize(vpsz.x, vpsz.y);
            }

            if (!is_ortho) {
                projection = gfxm::perspective(gfxm::radian(65.0f), vpsz.x / vpsz.y, 0.01f, 1000.0f);
            } else {
                float wh_ratio = vpsz.x / vpsz.y;
                float width = 20.f * zoom;
                float height = width / wh_ratio;
                projection = gfxm::ortho(-width * .5f, width * .5f, -height * .5f, height * .5f, 0.01f, 1000.0f);
            }
            render_instance->projection = projection;

            gfxm::quat qx = gfxm::angle_axis(gfxm::radian(cam_angle_x), gfxm::vec3(1, 0, 0));
            gfxm::quat qy = gfxm::angle_axis(gfxm::radian(cam_angle_y), gfxm::vec3(0, 1, 0));
            gfxm::quat q = qy * qx;
            gfxm::mat4 m = gfxm::translate(gfxm::mat4(1.f), cam_pivot) * gfxm::to_mat4(q);
            m = gfxm::translate(m, gfxm::vec3(0, 0, 1) * zoom);
            this->view_transform = gfxm::inverse(m);
            render_instance->view_transform = this->view_transform;

            render_instance->render_bucket->addLightDirect(-m[2], gfxm::vec3(1, 1, 1), 1.f);

            {
                // Calculating world pos
                gfxm::vec2 mouse = last_mouse_pos - client_area.min;
                const gfxm::mat4& proj = projection;
                gfxm::mat4 m4 
                    = projection
                    * render_instance->view_transform;
                m4 = gfxm::inverse(m4);
                float half_w = (client_area.max.x - client_area.min.x) * .5f;
                float half_h = (client_area.max.y - client_area.min.y) * .5f;
                gfxm::vec4 mp(
                    (mouse.x - half_w) / half_w,
                    -(mouse.y - half_h) / half_h,
                    -.9f, 1.f
                );
                mp = m4 * mp;
                mp = mp / mp.w;
                //world_pos = mp;

                /*
                gfxm::ray R = gfxm::ray_viewport_to_world(
                    client_area.max - client_area.min, gfxm::vec2(mouse.x, (client_area.max.y - client_area.min.y) - mouse.y),
                    proj, render_instance->view_transform
                );
                
                if (gfxm::intersect_line_plane_point(R.origin, R.direction, gfxm::vec3(0, 1, 0), .0f, world_pos)) {
                    // ...
                }
                
                const float snap_step = .25f;
                const float inv_snap_step = 1.f / snap_step;
                world_pos.x = roundf(world_pos.x * inv_snap_step) * snap_step;
                world_pos.y = roundf(world_pos.y * inv_snap_step) * snap_step;
                world_pos.z = roundf(world_pos.z * inv_snap_step) * snap_step;*/
            }

            for (auto& tool : tools) {
                tool->projection = projection;
                tool->view = render_instance->view_transform;
                tool->layout_position = client_area.min;
                tool->layout(gfxm::rect_size(client_area), 0);
            }
        }
    }
    void onDraw() override {
        Font* font = getFont();

        if (render_instance) {
            // TODO: Handle double buffered
            guiDrawRectTextured(client_area, render_instance->render_target->getTexture("Final"), GUI_COL_WHITE);

            float tool_name_offs = .0f;
            for (auto& tool : tools) {
                tool->onDrawTool(client_area, projection, render_instance->view_transform);
                guiDrawText(
                    client_area.min + gfxm::vec2(GUI_MARGIN, GUI_MARGIN + tool_name_offs),
                    tool->getToolName(),
                    font, 0, 0xFFFFFFFF
                );
                tool_name_offs += GUI_MARGIN;
            }
        } else {
            guiDrawRect(client_area, 0xFF000000);
            guiDrawText(
                client_area.min + gfxm::vec2(GUI_MARGIN, GUI_MARGIN),
                "No render instance",
                font, 0, 0xFFFFFFFF
            );
        }
        if (drag_drop_highlight) {
            gfxm::rect rc = client_area;
            gfxm::expand(rc, -10.f);
            guiDrawRectLine(rc, GUI_COL_TIMELINE_CURSOR);
        }
    }
};
