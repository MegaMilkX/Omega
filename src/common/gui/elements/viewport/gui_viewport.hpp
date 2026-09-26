#pragma once

#include "gui/gui.hpp"
#include "gui/elements/zstack.hpp"
#include "gui/elements/viewport/tools/gui_viewport_tool_base.hpp"
#include "world/world.hpp"
#include "gizmo/gizmo.hpp"

#include "math/intersection.hpp"


class GuiViewport : public GuiZStack {
    EngineRenderView* render_view = nullptr;

    GuiZStack* tool_stack = nullptr;
    GuiElement* overlay = nullptr;

    std::list<GuiViewportToolBase*> tools;
    bool hide_tools = false;
    bool drag_drop_highlight = false;
    gfxm::mat4 view_transform = gfxm::mat4(1.f);

    void updateOverlay() {
        overlay->clearChildren();
        for (auto tool : tools) {
            overlay->pushBack(guiCreate<GuiTextElement>(tool->getToolName()));
        }
    }

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

    std::unique_ptr<GizmoContext, void(*)(GizmoContext*)> gizmo_ctx = std::unique_ptr<GizmoContext, void(*)(GizmoContext*)>(
        gizmoCreateContext(), &gizmoReleaseContext
    );

    GuiViewport() {
        render_view = gpuGetPipeline()->createOffscreenView(RendererType::Default, 640, 480);
        render_view->addQueryInterface(gizmo_ctx.get());

        setSize(gui::fill(), gui::fill());

        tool_stack = pushBack(guiCreate<GuiZStack>());
        overlay = pushBack(guiCreate<GuiElement>());
        overlay->addFlags(GUI_FLAG_NO_HIT);

        subscribe([this](const GuiEvt_Focus& e) {
            e.new_focused = this;
        });

        subscribe([this](const GuiEvt_KeyDown& e) {
            switch (e.vkey) {
            case 90: // Z key
                setCameraPivot(gfxm::vec3(0,0,0), 2.f);
                return;
            }
            e.consume = false;
            e.invoke_next();
        });

        subscribe([this](const GuiEvt_MouseBtn& e) {
            if (e.btn == GUI_MOUSE_MID) {
                if (e.state == GUI_KEY_DOWN) {
                    cam_dragging = true;
                    guiCaptureMouse(this);
                } else if (e.state == GUI_KEY_UP) {
                    cam_dragging = false;
                    guiCaptureMouse(0);
                }
            }
        });
        subscribe([this](const GuiEvt_MouseMove& e) {
            gfxm::vec2 mouse_pos = gfxm::vec2(e.x, e.y);
            mouse_pos = guiConvertToLocal(this, mouse_pos);

            float dx = (mouse_pos.x - last_mouse_pos.x);
            float dy = (mouse_pos.y - last_mouse_pos.y);
            if (cam_dragging) {
                if (guiIsModifierKeyPressed(GUI_KEY_SHIFT)) {
                    cam_angle_x -= dy * .35f;
                    cam_angle_y -= dx * .35f;
                } else {
                    gfxm::mat4 m = gfxm::inverse(render_view->getViewTransform());
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
        });
        subscribe([this](const GuiEvt_DragStart& e) {
            auto payload = guiDragGetPayload<GuiStringDDPayload>();
            if (payload) {
                std::filesystem::path path = payload->string;

                drag_drop_highlight = true;
                hide_tools = true;
            }
        });
        subscribe([this](const GuiEvt_DragStop& e) {
            drag_drop_highlight = false;
            hide_tools = false;
        });
        subscribe([this](const GuiEvt_DragDrop& e) {
            if (drag_drop_highlight) {
                notifyOwner(GUI_NOTIFY::VIEWPORT_DRAG_DROP,
                    (int)(last_mouse_pos.x - client_area.min.x),
                    (int)(last_mouse_pos.y - client_area.min.y)
                );
            }
        });
    }
    ~GuiViewport() {
        gpuGetPipeline()->destroyView(render_view);
    }

    EngineRenderView* getRenderView() const { return render_view; }

    void addTool(GuiViewportToolBase* tool) {
        tool_stack->pushBack(tool);
        tools.push_front(tool);
        tool->setViewport(this);
        guiSetFocusedWindow(tool);
        updateOverlay();
    }
    void removeTool(GuiViewportToolBase* tool) {
        for (auto it = tools.begin(); it != tools.end(); ++it) {
            if ((*it) == tool) {
                tools.erase(it);
                if (guiGetFocusedWindow() == tool) {
                    guiSetFocusedWindow(this);
                }
                tool->remove();
                break;
            }
        }
        updateOverlay();
    }
    void clearTools() {
        for (auto& tool : tools) {
            tool->remove();
        }
        tools.clear();
        updateOverlay();
    }

    void setCameraPivot(const gfxm::vec3& new_pivot, float new_zoom) {
        cam_pivot = new_pivot;
        zoom = new_zoom;
    }

    gfxm::ray makeRayFromMousePos() {
        gfxm::mat4 proj = render_view->getProjection();
        gfxm::vec2 mouse = last_mouse_pos;
        gfxm::ray R = gfxm::ray_viewport_to_world(
            rc_bounds.max - rc_bounds.min, gfxm::vec2(mouse.x, (rc_bounds.max.y - rc_bounds.min.y) - mouse.y),
            proj, render_view->getViewTransform()
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
    const gfxm::mat4& getView() const {
        return render_view->getViewTransform();
    }
    const gfxm::mat4& getProjection() const {
        return render_view->getProjection();
    }

    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::MOUSE_SCROLL: {
            float dz = (zoom + 1.f) * .2f;
            zoom -= params.getA<int32_t>() * dz * 0.01f;
            zoom = gfxm::_max(.0f, zoom);
            return true;
        }
        }
        return false;
    }

    void layout_2(const gui_layout_context& ctx) override {
        rc_bounds = gfxm::rect(gfxm::vec2(0, 0), gfxm::vec2(ctx.width.value_or(0), ctx.height.value_or(0)));
        client_area = rc_bounds;
        if (render_view) {
            gfxm::vec2 vpsz = rc_bounds.max - rc_bounds.min;
            if (render_view->getRenderTarget()->getWidth() != vpsz.x
                || render_view->getRenderTarget()->getHeight() != vpsz.y)
            {
                render_view->getRenderTarget()->setSize(vpsz.x, vpsz.y);
            }
            /*
            if (!is_ortho) {
                projection = gfxm::perspective(gfxm::radian(65.0f), vpsz.x / vpsz.y, 0.01f, 1000.0f);
            } else {
                float wh_ratio = vpsz.x / vpsz.y;
                float width = 20.f * zoom;
                float height = width / wh_ratio;
                projection = gfxm::ortho(-width * .5f, width * .5f, -height * .5f, height * .5f, 0.01f, 1000.0f);
            }*/
            render_view->setFov(gfxm::radian(65.f));
            render_view->setZNear(.01f);
            render_view->setZFar(1000.f);

            gfxm::quat qx = gfxm::angle_axis(gfxm::radian(cam_angle_x), gfxm::vec3(1, 0, 0));
            gfxm::quat qy = gfxm::angle_axis(gfxm::radian(cam_angle_y), gfxm::vec3(0, 1, 0));
            gfxm::quat q = qy * qx;
            gfxm::mat4 m = gfxm::translate(gfxm::mat4(1.f), cam_pivot) * gfxm::to_mat4(q);
            m = gfxm::translate(m, gfxm::vec3(0, 0, 1) * zoom);
            this->view_transform = gfxm::inverse(m);
            render_view->setView(this->view_transform);

            render_view->getRenderBucket()->addLightDirect(-m[2], gfxm::vec3(1, 1, 1), 1.f);

            for (auto& tool : tools) {
                tool->projection = projection;
                tool->view = render_view->getViewTransform();
            }
        }

        GuiZStack::layout_2(ctx);
    }
    void onDraw() override {
        // TODO: Gizmo drawing should not be done in onDraw
        gizmoClearContext(gizmo_ctx.get());

        if (render_view) {
            // TODO: Handle double buffered
            guiDrawRectTextured(client_area, render_view->getRenderTarget()->getTexture("Final"), GUI_COL_WHITE);

            const gfxm::mat4& proj = render_view->getProjection();
            const gfxm::mat4& view = render_view->getViewTransform();

            for (auto& tool : tools) {
                tool->onDrawTool(client_area, proj, view);
            }
        } else {
            Font* font = getFont();
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

        GuiZStack::onDraw();
    }
};
