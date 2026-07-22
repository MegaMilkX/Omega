#pragma once

#include "gui/elements/window.hpp"
#include "gui/elements/viewport/gui_viewport.hpp"
#include "gizmo/gizmo.hpp"

#include "m3d/m3d_project.hpp"
#include "m3d/skeletal_instance.hpp"


class GuiImportM3dWindow : public GuiWindow {
    GameRenderInstance render_instance;

    GizmoContext* gizmo_ctx = nullptr;

    m3dpProject m3d_proj;
    ResourceRef<m3dModel> m3d;
    std::unique_ptr<m3dSkeletalInstance> m3d_inst;

    std::string project_path;

    // preview
    struct {
        int current_anim = 0;
        float t_anim = .0f;
    };

    // reference plane
    struct {
        gpuMesh mesh;
        std::unique_ptr<gpuRenderable> renderable;
        gpuTransformBlock* transform_block = nullptr;
        ResourceRef<gpuMaterial> material;
    } ref_plane;

    void initFromSource(const std::string& path);
    void initFromProject(const std::string& path);
    void initControls();
public:
    GuiImportM3dWindow(const std::string& path);
    ~GuiImportM3dWindow();

    void onTick(float dt, GUI_TICK_ID id) override;
};