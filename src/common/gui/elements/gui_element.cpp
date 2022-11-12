#include "gui/elements/gui_element.hpp"

#include "gui/gui_system.hpp"

bool GuiElement::isHovered() const {
    return guiGetHoveredElement() == this;
}
bool GuiElement::isPressed() const {
    return guiGetPressedElement() == this;
}
bool GuiElement::isPulled() const {
    return guiGetPulledElement() == this;
}

void GuiElement::layout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) {
    if (this->font) { guiPushFont(this->font); }
    onLayout(cursor, rc, flags);
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