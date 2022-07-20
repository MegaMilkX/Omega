#pragma once

#include <assert.h>
#include <stack>
#include "platform/platform.hpp"
#include "math/gfxm.hpp"
#include "gpu/gpu_pipeline.hpp"
#include "gpu/gpu_text.hpp"

#include "gui/gui_draw.hpp"

#include "gui/gui_values.hpp"
#include "gui/gui_color.hpp"

#include "gui/gui_system.hpp"

#include "gui/elements/gui_dock_space.hpp"
#include "gui/elements/gui_text.hpp"


class GuiImage : public GuiElement {
    gpuTexture2d* texture = 0;
public:
    GuiImage(gpuTexture2d* texture)
        : texture(texture) {

    }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        this->bounding_rect = gfxm::rect(
            rc.min,
            rc.min + gfxm::vec2(texture->getWidth(), texture->getHeight()) + gfxm::vec2(GUI_MARGIN, GUI_MARGIN) * 2.0f
        );
        this->client_area = bounding_rect;
    }

    void onDraw() override {
        guiDrawRectTextured(client_area, texture, GUI_COL_WHITE);
        //guiDrawColorWheel(client_area);
    }
};

class GuiButton : public GuiElement {
    GuiTextBuffer caption;

    gfxm::vec2 text_pos;
public:
    GuiButton(const char* caption = "Button")
    : caption(guiGetDefaultFont()) {
        this->size = gfxm::vec2(130.0f, 30.0f);
        this->caption.replaceAll(caption, strlen(caption));
    }

    GuiHitResult hitTest(int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }

        return GuiHitResult{ GUI_HIT::CLIENT, this };
    }

    void onMessage(GUI_MSG msg, uint64_t a_param, uint64_t b_param) override {
        switch (msg) {
        case GUI_MSG::CLICKED: {
            std::string str;
            caption.getWholeText(str);
            LOG_WARN(str << " clicked!");
            } break;
        }

        GuiElement::onMessage(msg, a_param, b_param);
    }

    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        Font* font = guiGetCurrentFont()->font;

        gfxm::vec2 text_sz = caption.getBoundingSize();
        bounding_rect = gfxm::rect(
            rc.min,
            rc.min + gfxm::vec2(text_sz.x + GUI_PADDING * 2.0f, font->getLineHeight() + GUI_PADDING * 2.0f)
        );
        client_area = bounding_rect;

        caption.prepareDraw(guiGetCurrentFont(), false);
        text_pos = guiCalcTextPosInRect(gfxm::rect(gfxm::vec2(0, 0), text_sz), client_area, 0, gfxm::rect(0, 0, 0, 0), font);
    }

    void onDraw() override {
        Font* font = guiGetCurrentFont()->font;

        uint32_t col = GUI_COL_BUTTON;
        if (isPressed()) {
            col = GUI_COL_ACCENT;
        } else if (isHovered()) {
            col = GUI_COL_BUTTON_HOVER;
        }
        guiDrawRect(client_area, col);

        caption.draw(text_pos, GUI_COL_TEXT, GUI_COL_ACCENT);
    }
};


class GuiInputTextLine : public GuiElement {
    GuiTextBuffer text_content;

    gfxm::vec2 pos_content;

    gfxm::vec2 mouse_pos;
    bool pressing = false;
public:
    GuiInputTextLine()
    : text_content(guiGetDefaultFont()) {
        text_content.putString("MyTextString", strlen("MyTextString"));
    }

    void setText(const char* text) {
        text_content.putString(text, strlen(text));
    }

    GuiHitResult hitTest(int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }

        // TODO  

        return GuiHitResult{ GUI_HIT::CLIENT, this };
    }
    void onMessage(GUI_MSG msg, uint64_t a_param, uint64_t b_param) override {
        switch (msg) {
        case GUI_MSG::KEYDOWN:
            // TODO: Remove winapi usage
            switch (a_param) {
            case VK_RIGHT: text_content.advanceCursor(1, !(guiGetModifierKeysState() & GUI_KEY_SHIFT)); break;
            case VK_LEFT: text_content.advanceCursor(-1, !(guiGetModifierKeysState() & GUI_KEY_SHIFT)); break;
            case VK_UP: text_content.advanceCursorLine(-1, !(guiGetModifierKeysState() & GUI_KEY_SHIFT)); break;
            case VK_DOWN: text_content.advanceCursorLine(1, !(guiGetModifierKeysState() & GUI_KEY_SHIFT)); break;
            case VK_DELETE:
                text_content.del();
                break;
            // CTRL+V key
            case 0x0056: if (guiGetModifierKeysState() & GUI_KEY_CONTROL) {
                std::string str;
                if (guiClipboardGetString(str)) {
                    text_content.putString(str.c_str(), str.size());
                }
            } break;
            // CTRL+C, CTRL+X
            case 0x0043: // C
            case 0x0058: /* X */ if (guiGetModifierKeysState() & GUI_KEY_CONTROL) {
                std::string copied;
                if (text_content.copySelected(copied, a_param == 0x0058)) {
                    guiClipboardSetString(copied);
                }
            } break;
            // CTRL+A
            case 0x41: if (guiGetModifierKeysState() & GUI_KEY_CONTROL) { // A
                text_content.selectAll();
            } break;
            }
            break;
        case GUI_MSG::UNICHAR:
            switch ((GUI_CHAR)a_param) {
            case GUI_CHAR::BACKSPACE:
                text_content.backspace();
                break;
            case GUI_CHAR::ESCAPE:
                break;
            case GUI_CHAR::TAB:
                text_content.putString("\t", 1);
                break;
            default: {
                char ch = (char)a_param;
                if (ch > 0x1F || ch == 0x0A) {
                    text_content.putString(&ch, 1);
                }                
            }
            }
            break;
        case GUI_MSG::MOUSE_MOVE:
            mouse_pos = gfxm::vec2(a_param, b_param);
            if (pressing) {
                text_content.pickCursor(mouse_pos - pos_content, false);
            }
            break;
        case GUI_MSG::LBUTTON_DOWN:
            guiCaptureMouse(this);
            pressing = true;

            text_content.pickCursor(mouse_pos - pos_content, true);
            break;
        case GUI_MSG::LBUTTON_UP:
            pressing = false;
            break;
        }

        GuiElement::onMessage(msg, a_param, b_param);
    }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        Font* font = guiGetCurrentFont()->font;

        const float text_box_height = font->getLineHeight() + GUI_PADDING * 2.0f;
        const float client_area_height = text_box_height + GUI_MARGIN;
        bounding_rect = gfxm::rect(
            rc.min,
            gfxm::vec2(rc.max.x, rc.min.y + text_box_height)
        );
        client_area = bounding_rect;

        text_content.prepareDraw(guiGetDefaultFont(), true);

        const float content_y_offset = text_box_height * .5f - (font->getAscender() + font->getDescender()) * .5f;
        pos_content = client_area.min + gfxm::vec2(.0f, content_y_offset);
        pos_content.x = client_area.min.x + (client_area.max.x - client_area.min.x) * .5f - text_content.getBoundingSize().x * .5f;
    }
    void onDraw() override {
        guiDrawRect(client_area, GUI_COL_HEADER);
        text_content.draw(pos_content, GUI_COL_TEXT, GUI_COL_ACCENT);
    }
};

class GuiInputFloatBox : public GuiElement {
    GuiTextBuffer text_content;

    gfxm::vec2 pos_content;

    gfxm::vec2 mouse_pos;
    bool pressing = false;
    bool dragging = false;
    bool editing = false;

    float value = .0f;

    void updateValue() {
        std::string str;
        text_content.getWholeText(str);
        char* e = 0;
        errno = 0;
        float val = strtof(str.c_str(), &e);
        if (errno != 0) {
            setValue(value);
        } else {
            setValue(val);
        }
    }
public:
    GuiInputFloatBox()
    : text_content(guiGetDefaultFont()) {
        setValue(.0f);
    }

    void setValue(float val) {
        value = val;
        char buf[16];
        int str_len = snprintf(buf, 16, "%.2f", value);
        text_content.replaceAll(buf, str_len);
    }
    float getValue() const {
        return value;
    }

    GuiHitResult hitTest(int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }
        return GuiHitResult{ GUI_HIT::CLIENT, this };
    }
    void onMessage(GUI_MSG msg, uint64_t a_param, uint64_t b_param) override {
        switch (msg) {
        case GUI_MSG::UNFOCUS:
            if (editing) {
                updateValue();
                editing = false;
            }
            break;
        case GUI_MSG::KEYDOWN:
            // TODO: Remove winapi usage
            switch (a_param) {
            case VK_RIGHT: text_content.advanceCursor(1, !(guiGetModifierKeysState() & GUI_KEY_SHIFT)); break;
            case VK_LEFT: text_content.advanceCursor(-1, !(guiGetModifierKeysState() & GUI_KEY_SHIFT)); break;
            case VK_UP: text_content.advanceCursorLine(-1, !(guiGetModifierKeysState() & GUI_KEY_SHIFT)); break;
            case VK_DOWN: text_content.advanceCursorLine(1, !(guiGetModifierKeysState() & GUI_KEY_SHIFT)); break;
            case VK_DELETE:
                text_content.del();
                break;
            // CTRL+V key
            case 0x0056: if (guiGetModifierKeysState() & GUI_KEY_CONTROL) {
                std::string str;
                if (guiClipboardGetString(str)) {
                    text_content.putString(str.c_str(), str.size());
                }
            } break;
            // CTRL+C, CTRL+X
            case 0x0043: // C
            case 0x0058: /* X */ if (guiGetModifierKeysState() & GUI_KEY_CONTROL) {
                std::string copied;
                if (text_content.copySelected(copied, a_param == 0x0058)) {
                    guiClipboardSetString(copied);
                }
            } break;
            // CTRL+A
            case 0x41: if (guiGetModifierKeysState() & GUI_KEY_CONTROL) { // A
                text_content.selectAll();
            } break;
            }
            break;
        case GUI_MSG::UNICHAR:
            switch ((GUI_CHAR)a_param) {
            case GUI_CHAR::BACKSPACE:
                text_content.backspace();
                break;
            case GUI_CHAR::ESCAPE:
                break;
            case GUI_CHAR::RETURN:
                if (editing) {
                    updateValue();
                    editing = false;
                }
                break;
            default: {
                if (editing) {
                    char ch = (char)a_param;
                    if (ch > 0x1F || ch == 0x0A) {
                        text_content.putString(&ch, 1);
                    }
                }
            }
            }
            break;
        case GUI_MSG::MOUSE_MOVE: {
            gfxm::vec2 new_mouse_pos = gfxm::vec2(a_param, b_param);
            if (pressing && guiGetFocusedWindow() == this) {
                if (!editing) {
                    dragging = true;
                    setValue(getValue() + (new_mouse_pos.x - mouse_pos.x) * 0.01f);
                } else {
                    text_content.pickCursor(new_mouse_pos - pos_content, false);
                }
            }
            mouse_pos = new_mouse_pos;
            } break;
        case GUI_MSG::LBUTTON_DOWN:
            guiCaptureMouse(this);
            pressing = true;
            if (editing) {
                text_content.pickCursor(mouse_pos - pos_content, true);
            }
            break;
        case GUI_MSG::LBUTTON_UP:
            pressing = false;
            dragging = false;
            break;
        case GUI_MSG::DBL_CLICKED:
            editing = true;
            break;
        }

        GuiElement::onMessage(msg, a_param, b_param);
    }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        Font* font = guiGetCurrentFont()->font;

        const float text_box_height = font->getLineHeight() + GUI_PADDING * 2.0f;
        const float client_area_height = text_box_height + GUI_MARGIN;
        bounding_rect = gfxm::rect(
            rc.min,
            gfxm::vec2(rc.max.x, rc.min.y + text_box_height)
        );
        client_area = bounding_rect;

        text_content.prepareDraw(guiGetDefaultFont(), true);

        const float content_y_offset = text_box_height * .5f - (font->getAscender() + font->getDescender()) * .5f;
        pos_content = client_area.min + gfxm::vec2(.0f, content_y_offset);
        pos_content.x = client_area.min.x + (client_area.max.x - client_area.min.x) * .5f - text_content.getBoundingSize().x * .5f;
    }
    void onDraw() override {
        uint32_t col_box = GUI_COL_HEADER;
        if (isHovered() && !editing) {
            col_box = GUI_COL_BUTTON;
        }
        guiDrawRect(client_area, col_box);
        if (editing) {
            guiDrawRectLine(client_area, GUI_COL_ACCENT);
        }
        text_content.draw(pos_content, GUI_COL_TEXT, GUI_COL_ACCENT);

        //guiDrawRectLine(bounding_rect, 0xFF00FF00);
        //guiDrawRectLine(client_area, 0xFFFFFF00);
    }
};

class GuiLabel : public GuiElement {
    GuiTextBuffer text_caption;
    gfxm::vec2 pos_caption;
public:
    GuiLabel(const char* caption = "Label")
    : text_caption(guiGetDefaultFont()) {
        text_caption.replaceAll(caption, strlen(caption));
        text_caption.prepareDraw(guiGetDefaultFont(), false);
        size.x = text_caption.getBoundingSize().x + GUI_PADDING * 2.f;
        size.y = guiGetDefaultFont()->font->getLineHeight() + GUI_PADDING * 2.f;
    }

    void setCaption(const char* cap) {
        text_caption.replaceAll(cap, strlen(cap));
    }

    GuiHitResult hitTest(int x, int y) override {
        return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
    }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        bounding_rect = gfxm::rect(
            rc.min,
            rc.min + size
        );
        pos_caption = bounding_rect.min;
        pos_caption.x += GUI_PADDING;
        const float content_y_offset =  size.y * .5f - (text_caption.font->font->getAscender() + text_caption.font->font->getDescender()) * .5f;
        pos_caption.y += content_y_offset;

        text_caption.prepareDraw(guiGetCurrentFont(), false);
    }
    void onDraw() override {
        text_caption.draw(pos_caption, GUI_COL_TEXT, GUI_COL_ACCENT);
    }
};

inline void guiLayoutSplitRect2H(gfxm::rect& rc, gfxm::rect& reminder, float ratio) {
    const float w = rc.max.x - rc.min.x;
    const float w0 = w * ratio;
    const float w1 = w - w0;

    reminder = rc;
    rc.max.x = rc.min.x + w0;
    reminder.min.x = rc.max.x;
}
inline void guiLayoutSplitRectH(const gfxm::rect& total, gfxm::rect* rects, int rect_count, float margin) {
    const float w_total = total.max.x - total.min.x;
    const float w_single = w_total / rect_count;

    for (int i = 0; i < rect_count; ++i) {
        rects[i].min.x = total.min.x + w_single * i;
        rects[i].min.y = total.min.y;
        rects[i].max.x = rects[i].min.x + w_single - margin;
        rects[i].max.y = total.max.y;
    }
}
class GuiInputFloat : public GuiElement {
    GuiLabel*           elem_label;
    GuiInputFloatBox*   elem_input;
public:
    GuiInputFloat() {
        elem_label = (new GuiLabel());
        elem_input = (new GuiInputFloatBox());
        elem_label->setCaption("InputFloat");

        addChild(elem_label);
        addChild(elem_input);
    }

    GuiHitResult hitTest(int x, int y) override {
        return elem_input->hitTest(x, y);
    }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        gfxm::rect rc_label = rc;
        gfxm::rect rc_inp;
        guiLayoutSplitRect2H(rc_label, rc_inp, .25f);

        elem_label->layout(rc_label, flags);
        elem_input->layout(rc_inp, flags);

        bounding_rect = elem_label->getBoundingRect();
        gfxm::expand(bounding_rect, elem_input->getBoundingRect());
    }
    void onDraw() override {
        elem_label->draw();
        elem_input->draw();
    }
};

class GuiInputFloat3 : public GuiElement {
    GuiLabel*           elem_label;
    GuiInputFloatBox*   elem_inp_x;
    GuiInputFloatBox*   elem_inp_y;
    GuiInputFloatBox*   elem_inp_z;
public:
    GuiInputFloat3() {
        elem_label = (new GuiLabel());
        elem_inp_x = (new GuiInputFloatBox());
        elem_inp_y = (new GuiInputFloatBox());
        elem_inp_z = (new GuiInputFloatBox());
        elem_label->setCaption("InputFloat3");

        addChild(elem_label);
        addChild(elem_inp_x);
        addChild(elem_inp_y);
        addChild(elem_inp_z);
    }

    GuiHitResult hitTest(int x, int y) override {
        GuiHitResult h;
        h.elem = 0;
        h.hit = GUI_HIT::NOWHERE;
        GuiElement* elems[] = {
            elem_inp_x, elem_inp_y, elem_inp_z
        };
        int i = 0;
        while (h.hit == GUI_HIT::NOWHERE && i < 3) {
            GuiElement* e = elems[i];
            h = e->hitTest(x, y);
            ++i;
        }
        return h;
    }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        gfxm::rect rcs[4];
        guiLayoutSplitRectH(rc, rcs, sizeof(rcs) / sizeof(rcs[0]), GUI_PADDING);

        elem_label->layout(rcs[0], flags);
        elem_inp_x->layout(rcs[1], flags);
        elem_inp_y->layout(rcs[2], flags);
        elem_inp_z->layout(rcs[3], flags);

        bounding_rect = elem_label->getBoundingRect();
        gfxm::expand(bounding_rect, elem_inp_x->getBoundingRect());
        gfxm::expand(bounding_rect, elem_inp_y->getBoundingRect());
        gfxm::expand(bounding_rect, elem_inp_z->getBoundingRect());
    }
    void onDraw() override {
        elem_label->draw();
        elem_inp_x->draw();
        elem_inp_y->draw();
        elem_inp_z->draw();
    }
};

class GuiListItem : public GuiElement {
    GuiTextBuffer caption;
public:
    GuiListItem()
    : caption(guiGetDefaultFont()) {
        caption.replaceAll("ListItem", strlen("ListItem"));
    }

    GuiHitResult hitTest(int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }
        return GuiHitResult{ GUI_HIT::CLIENT, this };
    }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        const float h = caption.font->font->getLineHeight();
        bounding_rect = rc;
        bounding_rect.max.y = bounding_rect.min.y + h;
        client_area = bounding_rect;

        caption.prepareDraw(guiGetCurrentFont(), false);
    }
    void onDraw() override {
        if (isHovered()) {
            guiDrawRect(client_area, GUI_COL_BUTTON);
        }
        caption.draw(client_area.min, GUI_COL_TEXT, GUI_COL_ACCENT);
    }
};

class GuiTreeItem : public GuiElement {
    GuiTextBuffer caption;
    GuiTextBuffer tree_indicator;

    gfxm::rect rc_header;
    gfxm::rect rc_children;

    bool collapsed = true;
public:
    GuiTreeItem(const char* cap = "TreeItem")
    : caption(guiGetDefaultFont()), tree_indicator(guiGetDefaultFont()) {
        caption.replaceAll(cap, strlen(cap));
        tree_indicator.replaceAll("-", strlen("-"));
    }

    void setCaption(const char* caption) {
        this->caption.replaceAll(caption, strlen(caption));
    }

    GuiHitResult hitTest(int x, int y) override {
        if (!gfxm::point_in_rect(bounding_rect, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }

        if (gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::CLIENT, this };
        }

        for (int i = 0; i < childCount(); ++i) {
            auto ch = getChild(i);
            GuiHitResult h = ch->hitTest(x, y);
            if (h.hit != GUI_HIT::NOWHERE) {
                return h;
            }
        }

        return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
    }
    void onMessage(GUI_MSG msg, uint64_t a_param, uint64_t b_param) override {
        switch (msg) {
        case GUI_MSG::LBUTTON_DOWN:
            // TODO: Select
            break;
        case GUI_MSG::DBL_CLICKED:
            collapsed = !collapsed;
            break;
        }
    }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        const float h = caption.font->font->getLineHeight();
        bounding_rect = rc;
        bounding_rect.max.y = bounding_rect.min.y + h;
        client_area = bounding_rect;

        if (collapsed) {
            tree_indicator.replaceAll("+", strlen("+"));
        } else {
            tree_indicator.replaceAll("-", strlen("-"));
        }
        tree_indicator.prepareDraw(guiGetCurrentFont(), false);
        caption.prepareDraw(guiGetCurrentFont(), false);

        rc_header = client_area;
        rc_header.min.x = rc_header.min.x + tree_indicator.getBoundingSize().x + GUI_PADDING;

        if (!collapsed && childCount() > 0) {
            gfxm::rect rc_child = rc_header;
            rc_child.min.y = rc_child.min.y + caption.getBoundingSize().y;
            for (int i = 0; i < childCount(); ++i) {
                auto ch = getChild(i);
                ch->layout(rc_child, flags);
                
                gfxm::expand(bounding_rect, ch->getBoundingRect());                

                rc_child.min.y += ch->getBoundingRect().max.y - ch->getBoundingRect().min.y;
            }

            rc_children = getChild(0)->getBoundingRect();
            for (int i = 0; i < childCount(); ++i) {
                auto ch = getChild(i);
                gfxm::expand(rc_children, ch->getBoundingRect());
            }
        }
    }
    void onDraw() override {
        if (isHovered()) {
            guiDrawRect(client_area, GUI_COL_BUTTON);
        }

        gfxm::vec2 pt = client_area.min;
        if (childCount() > 0) {            
            tree_indicator.draw(pt, GUI_COL_TEXT, GUI_COL_ACCENT);
            caption.draw(rc_header.min, GUI_COL_TEXT, GUI_COL_ACCENT);
        } else {
            caption.draw(pt, GUI_COL_TEXT, GUI_COL_ACCENT);
        }

        if (!collapsed && childCount() > 0) {
            for (int i = 0; i < childCount(); ++i) {
                auto ch = getChild(i);
                ch->draw();
            }
        }
    }
};

class GuiContainer : public GuiElement {
public:
    GuiContainer() {
        size.y = 200.0f;
    }

    GuiHitResult hitTest(int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }
        
        for (int i = 0; i < childCount(); ++i) {
            auto ch = getChild(i);
            GuiHitResult h = ch->hitTest(x, y);
            if (h.hit != GUI_HIT::NOWHERE) {
                return h;
            }
        }

        return GuiHitResult{ GUI_HIT::CLIENT, this };
    }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        bounding_rect = rc;
        bounding_rect.max.y = bounding_rect.min.y + size.y;
        client_area = bounding_rect;

        gfxm::rect rc_content = client_area;
        gfxm::expand(rc_content, -GUI_MARGIN);
        gfxm::rect rc_child = rc_content;
        for (int i = 0; i < childCount(); ++i) {
            auto ch = getChild(i);
            ch->layout(rc_child, flags);

            rc_child.min.y += ch->getBoundingRect().max.y - ch->getBoundingRect().min.y;
        }
    }
    void onDraw() override {
        guiDrawRect(client_area, GUI_COL_HEADER);
        for (int i = 0; i < childCount(); ++i) {
            auto ch = getChild(i);
            ch->draw();
        }
    }
};

class GuiTreeView : public GuiElement {
    GuiContainer* container = 0;
public:
    GuiTreeView() {
        container = (new GuiContainer());
        addChild(container);

        // dbg
        auto a = new GuiTreeItem("Item A");
        auto b = new GuiTreeItem("Item B");
        auto c = new GuiTreeItem("Item C");
        auto aa = new GuiTreeItem("SubItem A");
        aa->addChild(new GuiTreeItem("Foobar"));
        b->addChild(aa);
        b->addChild(new GuiTreeItem("Hello"));
        c->addChild(new GuiTreeItem("World!"));
        container->addChild(a);
        container->addChild(b);
        container->addChild(c);
    }

    GuiHitResult hitTest(int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }

        return container->hitTest(x, y);
    }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        container->layout(rc, flags);
        bounding_rect = container->getBoundingRect();
        client_area = bounding_rect;
    }
    void onDraw() override {
        container->draw();
    }
};

class GuiDemoWindow : public GuiWindow {
public:
    GuiDemoWindow()
    : GuiWindow("DemoWindow") {
        pos = gfxm::vec2(850, 200);
        size = gfxm::vec2(800, 600);

        addChild(new GuiLabel("Hello, World!"));
        addChild(new GuiInputTextLine());
        addChild(new GuiInputFloatBox());
        addChild(new GuiInputFloat());
        addChild(new GuiInputFloat3());
        addChild(new GuiTreeView());
        addChild(new GuiButton("Button A"));
        addChild(new GuiButton("Button B"));
    }
};
