#pragma once

#include "gui/elements/element.hpp"
#include "gpu/gpu_texture_2d.hpp"


class GuiImage : public GuiElement {
    gpuTexture2d* texture = 0;
public:
    GuiImage(gpuTexture2d* texture)
        : texture(texture) {
        setSize(gui_vec2(texture->getWidth(), texture->getHeight(), gui_pixel));
    }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        rc_bounds = gfxm::rect(
            rc.min,
            rc.min + gfxm::vec2(texture->getWidth(), texture->getHeight()) + gfxm::vec2(GUI_MARGIN, GUI_MARGIN) * 2.0f
        );
        client_area = rc_bounds;
    }

    void onDraw() override {
        guiDrawRectTextured(client_area, texture, GUI_COL_WHITE);
        //guiDrawColorWheel(client_area);
    }
};