#pragma once

#include "gui/elements/element.hpp"


class GuiPopupLayer : public GuiElement {
public:
    GuiPopupLayer();
    void onHitTest(GuiHitResult& hit, int x, int y) override;
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override;
    void onLayout(const gfxm::vec2& extents, uint64_t flags) override;
    void onDraw() override;
};

