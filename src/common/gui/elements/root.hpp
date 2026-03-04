#pragma once

#include "gui/elements/element.hpp"
#include "gui/elements/menu_bar.hpp"
#include "gui/elements/window_layer.hpp"
#include "gui/elements/window_layer2.hpp"
#include "gui/elements/popup_layer.hpp"
#include "gui/elements/title_bar.hpp"


class GuiMenuBar;
class GuiDockSpace;
class GuiRoot : public GuiElement {
    GuiElement* menu_box = 0;
    GuiElement* content_box = 0;

    std::unique_ptr<GuiMenuBar> menu_bar;
    std::unique_ptr<GuiDockSpace> dock_space;
    std::unique_ptr<GuiWindowLayer> window_layer;
    std::unique_ptr<GuiPopupLayer> popup_layer;

    RHSHARED<gpuTexture2d> background_texture;
public:
    GuiRoot();

    GuiMenuBar* getMenuBar();
    GuiDockSpace* getDockSpace();
    GuiPopupLayer* getPopupLayer();

    void onHitTest(GuiHitResult& hit, int x, int y) override;
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override;
    void onLayout(const gfxm::vec2& extents, uint64_t flags) override;
    void onDraw() override;

    void addChild(GuiElement* elem) override;
    void removeChild(GuiElement* elem) override;
};