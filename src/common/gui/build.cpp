#include "build.hpp"

#include <stack>


namespace gui {
namespace build {


struct Style {
    gfxm::vec2 position = gfxm::vec2(-1, -1);
    gfxm::vec2 size = gfxm::vec2(-1, -1);
    GUI_OVERFLOW overflow = GUI_OVERFLOW_NONE;
    gui_flag_t flags = 0;
    gfxm::rect content_padding = gfxm::rect(GUI_PADDING, GUI_PADDING, GUI_PADDING, GUI_PADDING);
};
struct Builder {
    int styles_pushed = 0;
    std::stack<Style> style_stack;
    std::stack<GuiElement*> stack;
    Style& push_style() {
        ++styles_pushed;
        style_stack.push(style_stack.top());
        return style_stack.top();
    }
    Builder() {
        style_stack.push(Style());
    }
};
Builder* get_builder() {
    static Builder b;
    return &b;
}


bool ValidateBuilderState() {
    assert(get_builder()->stack.empty());
    return get_builder()->stack.empty();
}


void Position(float x, float y) {
    get_builder()->push_style().position = gfxm::vec2(x, y);
}
void Size(float x, float y) {
    get_builder()->push_style().size = gfxm::vec2(x, y);
}
void ContentPadding(float left, float top, float right, float bottom) {
    get_builder()->push_style().content_padding = gfxm::rect(left, top, right, bottom);
}
void Flags(gui_flag_t flags) {
    get_builder()->push_style().flags = flags;
}
void Overflow(GUI_OVERFLOW value) {
    get_builder()->push_style().overflow = value;
}


template<typename T>
T* InsertElement(T* e) {
    auto builder = get_builder();
    auto style = builder->style_stack.top();
    e->pos.x = style.position.x >= .0f ? style.position.x : e->pos.x;
    e->pos.y = style.position.y >= .0f ? style.position.y : e->pos.y;
    e->size.x = style.size.x >= .0f ? style.size.x : e->size.x;
    e->size.y = style.size.y >= .0f ? style.size.y : e->size.y;
    e->content_padding = style.content_padding;
    e->overflow = style.overflow;
    e->addFlags(style.flags);
    while (builder->styles_pushed) {
        builder->style_stack.pop();
        --builder->styles_pushed;
    }
    GuiElement* parent = 0;
    GuiElement* owner = 0;
    if (!builder->stack.empty()) {
        parent = builder->stack.top();
        owner = parent;
    }
    guiAdd(parent, owner, e);
    return e;
}
template<typename T>
T* BeginElement(T* e) {
    auto builder = get_builder();
    auto style = builder->style_stack.top();
    e->pos.x = style.position.x >= .0f ? style.position.x : e->pos.x;
    e->pos.y = style.position.y >= .0f ? style.position.y : e->pos.y;
    e->size.x = style.size.x >= .0f ? style.size.x : e->size.x;
    e->size.y = style.size.y >= .0f ? style.size.y : e->size.y;
    e->content_padding = style.content_padding;
    e->overflow = style.overflow;
    e->addFlags(style.flags);
    builder->stack.push(e);
    while (builder->styles_pushed) {
        builder->style_stack.pop();
        --builder->styles_pushed;
    }
    return e;
}
void EndElement() {
    auto builder = get_builder();
    assert(!builder->stack.empty());
    auto elem = builder->stack.top();
    builder->stack.pop();
    GuiElement* parent = 0;
    GuiElement* owner = 0;
    if (!builder->stack.empty()) {
        parent = builder->stack.top();
        owner = parent;
    }
    guiAdd(parent, owner, elem);
}


GuiWindow* BeginWindow(const char* title) {
    return BeginElement(new GuiWindow(title));
}
void EndWindow() {
    EndElement();
}

GuiElement* Begin() {
    return BeginElement(new GuiElement);
}
void End() {
    EndElement();
}

GuiLabel* Label(const char* caption) {
    return InsertElement(new GuiLabel(caption));
}
GuiButton* Button(const char* caption) {
    return InsertElement(new GuiButton(caption));
}
GuiInputFilePath* InputFile(
    const char* caption,
    std::string* output,
    GUI_INPUT_FILE_TYPE type,
    const char* filter,
    const char* root_dir
) {
    return InsertElement(new GuiInputFilePath(caption, output, type, filter, root_dir));
}
GuiInputFilePath* InputFile(
    const char* caption,
    std::function<void(const std::string&)> set_cb,
    std::function<std::string(void)> get_cb,
    GUI_INPUT_FILE_TYPE type,
    const char* filter,
    const char* root_dir
) {
    return InsertElement(new GuiInputFilePath(caption, set_cb, get_cb, type, filter, root_dir));
}


GuiAnimationSyncList* AnimationSyncList() {
    return InsertElement(new GuiAnimationSyncList());
}


}
}