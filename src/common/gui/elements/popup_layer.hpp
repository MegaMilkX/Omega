#pragma once

#include "gui/elements/element.hpp"


class GuiPopupLayer : public GuiElement {
public:
    GuiPopupLayer();
    void onHitTest(GuiHitResult& hit, int x, int y) override;
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override;
    void onLayoutOld(const gui_layout_context& ctx);
    void onDraw() override;
};

