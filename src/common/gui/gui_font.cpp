#include "gui_font.hpp"

#include <stack>
#include <memory>

static std::shared_ptr<Font> font_global;
static std::stack<Font*>     font_stack;



void guiFontInit(const std::shared_ptr<Font>& default_font) {
    font_global = default_font;
}
void guiFontCleanup() {
    font_global.reset();
}
void guiPushFont(Font* font) {
    font_stack.push(font);
}
void guiPopFont() {
    font_stack.pop();
}
Font* guiGetCurrentFont() {
    if (font_stack.empty()) {
        return guiGetDefaultFont();
    }
    return font_stack.top();
}
Font* guiGetDefaultFont() {
    return font_global.get();
}