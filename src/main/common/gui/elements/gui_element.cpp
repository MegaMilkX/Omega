#include "common/gui/elements/gui_element.hpp"

#include "common/gui/gui_system.hpp"

bool GuiElement::isHovered() const {
    return guiGetHoveredElement() == this;
}
bool GuiElement::isPressed() const {
    return guiGetPressedElement() == this;
}

void GuiElement::layout(const gfxm::rect& rc, uint64_t flags) {
    if (this->font) { guiPushFont(this->font); }
    onLayout(rc, flags);
    if (this->font) { guiPopFont(); }
}
void GuiElement::draw() {
    if (this->font) { guiPushFont(this->font); }
    onDraw();
    if (this->font) { guiPopFont(); }
}
void GuiElement::sendMessage(GUI_MSG msg, uint64_t a, uint64_t b) {
    onMessage(msg, a, b);
}