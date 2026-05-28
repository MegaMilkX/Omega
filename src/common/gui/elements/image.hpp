#pragma once

#include "gui/elements/element.hpp"
#include "gpu/gpu_texture_2d.hpp"


class GuiImage : public GuiElement {
    gpuTexture2d* texture = 0;
public:
    GuiImage(gpuTexture2d* texture)
        : texture(texture) {
        //float aspect = texture->getWidth() / texture->getHeight();
        setSize(gui::fill(), texture->getHeight());
        //setSize(gui_vec2(texture->getWidth(), texture->getHeight(), gui_pixel));
    }
    void onLayout(const gui_layout_context& ctx) override {
        rc_bounds = gfxm::rect(gfxm::vec2(0, 0), gfxm::vec2(ctx.width.value_or(0), ctx.height.value_or(0)));
        client_area = rc_bounds;
    }

    void onDraw() override {
        guiDrawRectTextured(client_area, texture, GUI_COL_WHITE);
        //guiDrawColorWheel(client_area);
    }
};