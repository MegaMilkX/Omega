#include "popup_layer.hpp"


GuiPopupLayer::GuiPopupLayer() {
    addFlags(GUI_FLAG_NO_HIT);
}
void GuiPopupLayer::onHitTest(GuiHitResult& hit, int x, int y) {
    GuiElement::onHitTest(hit, x, y);
}

bool GuiPopupLayer::onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) {
    return GuiElement::onMessage(msg, params);
}

void GuiPopupLayer::onLayout(const gui_layout_context& ctx) {
    GuiElement::onLayout(ctx);
}

void GuiPopupLayer::onDraw() {
    GuiElement::onDraw();
}

