#pragma once

#include "gui/elements/element.hpp"


class GuiPopupLayer : public GuiElement {
public:
    GuiPopupLayer();
    void onHitTest(GuiHitResult& hit, int x, int y) override;
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override;
    void onLayoutOld(const gui_layout_context& ctx);
    int measureWidth(const std::optional<int>& height) override;
    int measureHeight(const std::optional<int>& width) override;
    void layout_2(const gui_layout_context& ctx) override;
    void onDraw() override;
};

