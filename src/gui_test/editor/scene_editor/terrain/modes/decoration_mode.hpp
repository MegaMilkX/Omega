#pragma once

#include "mode.hpp"
#include "m3d/m3d_model.hpp"
#include "resource_manager/resource_ref.hpp"
#include "gpu/renderable/geometry.hpp"
#include "gpu/device.hpp"
#include "gui/elements/viewport/gui_viewport.hpp"


class TerrainDecorationMode : public TerrainEditMode {
    enum class State {
        Hover,
        Angle
    };

    State state = State::Hover;

    bool cursor_hit = false;
    gfxm::vec3 cursor_pos;

    gfxm::vec3 placement_pos;
    float placement_angle = .0f;

    ResourceRef<m3dModel> model;
    struct ActivePreview {
        std::vector<gpuRenderable> renderables;
        std::vector<gpuTransformBlock*> transform_blocks;
    } active_preview;
public:
    TerrainDecorationMode(TerrainSceneSpace* space);

    void onTick(float dt, GUI_TICK_ID id) override;
    void queryGeometry(const GeometryQuery& q) override {
        if(cursor_hit) {
            for (int i = 0; i < active_preview.renderables.size(); ++i) {
                q.bucket->add(&active_preview.renderables[i]);
            }
        }
    }
};

