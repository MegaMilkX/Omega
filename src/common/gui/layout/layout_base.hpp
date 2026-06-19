#pragma once

#include <optional>

struct gui_layout_context {
    std::optional<int> width;
    std::optional<int> height;
    uint64_t flags = 0;
};

class GuiElement;
class Font;

class GuiLayoutBase {
public:
    virtual ~GuiLayoutBase() {}

    virtual void fontChanged(GuiElement* elem, Font*) = 0;
    virtual int measureWidth(GuiElement* elem, const std::optional<int>& height_constraint) = 0;
    virtual int measureHeight(GuiElement* elem, const std::optional<int>& width_constraint) = 0;
    virtual void layout(GuiElement* elem, const gui_layout_context& ctx) = 0;
};

template<typename ELEM_T>
class GuiLayoutBaseT : public GuiLayoutBase {
public:
    virtual void onFontChanged(ELEM_T* elem, Font* font) = 0;
    virtual int onMeasureWidth(ELEM_T* elem, const std::optional<int>& height_constraint) = 0;
    virtual int onMeasureHeight(ELEM_T* elem, const std::optional<int>& width_constraint) = 0;
    virtual void onLayout(ELEM_T* elem, const gui_layout_context& ctx) = 0;

    void fontChanged(GuiElement* elem, Font* font) override {
        onFontChanged(dynamic_cast<ELEM_T*>(elem), font);
    }
    int measureWidth(GuiElement* elem, const std::optional<int>& height_constraint) override {
        return onMeasureWidth(dynamic_cast<ELEM_T*>(elem), height_constraint);
    }
    int measureHeight(GuiElement* elem, const std::optional<int>& width_constraint) override {
        return onMeasureHeight(dynamic_cast<ELEM_T*>(elem), width_constraint);
    }
    void layout(GuiElement* elem, const gui_layout_context& ctx) override {
        return onLayout(dynamic_cast<ELEM_T*>(elem), ctx);
    }
};

