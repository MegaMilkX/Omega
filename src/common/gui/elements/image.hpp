#pragma once

#include "gui/elements/element.hpp"
#include "gpu/gpu_texture_2d.hpp"


class GuiImage : public GuiElement {
    gpuTexture2d* texture = 0;
    int cached_width = 0;
    int cached_height = 0;
public:
    GuiImage(gpuTexture2d* texture)
        : texture(texture) {
        setSize(gui::fill(), gui::content());
    }
    int measureWidth(const std::optional<int>& height) override {
        if(!texture) return 100; // TODO: come up with something more graceful

        const int tex_width = texture->getWidth();
        if (!height.has_value()) {
            cached_width = tex_width;
            return cached_width;
        }
        const int tex_height = texture->getHeight();
        float ratio = height.value() / float(tex_height);
        cached_width = tex_width * ratio;
        return cached_width;
    }
    int measureHeight(const std::optional<int>& width) override {
        if(!texture) return 100; // TODO: come up with something more graceful

        const int tex_height = texture->getHeight();
        if (!width.has_value()) {
            cached_height = tex_height;
            return cached_height;
        }
        const int tex_width = texture->getWidth();
        float ratio = width.value() / float(tex_width);
        cached_height = tex_height * ratio;
        return cached_height;
    }
    void layout_2(const gui_layout_context& ctx) override {
        rc_bounds = gfxm::rect(gfxm::vec2(0, 0), gfxm::vec2(ctx.width.value_or(cached_width), ctx.height.value_or(cached_height)));
        client_area = rc_bounds;
    }

    void onDraw() override {
        guiDrawRectTextured(client_area, texture, GUI_COL_WHITE);
        //guiDrawColorWheel(client_area);
    }
};