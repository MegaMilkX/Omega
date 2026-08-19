#pragma once

#include "gui/elements/element.hpp"


class GuiZStack : public GuiElement {
public:
    GuiZStack();
    int measureWidth(const std::optional<int>& height) override;
    int measureHeight(const std::optional<int>& width) override;
    void layout_2(const gui_layout_context& ctx) override;
};

