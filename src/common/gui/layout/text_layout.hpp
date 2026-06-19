#pragma once

#include "layout_base.hpp"


class GuiTextElement;
class GuiTextLayout : public GuiLayoutBaseT<GuiTextElement> {
    // measureWidth/Height calls are (should) always be followed by a layout call
    // these are set to false in corresponding measure calls and reset to true at the end of the layout func
    bool width_constrained = true;
    bool height_constrained = true;
public:
    void onFontChanged(GuiTextElement* elem, Font* font) override;
    int onMeasureWidth(GuiTextElement* elem, const std::optional<int>& height_constraint) override;
    int onMeasureHeight(GuiTextElement* elem, const std::optional<int>& width_constraint) override;
    void onLayout(GuiTextElement* elem, const gui_layout_context& ctx) override;
};

