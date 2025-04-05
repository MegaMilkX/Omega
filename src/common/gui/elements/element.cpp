#include "gui/elements/element.hpp"

#include "gui/gui_system.hpp"

#include "gui/elements/text_element.hpp"


bool GuiElement::isHovered() const {
    if (guiGetMouseCaptor() != 0 && guiGetMouseCaptor() != this) {
        return false;
    }

    auto e = guiGetHoveredElement();
    if (e == (GuiElement*)0xdddddddddddddddd) {
        DebugBreak();
    }
    if (e && e->parent == (GuiElement*)0xdddddddddddddddd) {
        DebugBreak();
    }
    auto ee = e;
    while (ee) {
        if (ee == this) {
            return true;
        }
        ee = ee->parent;
    }

    return false;
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



void GuiElement::setParent(GuiElement* elem) {
    setStyleDirty();
    if (parent) {
        parent->_removeChild(this);
    }
    if (!elem) {
        parent = 0;
        return;
    }
    parent = elem->content;
    assert(parent);
    if (parent) {
        parent->children.push_back(this);
        //parent->box.addChild(&this->box);
        parent->sortChildren();

        GUI_MSG_PARAMS params;
        params.setA(this);
        elem->sendMessage(GUI_MSG::CHILD_ADDED, params);
    }
}


void GuiElement::pushBack(const std::string& text) {
    addChild(new GuiTextElement(text));
}
void GuiElement::pushBack(const std::string& text, const std::initializer_list<std::string>& style_classes) {
    auto ptr = new GuiTextElement(text);
    addChild(ptr);
    ptr->setStyleClasses(style_classes);
}


void GuiElement::_addChild(GuiElement* elem) {
    assert(elem != this);
    if (elem->getParent()) {
        elem->getParent()->_removeChild(elem);
    }
    children.push_back(elem);
    //box.addChild(&elem->box);
    elem->parent = this;
    if (elem->owner == 0) {
        elem->owner = this;
    }
    sortChildren();

    GUI_MSG_PARAMS params;
    params.setA(elem);
    this->sendMessage(GUI_MSG::CHILD_ADDED, params);
    // NOTE: Posting this to the queue overflows it when a lot of children change parents
    // which happens quite often
    //guiPostMessage(this, GUI_MSG::CHILD_ADDED, params);

    setStyleDirty();
}
void GuiElement::_removeChild(GuiElement* elem) {
    int id = -1;
    for (int i = 0; i < children.size(); ++i) {
        if (children[i] == elem) {
            id = i;
            break;
        }
    }
    if (id >= 0) {
        GUI_MSG_PARAMS params;
        params.setA(children[id]);
        this->sendMessage(GUI_MSG::CHILD_REMOVED, params);

        children[id]->parent = 0;
        children.erase(children.begin() + id);
    }

    setStyleDirty();
}


int GuiElement::update_selection_range(int begin) {
    int last_end = begin;
    for (int i = 0; i < children.size(); ++i) {
        auto ch = children[i];
        last_end = ch->update_selection_range(last_end);
    }
    if (begin == last_end) {
        last_end = begin + self_linear_size;
    }
    linear_begin = begin;
    linear_end = last_end;
    return linear_end;
}
void GuiElement::apply_style() {
    if (needs_style_update) {
        auto& sheet = guiGetStyleSheet();
        GUI_STYLE_FLAGS flags = 0;
        flags |= isHovered() ? GUI_STYLE_FLAG_HOVERED : 0;
        flags |= isPressed() ? GUI_STYLE_FLAG_PRESSED : 0;
        flags |= isSelected() ? GUI_STYLE_FLAG_SELECTED : 0;
        flags |= isFocused() ? GUI_STYLE_FLAG_FOCUSED : 0;
        // TODO: Add more flags
        // Active, Disabled, ReadOnly
        getStyle()->clear();
        sheet.select_styles(getStyle(), style_classes, flags);
        if (getParent()) {
            getStyle()->inherit(*getParent()->getStyle());
        }
        getStyle()->finalize();
    }
    
    for (int i = 0; i < children.size(); ++i) {
        GuiElement* ch = children[i];
        if (ch->is_hidden) {
            continue;
        }
        ch->apply_style();
    }

    needs_style_update = false;
}
void GuiElement::hitTest(GuiHitResult& hit, int x, int y) {
    if (is_hidden) {
        return;
    }

    onHitTest(hit, x - layout_position.x, y - layout_position.y);
}
void GuiElement::layout(const gfxm::vec2& extents, uint64_t flags) {
    if (is_hidden) {
        return;
    }

    onLayout(extents, flags);
}
void GuiElement::draw(int x, int y) {
    if (is_hidden) {
        return;
    }
    //Font* font = getFont();
    //if (font) { guiPushFont(font); }
    guiPushOffset(gfxm::vec2(x, y));
    onDraw();
    guiPopOffset();
    //if (font) { guiPopFont(); }
}
void GuiElement::draw() {
    draw(layout_position.x, layout_position.y);
}
