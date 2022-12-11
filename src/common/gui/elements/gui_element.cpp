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
bool GuiElement::hasMouseCapture() const {
    return guiGetMouseCaptor() == this;
}

void GuiElement::layout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) {
    if (is_hidden) {
        return;
    }
    if (this->font) { guiPushFont(this->font); }
    onLayout(cursor, rc, flags);
    if (this->font) { guiPopFont(); }
}
void GuiElement::draw() {
    if (is_hidden) {
        return;
    }
    if (this->font) { guiPushFont(this->font); }
    onDraw();
    if (this->font) { guiPopFont(); }
}