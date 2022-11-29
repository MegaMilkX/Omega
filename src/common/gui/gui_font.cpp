#include "gui_font.hpp"

#include <stack>
#include <memory>

static std::unique_ptr<GuiFont> font_global;
static std::stack<GuiFont*>     font_stack;

void guiFontInit(Font* font) {
    font_global.reset(new GuiFont);
    guiFontCreate(*font_global.get(), font);
}
void guiFontCleanup() {
    font_global.reset();
}
void guiPushFont(GuiFont* font) {
    font_stack.push(font);
}
void guiPopFont() {
    font_stack.pop();
}
GuiFont* guiGetCurrentFont() {
    if (font_stack.empty()) {
        return guiGetDefaultFont();
    }
    return font_stack.top();
}
GuiFont* guiGetDefaultFont() {
    return font_global.get();
}