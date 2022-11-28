#pragma once

#include <assert.h>
#include <stack>
#include "platform/platform.hpp"
#include "math/gfxm.hpp"
#include "gpu/gpu_pipeline.hpp"
#include "gpu/gpu_text.hpp"

#include "gui/gui_hit.hpp"
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
    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) override {
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
    gfxm::vec2 icon_pos;
    const GuiIcon* icon = 0;
public:
    GuiButton(const char* caption = "Button")
    : caption(guiGetDefaultFont()) {
        this->size = gfxm::vec2(130.0f, 30.0f);
        this->caption.replaceAll(caption, strlen(caption));
    }

    void setCaption(const char* cap) {
        caption.replaceAll(cap, strlen(cap));
    }
    void setIcon(const GuiIcon* icon) {
        this->icon = icon;
    }

    GuiHitResult hitTest(int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }

        return GuiHitResult{ GUI_HIT::CLIENT, this };
    }

    void onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::DBL_CLICKED:
        case GUI_MSG::CLICKED: {
            notifyOwner(GUI_NOTIFICATION::BUTTON_CLICKED, this);
            } break;
        }

        GuiElement::onMessage(msg, params);
    }

    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) override {
        Font* font = guiGetCurrentFont()->font;

        float icon_offs = .0f;
        if (icon) {
            icon_offs = guiGetCurrentFont()->font->getLineHeight();
        }
        bounding_rect = gfxm::rect(
            rc.min,
            rc.min + gfxm::vec2(icon_offs + caption.getBoundingSize().x + GUI_PADDING * 4.f, font->getLineHeight() * 2.f)
        );
        client_area = bounding_rect;

        caption.prepareDraw(guiGetCurrentFont(), false);
        text_pos = guiCalcTextPosInRect(gfxm::rect(gfxm::vec2(0, 0), caption.getBoundingSize()), client_area, 0, gfxm::rect(0, 0, 0, 0), font);
        icon_pos = text_pos;
        text_pos.x += icon_offs;
    }

    void onDraw() override {
        Font* font = guiGetCurrentFont()->font;

        uint32_t col = GUI_COL_BUTTON;
        if (isPressed()) {
            col = GUI_COL_ACCENT;
        } else if (isHovered()) {
            col = GUI_COL_BUTTON_HOVER;
        }
        guiDrawRectRound(client_area, GUI_PADDING * 2.f, col);

        auto fnt = guiGetCurrentFont();
        //guiDrawRect(gfxm::rect(icon_pos, icon_pos + gfxm::vec2(fnt->font->getLineHeight(), fnt->font->getLineHeight())), GUI_COL_WHITE);
        if (icon) {
            icon->draw(gfxm::rect(icon_pos, icon_pos + gfxm::vec2(fnt->font->getLineHeight(), fnt->font->getLineHeight())), GUI_COL_WHITE);
        }
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
        text_content.selectAll();
        text_content.eraseSelected();
        text_content.putString(text, strlen(text));
    }

    GuiHitResult hitTest(int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }

        // TODO  

        return GuiHitResult{ GUI_HIT::CLIENT, this };
    }
    void onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::KEYDOWN:
            // TODO: Remove winapi usage
            switch (params.getA<uint16_t>()) {
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
                if (text_content.copySelected(copied, params.getA<uint16_t>() == 0x0058)) {
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
            switch (params.getA<GUI_CHAR>()) {
            case GUI_CHAR::BACKSPACE:
                text_content.backspace();
                break;
            case GUI_CHAR::ESCAPE:
                break;
            case GUI_CHAR::TAB:
                text_content.putString("\t", 1);
                break;
            default: {
                char ch = (char)params.getA<GUI_CHAR>();
                if (ch > 0x1F || ch == 0x0A) {
                    text_content.putString(&ch, 1);
                }                
            }
            }
            break;
        case GUI_MSG::MOUSE_MOVE:
            mouse_pos = gfxm::vec2(params.getA<int32_t>(), params.getB<int32_t>());
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
        case GUI_MSG::UNFOCUS:
            // TODO: Discard selection?
            break;
        }

        GuiElement::onMessage(msg, params);
    }
    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) override {
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
        //pos_content.x = client_area.min.x + (client_area.max.x - client_area.min.x) * .5f - text_content.getBoundingSize().x * .5f;
        pos_content.x = client_area.min.x + GUI_MARGIN;
    }
    void onDraw() override {
        guiDrawRectRound(client_area, GUI_PADDING * 2.f, GUI_COL_HEADER);
        if (guiGetFocusedWindow() == this) {
            guiDrawRectRoundBorder(client_area, GUI_PADDING * 2.f, 2.f, GUI_COL_ACCENT, GUI_COL_ACCENT);
        }
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
    void onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::UNFOCUS:
            if (editing) {
                updateValue();
                editing = false;
            }
            break;
        case GUI_MSG::KEYDOWN:
            // TODO: Remove winapi usage
            switch (params.getA<uint16_t>()) {
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
                if (text_content.copySelected(copied, params.getA<uint16_t>() == 0x0058)) {
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
            switch (params.getA<GUI_CHAR>()) {
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
                    char ch = (char)params.getA<GUI_CHAR>();
                    if (ch > 0x1F || ch == 0x0A) {
                        text_content.putString(&ch, 1);
                    }
                }
            }
            }
            break;
        case GUI_MSG::MOUSE_MOVE: {
            gfxm::vec2 new_mouse_pos = gfxm::vec2(params.getA<int32_t>(), params.getB<int32_t>());
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

        GuiElement::onMessage(msg, params);
    }
    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) override {
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
        guiDrawRectRound(client_area, GUI_PADDING * 2.f, col_box);
        if (editing) {
            guiDrawRectRoundBorder(client_area, GUI_PADDING * 2.f, 2.f, GUI_COL_ACCENT, GUI_COL_ACCENT);
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
    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) override {
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

inline void guiLayoutSplitRectX(const gfxm::rect& rc, gfxm::rect& a, gfxm::rect& b, float width_a) {
    const float w = rc.max.x - rc.min.x;
    const float w0 = gfxm::_min(w, width_a);
    const float w1 = w - w0;

    gfxm::rect rc_original = rc;

    a = gfxm::rect(
        gfxm::vec2(rc_original.min.x, rc_original.min.y),
        gfxm::vec2(rc_original.min.x + w0, rc_original.max.y)
    );
    b = gfxm::rect(
        gfxm::vec2(a.max.x, a.min.y),
        gfxm::vec2(rc_original.max.x, rc_original.max.y)
    );
}
inline void guiLayoutSplitRectY(const gfxm::rect& rc, gfxm::rect& a, gfxm::rect& b, float height_a) {
    const float h = rc.max.y - rc.min.y;
    const float h0 = gfxm::_min(h, height_a);
    const float h1 = h - h0;

    gfxm::rect rc_original = rc;

    a = gfxm::rect(
        gfxm::vec2(rc_original.min.x, rc_original.min.y),
        gfxm::vec2(rc_original.max.x, rc_original.min.y + h0)
    );
    b = a;
    b.min.y = a.max.y;
    b.max.y = rc_original.max.y;
}
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

template<int WIDTH, int HEIGHT>
class GuiGridHelper {
    gfxm::rect rects[WIDTH * HEIGHT];
public:
    GuiGridHelper(const gfxm::rect& rc) {
        int count = WIDTH * HEIGHT;
        float cell_w = (rc.max.x - rc.min.x) / WIDTH;
        float cell_h = (rc.max.y - rc.min.y) / HEIGHT;
        for (int i = 0; i < count; ++i) {
            gfxm::vec2 min;
            min.x = (i % WIDTH) * cell_w;
            min.y = (i / HEIGHT) * cell_h;
            gfxm::vec2 max;
            max.x = min.x + cell_w;
            max.y = min.y + cell_h;
            rects[i] = gfxm::rect(rc.min + min, rc.min + max);
        }
    }
    const gfxm::rect& getRect(int x, int y) {
        return rects[y * WIDTH + x];
    }
};

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
    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) override {
        gfxm::rect rc_label = rc;
        gfxm::rect rc_inp;
        guiLayoutSplitRect2H(rc_label, rc_inp, .25f);

        elem_label->layout(cursor, rc_label, flags);
        elem_input->layout(cursor, rc_inp, flags);

        bounding_rect = elem_label->getBoundingRect();
        gfxm::expand(bounding_rect, elem_input->getBoundingRect());
    }
    void onDraw() override {
        elem_label->draw();
        elem_input->draw();
    }
};

class GuiInputFloat2 : public GuiElement {
    GuiLabel*           elem_label;
    GuiInputFloatBox*   elem_inp_x;
    GuiInputFloatBox*   elem_inp_y;
public:
    GuiInputFloat2() {
        elem_label = (new GuiLabel());
        elem_inp_x = (new GuiInputFloatBox());
        elem_inp_y = (new GuiInputFloatBox());
        elem_label->setCaption("InputFloat2");

        addChild(elem_label);
        addChild(elem_inp_x);
        addChild(elem_inp_y);
    }

    GuiHitResult hitTest(int x, int y) override {
        GuiHitResult h;
        h.elem = 0;
        h.hit = GUI_HIT::NOWHERE;
        GuiElement* elems[] = {
            elem_inp_x, elem_inp_y
        };
        int i = 0;
        while (h.hit == GUI_HIT::NOWHERE && i < 2) {
            GuiElement* e = elems[i];
            h = e->hitTest(x, y);
            ++i;
        }
        return h;
    }
    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) override {
        gfxm::rect rc_label = rc;
        gfxm::rect rc_inp;
        guiLayoutSplitRect2H(rc_label, rc_inp, .25f);
        gfxm::rect rcs[2];
        guiLayoutSplitRectH(rc_inp, rcs, sizeof(rcs) / sizeof(rcs[0]), GUI_PADDING);

        elem_label->layout(cursor, rc_label, flags);
        elem_inp_x->layout(cursor, rcs[0], flags);
        elem_inp_y->layout(cursor, rcs[1], flags);

        bounding_rect = elem_label->getBoundingRect();
        gfxm::expand(bounding_rect, elem_inp_x->getBoundingRect());
        gfxm::expand(bounding_rect, elem_inp_y->getBoundingRect());
    }
    void onDraw() override {
        elem_label->draw();
        elem_inp_x->draw();
        elem_inp_y->draw();
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
    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) override {
        gfxm::rect rc_label = rc;
        gfxm::rect rc_inp;
        guiLayoutSplitRect2H(rc_label, rc_inp, .25f);
        gfxm::rect rcs[3];
        guiLayoutSplitRectH(rc_inp, rcs, sizeof(rcs) / sizeof(rcs[0]), GUI_PADDING);

        elem_label->layout(cursor, rc_label, flags);
        elem_inp_x->layout(cursor, rcs[0], flags);
        elem_inp_y->layout(cursor, rcs[1], flags);
        elem_inp_z->layout(cursor, rcs[2], flags);

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

class GuiInputFloat4 : public GuiElement {
    GuiLabel*           elem_label;
    GuiInputFloatBox*   elem_inp_x;
    GuiInputFloatBox*   elem_inp_y;
    GuiInputFloatBox*   elem_inp_z;
    GuiInputFloatBox*   elem_inp_w;
public:
    GuiInputFloat4() {
        elem_label = (new GuiLabel());
        elem_inp_x = (new GuiInputFloatBox());
        elem_inp_y = (new GuiInputFloatBox());
        elem_inp_z = (new GuiInputFloatBox());
        elem_inp_w = (new GuiInputFloatBox());
        elem_label->setCaption("InputFloat4");

        addChild(elem_label);
        addChild(elem_inp_x);
        addChild(elem_inp_y);
        addChild(elem_inp_z);
        addChild(elem_inp_w);
    }

    GuiHitResult hitTest(int x, int y) override {
        GuiHitResult h;
        h.elem = 0;
        h.hit = GUI_HIT::NOWHERE;
        GuiElement* elems[] = {
            elem_inp_x, elem_inp_y, elem_inp_z, elem_inp_w
        };
        int i = 0;
        while (h.hit == GUI_HIT::NOWHERE && i < 4) {
            GuiElement* e = elems[i];
            h = e->hitTest(x, y);
            ++i;
        }
        return h;
    }
    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) override {
        gfxm::rect rc_label = rc;
        gfxm::rect rc_inp;
        guiLayoutSplitRect2H(rc_label, rc_inp, .25f);
        gfxm::rect rcs[4];
        guiLayoutSplitRectH(rc_inp, rcs, sizeof(rcs) / sizeof(rcs[0]), GUI_PADDING);

        elem_label->layout(cursor, rc_label, flags);
        elem_inp_x->layout(cursor, rcs[0], flags);
        elem_inp_y->layout(cursor, rcs[1], flags);
        elem_inp_z->layout(cursor, rcs[2], flags);
        elem_inp_w->layout(cursor, rcs[3], flags);

        bounding_rect = elem_label->getBoundingRect();
        gfxm::expand(bounding_rect, elem_inp_x->getBoundingRect());
        gfxm::expand(bounding_rect, elem_inp_y->getBoundingRect());
        gfxm::expand(bounding_rect, elem_inp_z->getBoundingRect());
        gfxm::expand(bounding_rect, elem_inp_w->getBoundingRect());
    }
    void onDraw() override {
        elem_label->draw();
        elem_inp_x->draw();
        elem_inp_y->draw();
        elem_inp_z->draw();
        elem_inp_w->draw();
    }
};

class GuiCheckBox : public GuiElement {
    GuiTextBuffer caption;
public:
    GuiCheckBox(const char* cap = "CheckBox")
        : caption(guiGetDefaultFont()) {
        caption.replaceAll(cap, strlen(cap));
    }
    GuiHitResult hitTest(int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }
        return GuiHitResult{ GUI_HIT::CLIENT, this };
    }
    void onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::CLICKED: {
            // TODO
        } break;
        }
    }
    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) override {
        bounding_rect = rc;// gfxm::rect(cursor, cursor + gfxm::vec2(rc.max.x - rc.min.x, 50.0f));
        bounding_rect.max.y = bounding_rect.min.y + guiGetCurrentFont()->font->getLineHeight();
        client_area = bounding_rect;

        size = bounding_rect.size();

        caption.prepareDraw(guiGetCurrentFont(), false);
    }
    void onDraw() override {
        gfxm::rect rc_box = client_area;
        rc_box.max.x = rc_box.min.x + (rc_box.max.y - rc_box.min.y);
        
        guiDrawCheckBox(rc_box, true, false);

        float line_gap = guiGetCurrentFont()->font->getLineGap();
        caption.draw(gfxm::vec2(rc_box.max.x + GUI_MARGIN, client_area.min.y - line_gap), GUI_COL_TEXT, GUI_COL_ACCENT);
    }
};

class GuiRadioButton : public GuiElement {
    GuiTextBuffer caption;
public:
    GuiRadioButton(const char* cap = "RadioButton")
        : caption(guiGetDefaultFont()) {
        caption.replaceAll(cap, strlen(cap));
    }
    GuiHitResult hitTest(int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }
        return GuiHitResult{ GUI_HIT::CLIENT, this };
    }
    void onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::CLICKED: {
            // TODO
        } break;
        }
    }
    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) override {
        bounding_rect = rc;// gfxm::rect(cursor, cursor + gfxm::vec2(rc.max.x - rc.min.x, 50.0f));
        bounding_rect.max.y = bounding_rect.min.y + guiGetCurrentFont()->font->getLineHeight();
        client_area = bounding_rect;

        size = bounding_rect.size();

        caption.prepareDraw(guiGetCurrentFont(), false);
    }
    void onDraw() override {
        gfxm::rect rc_box = client_area;
        rc_box.max.x = rc_box.min.x + (rc_box.max.y - rc_box.min.y);
        guiDrawCircle(rc_box.center(), rc_box.size().x * .5f, true, GUI_COL_HEADER);
        guiDrawCircle(rc_box.center(), rc_box.size().x * .24f, true, GUI_COL_TEXT);

        float line_gap = guiGetCurrentFont()->font->getLineGap();
        caption.draw(gfxm::vec2(rc_box.max.x + GUI_MARGIN, client_area.min.y - line_gap), GUI_COL_TEXT, GUI_COL_ACCENT);
    }
};

class GuiListItem : public GuiElement {
    GuiTextBuffer caption;
public:
    GuiListItem(const char* cap = "ListItem")
    : caption(guiGetDefaultFont()) {
        caption.replaceAll(cap, strlen(cap));
    }

    GuiHitResult hitTest(int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }

        return GuiHitResult{ GUI_HIT::CLIENT, this };
    }
    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) override {
        const float h = caption.font->font->getLineHeight();
        bounding_rect = gfxm::rect(cursor, gfxm::vec2(rc.max.x, .0f));
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

class GuiMenuBox : public GuiElement {
public:
    GuiMenuBox() {
        setFlags(getFlags() | GUI_FLAG_TOPMOST);

        addChild(new GuiListItem("ListItem"));
        addChild(new GuiListItem("ListItem2"));
        addChild(new GuiListItem("ListItem3"));
    }
    GuiHitResult hitTest(int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }

        for (int i = 0; i < childCount(); ++i) {
            auto ch = getChild(i);
            GuiHitResult hit = ch->hitTest(x, y);
            if (hit.hasHit()) {
                return hit;
            }
        }

        return GuiHitResult{ GUI_HIT::CLIENT, this };
    }
    void onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        }

        GuiElement::onMessage(msg, params);
    }
    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) override {
        bounding_rect = gfxm::rect(
            pos, pos + size
        );
        client_area = bounding_rect;
        
        gfxm::vec2 cur = client_area.min;
        gfxm::rect rc_content = rc;
        cur.y += GUI_PADDING;
        for (int i = 0; i < childCount(); ++i) {
            auto ch = getChild(i);
            ch->layout(cur, rc_content, flags);
            cur.y += ch->getBoundingRect().max.y - ch->getBoundingRect().min.y;
        }
    }
    void onDraw() override {
        guiDrawRectShadow(client_area);
        guiDrawRect(client_area, GUI_COL_HEADER);
        guiDrawRectLine(client_area, GUI_COL_BUTTON);
        for (int i = 0; i < childCount(); ++i) {
            auto ch = getChild(i);
            ch->draw();
        }
    }
};
class GuiComboBox : public GuiElement {
    GuiTextBuffer text_content;

    gfxm::vec2 pos_content;

    gfxm::vec2 mouse_pos;
    bool pressing = false;

    bool is_open = false;
    std::unique_ptr<GuiMenuBox> menu_box;
public:
    GuiComboBox()
    : text_content(guiGetDefaultFont()) {
        text_content.putString("ComboBox", strlen("ComboBox"));
    }

    GuiHitResult hitTest(int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }

        // TODO  

        return GuiHitResult{ GUI_HIT::CLIENT, this };
    }
    void onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::CLICKED:
        case GUI_MSG::DBL_CLICKED:
            if (!is_open) {
                menu_box.reset(new GuiMenuBox());
                menu_box->setOwner(this);
                menu_box->pos = gfxm::vec2(bounding_rect.min.x, bounding_rect.max.y);
                menu_box->size = gfxm::vec2(
                    bounding_rect.max.x - bounding_rect.min.x,
                    100.0f
                );
                guiGetRoot()->addChild(menu_box.get());
                is_open = true;
            } else {
                guiGetRoot()->removeChild(menu_box.get());
                menu_box.reset(0);
                is_open = false;
            }
            break;
        }

        GuiElement::onMessage(msg, params);
    }
    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) override {
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
        //pos_content.x = client_area.min.x + (client_area.max.x - client_area.min.x) * .5f - text_content.getBoundingSize().x * .5f;
        pos_content.x = client_area.min.x + GUI_MARGIN;
    }
    void onDraw() override {
        uint32_t col_box = GUI_COL_HEADER;
        if (isHovered()) {
            col_box = GUI_COL_BUTTON;
        }
        if (is_open) {
            guiDrawRectRound(client_area, GUI_PADDING * 2.f, col_box, GUI_DRAW_CORNER_TOP);
        } else {
            guiDrawRectRound(client_area, GUI_PADDING * 2.f, col_box);
        }
        text_content.draw(pos_content, GUI_COL_TEXT, GUI_COL_ACCENT);
    }
};

class GuiCollapsingHeader : public GuiElement {
    GuiTextBuffer caption;
    gfxm::vec2 pos_caption;
    bool is_open = false;
public:
    GuiCollapsingHeader()
        : caption(guiGetDefaultFont()) {
        caption.putString("CollapsingHeader", strlen("CollapsingHeader"));
    }
    GuiHitResult hitTest(int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }

        // TODO  

        return GuiHitResult{ GUI_HIT::CLIENT, this };
    }
    void onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::CLICKED:
        case GUI_MSG::DBL_CLICKED:
            if (!is_open) {                
                is_open = true;
            } else {
                is_open = false;
            }
            break;
        }

        GuiElement::onMessage(msg, params);
    }
    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) override {
        Font* font = guiGetCurrentFont()->font;

        const float text_box_height = font->getLineHeight() + GUI_PADDING * 2.0f;
        const float client_area_height = text_box_height + GUI_MARGIN;
        bounding_rect = gfxm::rect(
            rc.min,
            gfxm::vec2(rc.max.x, rc.min.y + text_box_height)
        );
        client_area = bounding_rect;

        caption.prepareDraw(guiGetDefaultFont(), false);

        const float content_y_offset = text_box_height * .5f - (font->getAscender() + font->getDescender()) * .5f;
        pos_caption = client_area.min + gfxm::vec2(.0f, content_y_offset);
        //pos_content.x = client_area.min.x + (client_area.max.x - client_area.min.x) * .5f - text_content.getBoundingSize().x * .5f;
        pos_caption.x = client_area.min.x + GUI_MARGIN;
    }
    void onDraw() override {
        uint32_t col_box = GUI_COL_BUTTON;
        if (isHovered()) {
            col_box = GUI_COL_BUTTON_HOVER;
        }
        if (is_open) {
            guiDrawRectRound(client_area, GUI_PADDING * 2.f, col_box, GUI_DRAW_CORNER_TOP);
        } else {
            guiDrawRectRound(client_area, GUI_PADDING * 2.f, col_box);
        }
        caption.draw(pos_caption, GUI_COL_TEXT, GUI_COL_ACCENT);
    }
};

class GuiFileListItem : public GuiElement {
    GuiTextBuffer caption;
public:
    GuiFileListItem(const char* cap = "FileListItem")
    : caption(guiGetDefaultFont()) {
        size = gfxm::vec2(64, 96);
        caption.replaceAll(cap, strlen(cap));
    }

    GuiHitResult hitTest(int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }
        return GuiHitResult{ GUI_HIT::CLIENT, this };
    }
    void onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::MOUSE_SCROLL: {
            if (getParent()) {
                getParent()->sendMessage(msg, params);
            }
        } break;
        }
    }
    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) override {
        const float h = caption.font->font->getLineHeight();
        bounding_rect = gfxm::rect(cursor, cursor + size);
        client_area = bounding_rect;

        caption.prepareDraw(guiGetCurrentFont(), false);
    }
    void onDraw() override {
        if (isHovered()) {
            guiDrawRect(client_area, GUI_COL_BUTTON);
        }
        gfxm::rect rc_img = client_area;
        gfxm::expand(rc_img, -GUI_PADDING);
        rc_img.max.y = rc_img.min.y + (rc_img.max.x - rc_img.min.x);
        guiDrawRect(rc_img, GUI_COL_BUTTON_HOVER);

        caption.draw(gfxm::vec2(rc_img.min.x, rc_img.max.y + GUI_MARGIN), GUI_COL_TEXT, GUI_COL_ACCENT);
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
    void onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::LBUTTON_DOWN:
            // TODO: Select
            break;
        case GUI_MSG::DBL_CLICKED:
            collapsed = !collapsed;
            break;
        }
    }
    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) override {
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
                ch->layout(cursor, rc_child, flags);
                
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
    int visible_items_begin = 0;
    int visible_items_end = 0;
    gfxm::vec2 smooth_scroll = gfxm::vec2(.0f, .0f);

    std::unique_ptr<GuiScrollBarV> scroll_bar_v;
    gfxm::rect rc_scroll_v;
    gfxm::rect rc_scroll_h;
public:
    gfxm::vec2 scroll_offset = gfxm::vec2(.0f, .0f);

    GuiContainer() {
        size.y = 200.0f;

        scroll_bar_v.reset(new GuiScrollBarV());
        scroll_bar_v->setOwner(this);
    }

    GuiHitResult hitTest(int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }
        
        GuiHitResult hit = scroll_bar_v->hitTest(x, y);
        if (hit.hit != GUI_HIT::NOWHERE) {
            return hit;
        }

        for (int i = visible_items_begin; i < visible_items_end; ++i) {
            auto ch = getChild(i);
            GuiHitResult h = ch->hitTest(x, y);
            if (h.hit != GUI_HIT::NOWHERE) {
                return h;
            }
        }

        return GuiHitResult{ GUI_HIT::CLIENT, this };
    }
    void onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::MOUSE_SCROLL: {
            scroll_offset.y -= params.getA<int32_t>();
            scroll_bar_v->setOffset(scroll_offset.y);
        } break;
        case GUI_MSG::SB_THUMB_TRACK:
            scroll_offset.y = params.getA<float>();
        break;
        }
    }
    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) override {
        bounding_rect = rc;
        client_area = bounding_rect;

        gfxm::rect rc_content = client_area;
        gfxm::expand(rc_content, -GUI_MARGIN);
        gfxm::rect rc_child = rc_content;

        gfxm::vec2 elem_size(64.0f + GUI_MARGIN, 96.0f);
        int elems_per_row = (rc_content.max.x - rc_content.min.x) / elem_size.x;
        int visible_row_count = (rc_content.max.y - rc_content.min.y) / elem_size.y + 2;
        int total_row_count = childCount() / (elems_per_row + 1);

        float max_scroll = gfxm::_max(.0f, (total_row_count * elem_size.y) - (rc_content.max.y - rc_content.min.y));

        scroll_offset.y = gfxm::_min(max_scroll, gfxm::_max(.0f, scroll_offset.y));
        smooth_scroll = gfxm::vec2(
            gfxm::lerp(smooth_scroll.x, scroll_offset.x, .4f),
            gfxm::lerp(smooth_scroll.y, scroll_offset.y, .4f)
        );

        visible_items_begin = gfxm::_min((int)childCount(), (int)(smooth_scroll.y / elem_size.y) * elems_per_row);
        visible_items_end = gfxm::_min((int)childCount(), visible_items_begin + visible_row_count * elems_per_row);
        int small_offset = smooth_scroll.y - (int)(smooth_scroll.y / elem_size.y) * elem_size.y;

        gfxm::vec2 cur = rc_content.min - gfxm::vec2(.0f, small_offset);
        for (int i = visible_items_begin; i < visible_items_end; ++i) {
            auto ch = getChild(i);
            if ((rc_content.max.x - cur.x) < elem_size.x) {
                cur.x = rc_content.min.x;
                cur.y += elem_size.y;
            }
            ch->layout(cur, rc_content, flags);
            cur.x += elem_size.x;
        }
        /*
        for (int i = 0; i < childCount(); ++i) {
            auto ch = getChild(i);
            ch->layout(rc_child, flags);

            rc_child.min.y += ch->getBoundingRect().max.y - ch->getBoundingRect().min.y;
        }*/

        gfxm::vec2 content_size(rc_content.max.x - rc_content.min.x, max_scroll);
        if (content_size.y > rc_content.max.y - rc_content.min.y) {
            scroll_bar_v->setEnabled(true);
            //rc_content.max.x -= 10.0f;
            //content_size = updateContentLayout();
            scroll_bar_v->setScrollData((rc_content.max.y - rc_content.min.y), content_size.y);
        } else {
            scroll_bar_v->setEnabled(false);
        }
        
        scroll_bar_v->layout(cursor, client_area, 0);
    }
    void onDraw() override {
        guiDrawRect(client_area, GUI_COL_HEADER);
        guiDrawPushScissorRect(client_area);
        for (int i = visible_items_begin; i < visible_items_end; ++i) {
            auto ch = getChild(i);
            ch->draw();
        }
        scroll_bar_v->draw();
        guiDrawPopScissorRect();
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
    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) override {
        container->layout(cursor, rc, flags);
        bounding_rect = container->getBoundingRect();
        client_area = bounding_rect;
    }
    void onDraw() override {
        container->draw();
    }
};

class GuiListView : public GuiElement {
    GuiContainer* container = 0;
public:
    GuiListView() {
        container = new GuiContainer();
        addChild(container);

        container->addChild(new GuiListItem("List item 0"));
        container->addChild(new GuiListItem("List item 1"));
        container->addChild(new GuiListItem("List item 2"));
    }
    GuiHitResult hitTest(int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }

        return container->hitTest(x, y);
    }
    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) override {
        container->layout(cursor, rc, flags);
        bounding_rect = container->getBoundingRect();
        client_area = bounding_rect;
    }
};

class GuiDemoWindow : public GuiWindow {
public:
    GuiDemoWindow()
    : GuiWindow("DemoWindow") {
        pos = gfxm::vec2(850, 200);
        size = gfxm::vec2(400, 600);

        addChild(new GuiLabel("Hello, World!"));
        addChild(new GuiInputTextLine());
        addChild(new GuiInputFloatBox());
        addChild(new GuiInputFloat());
        addChild(new GuiInputFloat2());
        addChild(new GuiInputFloat3());
        addChild(new GuiInputFloat4());
        addChild(new GuiComboBox());
        addChild(new GuiCollapsingHeader());
        addChild(new GuiCheckBox());
        addChild(new GuiRadioButton());
        addChild(new GuiButton("Button A"));
        addChild(new GuiButton("Button B"));
        addChild(new GuiTreeView());        
    }
};

class GuiFileExplorerWindow : public GuiWindow {
public:
    GuiFileExplorerWindow()
        : GuiWindow("FileExplorer") {
        size = gfxm::vec2(800, 600);

        auto dir = new GuiInputTextLine();
        addChild(dir);
        dir->setText("./example/path/");

        auto container = new GuiContainer();
        addChild(container);
        for (int i = 0; i < 100; ++i) {
            container->addChild(new GuiFileListItem(MKSTR("fname" << i).c_str()));
        }
    }
};


constexpr float GUI_NODE_CIRCLE_RADIUS = 7.f;
class GuiNodeInput : public GuiElement {
    GuiTextBuffer caption;
    gfxm::vec2 circle_pos;
    bool is_filled = false;
public:
    GuiNodeInput()
        : caption(guiGetDefaultFont()) {
        caption.replaceAll("In", strlen("In"));
    }

    const gfxm::vec2& getCirclePosition() { return circle_pos; }
    void setFilled(bool b) {
        is_filled = b;
    }

    GuiHitResult hitTest(int x, int y) override {
        if (!guiHitTestCircle(circle_pos, GUI_NODE_CIRCLE_RADIUS * 1.5f, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }
        return GuiHitResult{ GUI_HIT::CLIENT, this };
    }
    void onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::CLICKED:
            assert(getOwner());
            if (getOwner()) {
                getOwner()->notify<GuiNodeInput*>(GUI_NOTIFICATION::NODE_INPUT_CLICKED, this);
            }
            break;
        case GUI_MSG::RBUTTON_DOWN:
            assert(getOwner());
            if (getOwner()) {
                getOwner()->notify<GuiNodeInput*>(GUI_NOTIFICATION::NODE_INPUT_BREAK, this);
            }
            break;
        }
    }
    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) override {
        bounding_rect = gfxm::rect(
            cursor,
            gfxm::vec2(rc.max.x, cursor.y + 20.0f)
        );
        client_area = bounding_rect;

        caption.prepareDraw(guiGetCurrentFont(), false);

        circle_pos = gfxm::vec2(
            client_area.min.x,
            client_area.center().y
        );
    }
    void onDraw() override {
        if (is_filled) {
            guiDrawCircle(circle_pos, GUI_NODE_CIRCLE_RADIUS, true);
        } else {
            guiDrawCircle(circle_pos, GUI_NODE_CIRCLE_RADIUS, true, GUI_COL_BLACK);
            guiDrawCircle(circle_pos, GUI_NODE_CIRCLE_RADIUS, false);
        }
        if (isHovered()) {
            guiDrawCircle(circle_pos, GUI_NODE_CIRCLE_RADIUS * 1.5f);
        }
        caption.draw(gfxm::vec2(client_area.min.x + GUI_NODE_CIRCLE_RADIUS + GUI_MARGIN, caption.findCenterOffsetY(client_area)), GUI_COL_TEXT, GUI_COL_TEXT);
    }
};
class GuiNodeOutput : public GuiElement {
    GuiTextBuffer caption;
    gfxm::vec2 circle_pos;
    bool is_filled = false;
public:
    GuiNodeOutput()
        : caption(guiGetDefaultFont()) {
        caption.replaceAll("Out", strlen("Out"));
    }

    const gfxm::vec2& getCirclePosition() { return circle_pos; }
    void setFilled(bool b) {
        is_filled = b;
    }

    GuiHitResult hitTest(int x, int y) override {
        if (!guiHitTestCircle(circle_pos, GUI_NODE_CIRCLE_RADIUS * 1.5f, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }
        return GuiHitResult{ GUI_HIT::CLIENT, this };
    }
    void onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::CLICKED:
            assert(getOwner());
            if (getOwner()) {
                getOwner()->notify<GuiNodeOutput*>(GUI_NOTIFICATION::NODE_OUTPUT_CLICKED, this);
            }
            break;
        case GUI_MSG::RBUTTON_DOWN:
            assert(getOwner());
            if (getOwner()) {
                getOwner()->notify<GuiNodeOutput*>(GUI_NOTIFICATION::NODE_OUTPUT_BREAK, this);
            }
            break;
        }
    }
    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) override {
        bounding_rect = gfxm::rect(
            cursor,
            gfxm::vec2(rc.max.x, cursor.y + 20.0f)
        );
        client_area = bounding_rect;

        caption.prepareDraw(guiGetCurrentFont(), false);

        circle_pos = gfxm::vec2(
            client_area.max.x,
            client_area.center().y
        );
    }
    void onDraw() override {
        if (is_filled) {
            guiDrawCircle(circle_pos, GUI_NODE_CIRCLE_RADIUS, true);
        } else {
            guiDrawCircle(circle_pos, GUI_NODE_CIRCLE_RADIUS, true, GUI_COL_BLACK);
            guiDrawCircle(circle_pos, GUI_NODE_CIRCLE_RADIUS, false);
        }
        if (isHovered()) {
            guiDrawCircle(circle_pos, GUI_NODE_CIRCLE_RADIUS * 1.5f);
        }
        caption.draw(
            gfxm::vec2(
                client_area.max.x - caption.getBoundingSize().x - 7.f - GUI_MARGIN, 
                caption.findCenterOffsetY(client_area)
            ), GUI_COL_TEXT, GUI_COL_TEXT
        );
    }
};
class GuiNode : public GuiElement {
    GuiTextBuffer caption;

    uint32_t color = GUI_COL_RED;
    gfxm::rect rc_title;
    gfxm::rect rc_body;
    std::vector<std::unique_ptr<GuiNodeInput>> inputs;
    std::vector<std::unique_ptr<GuiNodeOutput>> outputs;

    bool is_selected = false;

    gfxm::vec2 last_mouse_pos;

    void requestSelectedState() {
        assert(getOwner());
        if (getOwner()) {
            getOwner()->sendMessage(GUI_MSG::NOTIFY, (uint64_t)GUI_NOTIFICATION::NODE_CLICKED, (uint64_t)this);
        }
    }
public:
    GuiNode()
    : caption(guiGetDefaultFont()) {
        caption.replaceAll("NodeName", strlen("NodeName"));
    }

    void setColor(uint32_t color) {
        this->color = color;
    }
    void setPosition(float x, float y) {
        pos = gfxm::vec2(x, y);
    }
    void setSelected(bool b) {
        is_selected = b;
    }

    GuiNodeInput* createInput() {
        auto ptr = new GuiNodeInput();
        ptr->setOwner(this);
        inputs.push_back(std::unique_ptr<GuiNodeInput>(ptr));
        return ptr;
    }
    GuiNodeOutput* createOutput() {
        auto ptr = new GuiNodeOutput();
        ptr->setOwner(this);
        outputs.push_back(std::unique_ptr<GuiNodeOutput>(ptr));
        return ptr;
    }
    int getInputCount() const { return inputs.size(); }
    int getOutputCount() const { return outputs.size(); }
    GuiNodeInput* getInput(int i) { return inputs[i].get(); }
    GuiNodeOutput* getOutput(int i) { return outputs[i].get(); }

    GuiHitResult hitTest(int x, int y) override {
        gfxm::rect expanded_client_rc = client_area;
        gfxm::expand(expanded_client_rc, 10.0f);
        if (!guiHitTestRect(expanded_client_rc, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }

        GuiHitResult hit;
        for (int i = 0; i < inputs.size(); ++i) {
            hit = inputs[i]->hitTest(x, y);
            if (hit.hasHit()) {
                return hit;
            }
        }
        for (int i = 0; i < outputs.size(); ++i) {
            hit = outputs[i]->hitTest(x, y);
            if (hit.hasHit()) {
                return hit;
            }
        }

        if (!guiHitTestRect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }

        return GuiHitResult{ GUI_HIT::CLIENT, this };
    }
    void onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::MOUSE_SCROLL:
            assert(getOwner());
            if (getOwner()) {
                getOwner()->sendMessage(msg, params);
            }
            break;
        case GUI_MSG::MOUSE_MOVE:
            if (isPulled()) {
                if (!is_selected) {
                    requestSelectedState();
                }
                gfxm::vec2 mouse_pos(params.getA<int32_t>(), params.getB<int32_t>());
                gfxm::vec2 diff = (mouse_pos - last_mouse_pos) * getContentViewScale();
                pos += diff;
            }
            last_mouse_pos = gfxm::vec2(params.getA<int32_t>(), params.getB<int32_t>());
            break;
        case GUI_MSG::CLICKED:
            requestSelectedState();
            break;
        case GUI_MSG::NOTIFY:
            assert(getOwner());
            if (getOwner()) {
                getOwner()->sendMessage(msg, params);
            }
            break;
        }
    }
    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) override {
        bounding_rect = gfxm::rect(
            cursor,
            cursor + gfxm::vec2(200, 300)
        );
        client_area = bounding_rect;
        rc_title = gfxm::rect(
            bounding_rect.min,
            gfxm::vec2(bounding_rect.max.x, bounding_rect.min.y + 30.0f)
        );
        rc_body = gfxm::rect(
            gfxm::vec2(bounding_rect.min.x, bounding_rect.min.y + 30.0f),
            bounding_rect.max
        );

        caption.prepareDraw(guiGetCurrentFont(), false);

        float y_latest = rc_title.max.y + GUI_MARGIN;
        gfxm::vec2 cur = rc_body.min + gfxm::vec2(.0f, GUI_MARGIN);
        for (int i = 0; i < childCount(); ++i) {
            auto ch = getChild(i);
            ch->layout(cur, rc_body, flags);
            cur.y += (ch->getBoundingRect().max.y - ch->getBoundingRect().min.y) + GUI_MARGIN;
            y_latest = ch->getBoundingRect().max.y;
        }
        for (int i = 0; i < inputs.size(); ++i) {
            inputs[i]->onLayout(cur, rc_body, flags);
            cur.y 
                += (inputs[i]->getBoundingRect().max.y - inputs[i]->getBoundingRect().min.y)
                + GUI_MARGIN;
            y_latest = inputs[i]->getBoundingRect().max.y;
        }
        for (int i = 0; i < outputs.size(); ++i) {
            outputs[i]->onLayout(cur, rc_body, flags);
            cur.y
                += (outputs[i]->getBoundingRect().max.y - outputs[i]->getBoundingRect().min.y)
                + GUI_MARGIN;
            y_latest = outputs[i]->getBoundingRect().max.y;
        }
        rc_body.max.y = y_latest + GUI_MARGIN;
        bounding_rect.max.y = rc_body.max.y;
        client_area.max.y = rc_body.max.y;
    }
    void onDraw() override {
        gfxm::vec2 shadow_offset(.0f, 10.0f);
        guiDrawRectRoundBorder(gfxm::rect(
            rc_title.min + shadow_offset, rc_body.max + shadow_offset
        ), 20.0f, 10.0f, 0xAA000000, 0x00000000);
        guiDrawRectRound(gfxm::rect(
            rc_title.min + shadow_offset, rc_body.max + shadow_offset
        ), 20.0f, 0xAA000000);

        guiDrawRectRound(rc_title, 20.0f, color, GUI_DRAW_CORNER_TOP);
        caption.draw(gfxm::vec2(rc_title.min.x + 10.0f, caption.findCenterOffsetY(rc_title)), GUI_COL_TEXT, GUI_COL_ACCENT);

        guiDrawRectRound(rc_body, 20.0f, GUI_COL_BG, GUI_DRAW_CORNER_BOTTOM);
        if (is_selected) {
            guiDrawRectRoundBorder(gfxm::rect(
                rc_title.min, rc_body.max
            ), 20.0f, 10.0f, GUI_COL_WHITE, 0x00000000);
        }

        for (int i = 0; i < childCount(); ++i) {
            getChild(i)->draw();
        }
        for (int i = 0; i < inputs.size(); ++i) {
            inputs[i]->draw();
        }
        for (int i = 0; i < outputs.size(); ++i) {
            outputs[i]->draw();
        }
    }
};

struct GuiNodeLink {
    GuiNodeOutput* from;
    GuiNodeInput* to;
};
struct GuiNodeLinkPreview {
    GuiNodeOutput* out;
    GuiNodeInput* in;
    gfxm::vec2 from;
    gfxm::vec2 to;
    bool is_active;
    bool is_output;
};
class GuiNodeContainer : public GuiElement {
    std::vector<std::unique_ptr<GuiNode>> nodes;
    std::vector<GuiNodeLink> links;
    GuiNodeLinkPreview link_preview;
    gfxm::vec2 last_mouse_pos;

    GuiNode* selected_node = 0;

    void startLinkPreview(GuiNodeOutput* out, GuiNodeInput* in, const gfxm::vec2& from, bool is_output) {
        assert(out == 0 || in == 0);
        if (out) {
            link_preview.out = out;
            link_preview.in = 0;
        } else if(in) {
            link_preview.out = 0;
            link_preview.in = in;
        } else {
            assert(false);
            return;
        }
        link_preview.is_active = true;
        link_preview.from = from;
        link_preview.to = last_mouse_pos;
        link_preview.is_output = is_output;
    }
    void stopLinkPreview() {
        link_preview.is_active = false;
    }

    void setSelectedNode(GuiNode* node) {
        if (selected_node) {
            selected_node->setSelected(false);
        }
        selected_node = node;
        if (selected_node) {
            selected_node->setSelected(true);
        }
    }
public:
    GuiNodeContainer() {
        stopLinkPreview();
    }

    GuiNode* createNode() {
        auto ptr = new GuiNode();
        ptr->setOwner(this);
        nodes.push_back(std::unique_ptr<GuiNode>(ptr));
        return ptr;
    }
    void destroyNode(GuiNode* node) {
        for (int i = 0; i < nodes.size(); ++i) {
            if (nodes[i].get() == node) {
                nodes.erase(nodes.begin() + i);
                break;
            }
        }
    }

    void link(GuiNode* node_a, int nout, GuiNode* node_b, int nin) {
        auto out = node_a->getOutput(nout);
        auto in = node_b->getInput(nin);
        link(out, in);
    }
    void link(GuiNodeOutput* out, GuiNodeInput* in) {
        assert(in != nullptr && out != nullptr);
        linkBreak(in);
        out->setFilled(true);
        in->setFilled(true);
        links.push_back(GuiNodeLink{ out, in });
    }
    void linkBreak(GuiNodeOutput* out) {
        for (int i = 0; i < links.size(); ++i) {
            if (links[i].from == out) {
                links[i].from->setFilled(false);
                links[i].to->setFilled(false);
                links.erase(links.begin() + i);
                --i;
            }
        }
    }
    void linkBreak(GuiNodeInput* in) {
        for (int i = 0; i < links.size(); ++i) {
            if (links[i].to == in) {
                links[i].from->setFilled(false);
                links[i].to->setFilled(false);
                links.erase(links.begin() + i);
                break;
            }
        }
    }

    GuiHitResult hitTest(int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }
        
        //if (!link_preview.is_active) {
            guiPushViewTransform(getContentViewTransform());
            GuiHitResult hit;
            for (int i = nodes.size() - 1; i >= 0; --i) {
                auto ch = nodes[i].get();
                hit = ch->hitTest(x, y);
                if (hit.hit != GUI_HIT::NOWHERE) {
                    return hit;
                }
            }
            guiPopViewTransform();
        //}

        return GuiHitResult{ GUI_HIT::CLIENT, this };
    }
    void onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::CLICKED: {
            setSelectedNode(0);
            } break;
        case GUI_MSG::MOUSE_SCROLL: {
            gfxm::vec2 mouse2_lcl = guiGetMousePosLocal(client_area);
            gfxm::vec3 mouse_lcl(
                mouse2_lcl.x,
                mouse2_lcl.y,
                .0f
            );
            if (params.getA<int32_t>() < .0f) {
                float new_scale = getContentViewScale() * 1.1f;
                setContentViewTranslation(
                    getContentViewTranslation() - mouse_lcl * getContentViewScale() * .1f
                );
                setContentViewScale(new_scale);
            } else {
                float new_scale = getContentViewScale() * 0.9f;
                setContentViewTranslation(
                    getContentViewTranslation() + mouse_lcl * getContentViewScale() * .1f
                );
                setContentViewScale(new_scale);
                
            }
        } break;
        case GUI_MSG::SB_THUMB_TRACK:
            // TODO: scroll
            break;
        case GUI_MSG::MOUSE_MOVE:
            if (isPulled()) {
                gfxm::vec2 mouse_pos(params.getA<int32_t>(), params.getB<int32_t>());
                gfxm::vec2 diff = (mouse_pos - last_mouse_pos) * getContentViewScale();
                translateContentView(-diff.x, -diff.y);
            }
            last_mouse_pos = gfxm::vec2(params.getA<int32_t>(), params.getB<int32_t>());
            break;
        case GUI_MSG::UNFOCUS:
            stopLinkPreview();
            break;
        case GUI_MSG::RBUTTON_DOWN:
            stopLinkPreview();
            break;
        case GUI_MSG::NOTIFY: {
            switch (params.getA<GUI_NOTIFICATION>()) {
            case GUI_NOTIFICATION::NODE_CLICKED: {
                GuiNode* node = params.getB<GuiNode*>();
                setSelectedNode(node);
                }break;
            case GUI_NOTIFICATION::NODE_INPUT_CLICKED: {
                GuiNodeInput* inp = params.getB<GuiNodeInput*>();
                if (link_preview.is_active && link_preview.is_output) {
                    link(link_preview.out, inp);
                    stopLinkPreview();
                } else {
                    startLinkPreview(0, inp, inp->getCirclePosition(), false);
                }
                }break;
            case GUI_NOTIFICATION::NODE_OUTPUT_CLICKED: {
                GuiNodeOutput* out = params.getB<GuiNodeOutput*>();
                if (link_preview.is_active && !link_preview.is_output) {
                    link(out, link_preview.in);
                    stopLinkPreview();
                } else {
                    startLinkPreview(out, 0, out->getCirclePosition(), true);
                }
                }break;
            case GUI_NOTIFICATION::NODE_INPUT_BREAK: {
                GuiNodeInput* in = params.getB<GuiNodeInput*>();
                linkBreak(in);
                }break;
            case GUI_NOTIFICATION::NODE_OUTPUT_BREAK: {
                GuiNodeOutput* out = params.getB<GuiNodeOutput*>();
                linkBreak(out);
                }break;
            }            
            }break;
        }
    }
    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) override {
        bounding_rect = rc;
        client_area = bounding_rect;

        gfxm::rect rc_content = client_area;
        gfxm::expand(rc_content, -GUI_MARGIN);
        
        for (int i = 0; i < nodes.size(); ++i) {
            auto n = nodes[i].get();
            n->onLayout(client_area.min + n->pos, rc, flags);
        }

        if (link_preview.is_active) {
            gfxm::vec2 mpos = guiGetMousePosLocal(client_area);
            gfxm::vec3 mpos3 = gfxm::inverse(getContentViewTransform()) * gfxm::vec4(mpos.x, mpos.y, .0f, 1.f);
            mpos = gfxm::vec2(mpos3.x, mpos3.y);
            link_preview.to = mpos;
        }
    }
    void onDraw() override {
        guiDrawRect(client_area, GUI_COL_HEADER);
        guiDrawPushScissorRect(client_area);

        guiPushViewTransform(getContentViewTransform());

        for (int i = 0; i < links.size(); ++i) {
            guiDrawCurveSimple(
                links[i].from->getCirclePosition(),
                links[i].to->getCirclePosition(), 3.f
            );
        }
        for (int i = 0; i < nodes.size(); ++i) {
            auto n = nodes[i].get();
            n->draw();
        }

        if (link_preview.is_active) {
            guiDrawCurveSimple(
                link_preview.from,
                link_preview.to, 3.0f, 0xFF777777
            );
        }
        guiPopViewTransform();

        // TODO
        guiDrawPopScissorRect();
    }
};

class GuiNodeEditorWindow : public GuiWindow {
    GuiNodeContainer* node_container;
public:
    GuiNodeEditorWindow()
        : GuiWindow("NodeEditor") {
        size = gfxm::vec2(800, 600);

        node_container = (new GuiNodeContainer);
        addChild(node_container);
        auto node0 = node_container->createNode();
        node0->createInput();
        node0->createInput();
        node0->createInput();
        node0->createOutput();
        node0->setPosition(450, 200);
        node0->setColor(0xFF352A53);
        node0->addChild(new GuiLabel("Any gui element can be\ndisplayed inside a node"));
        auto node1 = node_container->createNode();
        node1->createOutput();
        node1->setPosition(100, 100);
        node1->setColor(0xFF4F5A63);
        auto node2 = node_container->createNode();
        node2->createInput();
        node2->setPosition(620, 400);
        node2->setColor(0xFF828CAA);
        auto node3 = node_container->createNode();
        node3->createOutput();
        node3->setPosition(100, 400);
        node3->setColor(0xFF4F5A63);
        //node_container->link(node1, 0, node0, 0);
        //node_container->link(node1, 0, node0, 1);
        //node_container->link(node1, 0, node0, 2);
    }
};

class GuiTimelineTrackListItem : public GuiElement {
    GuiTextBuffer caption;
public:
    GuiTimelineTrackListItem()
    : caption(guiGetDefaultFont()) {}
    void setCaption(const char* cap) {
        caption.replaceAll(cap, strlen(cap));
    }
    GuiHitResult hitTest(int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }
        return GuiHitResult{ GUI_HIT::CLIENT, this };
    }
    void onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {}
    }
    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) override {
        bounding_rect = rc;
        client_area = bounding_rect;
        caption.prepareDraw(guiGetCurrentFont(), false);
    }
    void onDraw() override {
        guiDrawRectRound(
            client_area, 15, GUI_COL_BUTTON,
            GUI_DRAW_CORNER_NW | GUI_DRAW_CORNER_SW
        );
        caption.draw(
            gfxm::vec2(client_area.min.x + GUI_MARGIN, caption.findCenterOffsetY(client_area)),
            GUI_COL_TEXT, GUI_COL_TEXT
        );
    }
};
class GuiTimelineTrackList : public GuiElement {
    std::vector<std::unique_ptr<GuiTimelineTrackListItem>> items;
public:
    GuiTimelineTrackList() {}
    GuiTimelineTrackListItem* addItem(const char* caption) {
        auto ptr = new GuiTimelineTrackListItem();
        ptr->setCaption(caption);
        items.push_back(std::unique_ptr<GuiTimelineTrackListItem>(ptr));
        addChild(ptr);
        return ptr;
    }
    GuiHitResult hitTest(int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }
        for (auto& i : items) {
            GuiHitResult hit = i->hitTest(x, y);
            if (hit.hasHit()) {
                return hit;
            }
        }
        return GuiHitResult{ GUI_HIT::CLIENT, this };
    }
    void onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {}
    }
    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) override {
        bounding_rect = rc;
        client_area = bounding_rect;
        float track_height = 30.0f;
        float track_margin = 1.0f;
        int track_count = 3;
        for (int i = 0; i < items.size(); ++i) {
            float y_offs = i * (track_height + track_margin);
            gfxm::rect rc(
                client_area.min + gfxm::vec2(.0f, y_offs),
                gfxm::vec2(client_area.max.x, client_area.min.y + y_offs + track_height)
            );
            items[i]->layout(rc.min, rc, flags);
        }
    }
    void onDraw() override {
        for (auto& i : items) {
            i->draw();
        }
    }
};

class GuiTimelineTrackViewTrackBase : public GuiElement {
    
public:
    float content_offset_x = .0f;
    float frame_screen_width = 20.0f;

    int getFrameAtScreenPos(float x, float y) {
        return std::max(0.f, ((x + content_offset_x - 10.0f + (frame_screen_width * .5f)) / frame_screen_width) + .1f);
    }
    float getScreenXAtFrame(int frame) {
        return frame * frame_screen_width + client_area.min.x + 10.0f - content_offset_x;
    }

    GuiTimelineTrackViewTrackBase() {}
    GuiHitResult hitTest(int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }
        return GuiHitResult{ GUI_HIT::CLIENT, this };
    }
    void onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::MOUSE_SCROLL: {
            if (getOwner()) {
                getOwner()->sendMessage(msg, params);
            }
        } break;
        case GUI_MSG::LBUTTON_DOWN:
            if (getOwner()) {
                getOwner()->sendMessage(msg, params);
            }
            break;
        case GUI_MSG::LBUTTON_UP:
            if (getOwner()) {
                getOwner()->sendMessage(msg, params);
            }
            break;
        case GUI_MSG::MBUTTON_DOWN:
            if (getOwner()) {
                getOwner()->sendMessage(msg, params);
            }
            break;
        case GUI_MSG::MBUTTON_UP:
            if (getOwner()) {
                getOwner()->sendMessage(msg, params);
            }
            break;
        case GUI_MSG::MOUSE_MOVE:
            if (getOwner()) {
                getOwner()->sendMessage(msg, params);
            }
            break;
        }
    }
    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) override {
        bounding_rect = rc;
        client_area = bounding_rect;
    }
    void onDraw() override {}
};

class GuiTimelineEventItem : public GuiElement {
    float radius = 7.f;
    bool is_dragging = false;
public:
    int frame = 0;

    GuiTimelineEventItem(int at)
        : frame(at) {}
    GuiHitResult hitTest(int x, int y) override {
        if (!guiHitTestCircle(pos, radius, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }
        return GuiHitResult{ GUI_HIT::CLIENT, this };
    }
    void onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::MOUSE_MOVE:
            if (is_dragging) {
                notifyOwner(GUI_NOTIFICATION::TIMELINE_DRAG_EVENT, this);
            }
            break;
        case GUI_MSG::LBUTTON_DOWN:
            guiCaptureMouse(this);
            is_dragging = true;
            break;
        case GUI_MSG::LBUTTON_UP:
            guiCaptureMouse(0);
            is_dragging = false;
            break;
        case GUI_MSG::RBUTTON_DOWN:
            notifyOwner(GUI_NOTIFICATION::TIMELINE_ERASE_EVENT, this);
            break;
        }

    }
    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) override {
        gfxm::rect rc_(
            cursor - gfxm::vec2(radius, radius),
            cursor + gfxm::vec2(radius, radius)
        );
        pos = cursor;
        bounding_rect = rc_;
        client_area = rc_;
    }
    void onDraw() override {
        uint32_t color = GUI_COL_TEXT;
        if (isHovered() || is_dragging) {
            color = GUI_COL_TIMELINE_CURSOR;
        }
        guiDrawDiamond(pos, radius, color, color, color);
    }
};
class GuiTimelineTrackViewEventTrack : public GuiTimelineTrackViewTrackBase {  
    std::vector<std::unique_ptr<GuiTimelineEventItem>> items;
    std::set<int> occupied_frames;
    gfxm::vec2 last_mouse_pos;
    void sort() {
        std::sort(items.begin(), items.end(), [](const std::unique_ptr<GuiTimelineEventItem>& a, const std::unique_ptr<GuiTimelineEventItem>& b)->bool {
            return a->frame < b->frame;
        });
    }
public:
    GuiTimelineEventItem* addItem(int at) {
        if (occupied_frames.find(at) != occupied_frames.end()) {
            return 0;
        }
        auto ptr = new GuiTimelineEventItem(at);
        ptr->setOwner(this);
        addChild(ptr);
        items.push_back(std::unique_ptr<GuiTimelineEventItem>(ptr));
        occupied_frames.insert(at);
        sort();
        return ptr;
    }
    void removeItem(GuiTimelineEventItem* item) {
        for (int i = 0; i < items.size(); ++i) {
            if (items[i].get() == item) {
                occupied_frames.erase(items[i]->frame);
                removeChild(item);
                items.erase(items.begin() + i);
                break;
            }
        }
    }
    GuiHitResult hitTest(int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }
        for (int i = items.size() - 1; i >= 0; --i) {
            auto& item = items[i];
            GuiHitResult hit = item->hitTest(x, y);
            if (hit.hasHit()) {
                return hit;
            }
        }
        return GuiHitResult{ GUI_HIT::CLIENT, this };
    }
    void onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::LBUTTON_DOWN:
            getOwner()->sendMessage(msg, params);
            break;
        case GUI_MSG::LBUTTON_UP:
            getOwner()->sendMessage(msg, params);
            break;
        case GUI_MSG::MOUSE_MOVE:
            last_mouse_pos = gfxm::vec2(params.getA<int32_t>(), params.getB<int32_t>()) - client_area.min;
            getOwner()->sendMessage(msg, params);
            break;
        case GUI_MSG::RBUTTON_DOWN: {
            int frame = getFrameAtScreenPos(last_mouse_pos.x, last_mouse_pos.y);
            addItem(frame);
            }break;
        case GUI_MSG::NOTIFY:
            switch (params.getA<GUI_NOTIFICATION>()) {
            case GUI_NOTIFICATION::TIMELINE_DRAG_EVENT: {
                auto evt = params.getB<GuiTimelineEventItem*>();
                gfxm::vec2 mouse = guiGetMousePosLocal(client_area);
                mouse = mouse - client_area.min;
                int frame = getFrameAtScreenPos(mouse.x, mouse.y);
                if (occupied_frames.find(frame) == occupied_frames.end() && evt->frame != frame) {
                    occupied_frames.erase(evt->frame);
                    evt->frame = frame;
                    occupied_frames.insert(frame);
                    sort();
                }
                notifyOwner(GUI_NOTIFICATION::TIMELINE_DRAG_EVENT, evt);
                return;
                }break;
            case GUI_NOTIFICATION::TIMELINE_DRAG_EVENT_CROSS_TRACK: {
                auto evt = params.getB<GuiTimelineEventItem*>();
                gfxm::vec2 mouse = guiGetMousePosLocal(client_area);
                mouse = mouse - client_area.min;
                if (evt->getOwner() != this) {
                    if (gfxm::point_in_rect(client_area, guiGetMousePosLocal(client_area))) {
                        GuiTimelineEventItem* new_evt = addItem(evt->frame);
                        if (new_evt) {
                            ((GuiTimelineTrackViewEventTrack*)evt->getOwner())
                                ->removeItem(evt);
                            new_evt->sendMessage(GUI_MSG::LBUTTON_DOWN, GUI_MSG_PARAMS());
                        }
                    }
                }
                }break;
            case GUI_NOTIFICATION::TIMELINE_ERASE_EVENT: {
                removeItem(params.getB<GuiTimelineEventItem*>());
            }
            }
            break;
        }
        GuiTimelineTrackViewTrackBase::onMessage(msg, params);
    }
    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) override {
        bounding_rect = rc;
        client_area = bounding_rect;
        for (auto& i : items) {
            gfxm::vec2 p(
                getScreenXAtFrame(i->frame),
                client_area.center().y
            );
            i->layout(p, rc, flags);
        }
    }
    void onDraw() override {
        for (auto& i : items) {
            i->draw();
        }
    }
};
template<bool IS_RIGHT>
class GuiTimelineBlockResizer : public GuiElement {
public:
    GuiHitResult hitTest(int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }
        if (IS_RIGHT) {
            return GuiHitResult{ GUI_HIT::RIGHT, this };
        } else {
            return GuiHitResult{ GUI_HIT::LEFT, this };            
        }
    }
    void onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::RESIZING: {
            if (getOwner()) {
                getOwner()->sendMessage(msg, params);
            }
            }break;
        }
    }
    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) override {
        bounding_rect = rc;
        client_area = rc;
    }
    void onDraw() override {
        if (IS_RIGHT) {
            guiDrawRectRound(client_area, 5.f, GUI_COL_TEXT, GUI_DRAW_CORNER_RIGHT);
        } else {
            guiDrawRectRound(client_area, 5.f, GUI_COL_TEXT, GUI_DRAW_CORNER_LEFT);
        }
    }
};
class GuiTimelineBlockItem : public GuiElement {
    bool is_dragging = false;
    gfxm::rect rc_resize_left;
    gfxm::rect rc_resize_right;
    std::unique_ptr<GuiTimelineBlockResizer<true>> resizer_right;
    std::unique_ptr<GuiTimelineBlockResizer<false>> resizer_left;
public:
    bool highlight_override = false;
    int frame;
    int length;
    gfxm::vec2 grab_point;
    GuiTimelineBlockItem(int frame, int len)
        : frame(frame), length(len) {
        resizer_right.reset(new GuiTimelineBlockResizer<true>());
        resizer_right->setOwner(this);
        addChild(resizer_right.get());
        resizer_left.reset(new GuiTimelineBlockResizer<false>());
        resizer_left->setOwner(this);
        addChild(resizer_left.get());
    }
    GuiHitResult hitTest(int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }
        GuiHitResult hit;
        hit = resizer_left->hitTest(x, y);
        if (hit.hasHit()) {
            return hit;
        }
        hit = resizer_right->hitTest(x, y);
        if (hit.hasHit()) {
            return hit;
        }
        return GuiHitResult{ GUI_HIT::CLIENT, this };
    }
    void onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::MOUSE_MOVE:
            if (is_dragging) {
                if (getOwner()) {
                    getOwner()->notify(GUI_NOTIFICATION::TIMELINE_DRAG_BLOCK, this);
                }
            } else {
                grab_point = gfxm::vec2(params.getA<int32_t>(), params.getB<int32_t>()) - client_area.min;
            }
            break;
        case GUI_MSG::LBUTTON_DOWN:
            guiCaptureMouse(this);
            is_dragging = true;
            break;
        case GUI_MSG::LBUTTON_UP:
            guiCaptureMouse(0);
            is_dragging = false;
            break;
        case GUI_MSG::RBUTTON_DOWN:
            notifyOwner(GUI_NOTIFICATION::TIMELINE_ERASE_BLOCK, this);
            break;
        case GUI_MSG::RESIZING: {
            GUI_HIT hit = params.getA<GUI_HIT>();
            gfxm::rect* prc = params.getB<gfxm::rect*>();
            if (hit == GUI_HIT::LEFT) {
                notifyOwner(GUI_NOTIFICATION::TIMELINE_RESIZE_BLOCK_LEFT, this, prc->min.x);
            } else if(hit == GUI_HIT::RIGHT) {
                notifyOwner(GUI_NOTIFICATION::TIMELINE_RESIZE_BLOCK_RIGHT, this, prc->max.x);
            }
            }break;
        }
    }
    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) override {
        bounding_rect = rc;
        client_area = rc;
        rc_resize_left = rc;
        rc_resize_left.max.x = rc_resize_left.min.x + 5.f;
        rc_resize_right = rc;
        rc_resize_right.min.x = rc_resize_right.max.x - 5.f;

        resizer_right->layout(rc_resize_right.min, rc_resize_right, flags);
        resizer_left->layout(rc_resize_left.min, rc_resize_left, flags);
    }
    void onDraw() override {
        uint32_t color = GUI_COL_GREEN;
        bool draw_resizers = false;
        if (highlight_override 
            || isHovered() 
            || resizer_right->isHovered() || resizer_left->isHovered() 
            || resizer_right->hasMouseCapture() || resizer_left->hasMouseCapture() 
            || is_dragging
        ) {
            color = GUI_COL_TIMELINE_CURSOR;
            draw_resizers = true;
        }
        guiDrawRectRound(client_area, 5.f, color);
        guiDrawRectRoundBorder(client_area, 5.f, 2.f, GUI_COL_BG, GUI_COL_BG);

        if (draw_resizers) {
            resizer_right->draw();
            resizer_left->draw();
        }
    }
};
class GuiTimelineTrackViewBlockTrack : public GuiTimelineTrackViewTrackBase {
    std::vector<std::unique_ptr<GuiTimelineBlockItem>> blocks;
    int block_paint_start_frame = 0;
    int mouse_cur_frame = 0;
    GuiTimelineBlockItem* block_painted_item = 0;
    void startBlockPaint(int at) {
        block_paint_start_frame = at;
        block_painted_item = addItem(at, 1);
        block_painted_item->highlight_override = true;
    }
    void stopBlockPaint() {
        if (block_painted_item) {
            block_painted_item->highlight_override = false;
            block_painted_item = 0;
        }
    }
    void updateBlockPaint() {
        if (!block_painted_item) {
            return;
        }
        if (mouse_cur_frame > block_paint_start_frame) {
            block_painted_item->frame = block_paint_start_frame;
            block_painted_item->length = mouse_cur_frame - block_paint_start_frame;
        } else if(mouse_cur_frame < block_paint_start_frame) {
            block_painted_item->frame = mouse_cur_frame;
            block_painted_item->length = block_paint_start_frame - mouse_cur_frame;
        }
    }
    void sort() {
        std::sort(blocks.begin(), blocks.end(), [](const std::unique_ptr<GuiTimelineBlockItem>& a, const std::unique_ptr<GuiTimelineBlockItem>& b)->bool {
            return a->frame < b->frame;
        });
    }
public:
    GuiTimelineBlockItem* addItem(int at, int len) {
        auto ptr = new GuiTimelineBlockItem(at, len);
        ptr->setOwner(this);
        addChild(ptr);
        blocks.push_back(std::unique_ptr<GuiTimelineBlockItem>(ptr));
        sort();
        return ptr;
    }
    void removeItem(GuiTimelineBlockItem* item) {
        stopBlockPaint();
        for (int i = 0; i < blocks.size(); ++i) {
            if (blocks[i].get() == item) {
                removeChild(item);
                blocks.erase(blocks.begin() + i);
                break;
            }
        }
    }
    GuiHitResult hitTest(int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }
        for (int i = blocks.size() - 1; i >= 0; --i) {
            auto& b = blocks[i];
            GuiHitResult hit;
            hit = b->hitTest(x, y);
            if (hit.hasHit()) {
                return hit;
            }
        }
        return GuiHitResult{ GUI_HIT::CLIENT, this };
    }
    void onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::LBUTTON_DOWN:
            getOwner()->sendMessage(msg, params);
            break;
        case GUI_MSG::LBUTTON_UP:
            getOwner()->sendMessage(msg, params);
            break;
        case GUI_MSG::MOUSE_MOVE: {
            gfxm::vec2 m(params.getA<int32_t>(), params.getB<int32_t>());
            gfxm::vec2 mlcl = m - client_area.min;
            mouse_cur_frame = getFrameAtScreenPos(mlcl.x, mlcl.y);
            getOwner()->sendMessage(msg, params);
            } break;
        case GUI_MSG::RBUTTON_DOWN: {
            guiCaptureMouse(this);
            auto m = guiGetMousePosLocal(client_area);
            gfxm::vec2 mlcl = m - client_area.min;
            startBlockPaint(getFrameAtScreenPos(mlcl.x, mlcl.y));
            } break;
        case GUI_MSG::RBUTTON_UP: {
            auto m = guiGetMousePosLocal(client_area);
            stopBlockPaint();
            guiCaptureMouse(0);
            } break;
        case GUI_MSG::NOTIFY:
            switch (params.getA<GUI_NOTIFICATION>()) {
            case GUI_NOTIFICATION::TIMELINE_DRAG_BLOCK: {
                auto block = params.getB<GuiTimelineBlockItem*>();
                gfxm::vec2 mouse = guiGetMousePosLocal(client_area);
                mouse = mouse - client_area.min;
                mouse -= block->grab_point;
                int frame = getFrameAtScreenPos(mouse.x, mouse.y);
                block->frame = frame;
                sort();
                notifyOwner(GUI_NOTIFICATION::TIMELINE_DRAG_BLOCK, block);
                }break;
            case GUI_NOTIFICATION::TIMELINE_DRAG_BLOCK_CROSS_TRACK: {
                auto block = params.getB<GuiTimelineBlockItem*>();
                gfxm::vec2 mouse = guiGetMousePosLocal(client_area);
                mouse = mouse - client_area.min;
                mouse -= block->grab_point;
                if (block->getOwner() != this) {
                    if (gfxm::point_in_rect(client_area, guiGetMousePosLocal(client_area))) {
                        GuiTimelineBlockItem* new_block = addItem(block->frame, block->length);
                        if (new_block) {
                            new_block->grab_point = block->grab_point;
                            ((GuiTimelineTrackViewBlockTrack*)block->getOwner())
                                ->removeItem(block);
                            new_block->sendMessage(GUI_MSG::LBUTTON_DOWN, GUI_MSG_PARAMS());
                        }
                    }
                }
                }break;
            case GUI_NOTIFICATION::TIMELINE_ERASE_BLOCK: {
                removeItem(params.getB<GuiTimelineBlockItem*>());
                }break;
            case GUI_NOTIFICATION::TIMELINE_RESIZE_BLOCK_LEFT: {
                auto block = params.getB<GuiTimelineBlockItem*>();
                gfxm::vec2 mouse = guiGetMousePosLocal(client_area);
                mouse = mouse - client_area.min;
                int frame = getFrameAtScreenPos(mouse.x, mouse.y);
                if (frame != block->frame && frame != block->frame + block->length) {
                    if (frame < block->frame + block->length) {
                        int diff = frame - block->frame;
                        block->frame = frame;
                        block->length -= diff;
                    }
                }
                }break;
            case GUI_NOTIFICATION::TIMELINE_RESIZE_BLOCK_RIGHT:
                auto block = params.getB<GuiTimelineBlockItem*>();
                gfxm::vec2 mouse = guiGetMousePosLocal(client_area);
                mouse = mouse - client_area.min;
                int frame = getFrameAtScreenPos(mouse.x, mouse.y);
                if (frame != block->frame && frame != block->frame + block->length) {
                    if (frame > block->frame) {
                        block->length = frame - block->frame;
                    }
                }
                break;
            }
            break;
        }
        GuiTimelineTrackViewTrackBase::onMessage(msg, params);
    }
    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) override {
        bounding_rect = rc;
        client_area = rc;
        
        updateBlockPaint();
        for (auto& i : blocks) {
            gfxm::vec2 p(
                getScreenXAtFrame(i->frame),
                client_area.min.y
            );
            gfxm::vec2 p2(
                getScreenXAtFrame(i->frame + i->length),
                client_area.max.y
            );
            i->layout(p, gfxm::rect(p, p2), flags);
        }
    }
    void onDraw() override {
        for (auto& i : blocks) {
            i->draw();
        }
    }
};

inline void guiTimelineCalcDividers(float frame_width, int& prime, int& mid, int& skip) {
    int id = (100.0f / frame_width) / 5;
    int div = std::max(5, id * 5);
    prime = div;
    if ((div % 2) == 0) {
        mid = std::max(1, div / 2);
    }
    else {
        mid = 0;
    }
    if (mid != 0) {
        skip = std::max(1, div / 5);// std::max(1.0f, 10.0f / frame_screen_width);
    } else {
        skip = std::max(1, div / 5);
    }
}

class GuiTimelineTrackView : public GuiElement {
    std::vector<std::unique_ptr<GuiTimelineTrackViewTrackBase>> tracks;
    int cursor_frame = 0;
    bool is_pressed = false;
    bool is_panning = false;
    gfxm::vec2 last_mouse_pos = gfxm::vec2(0, 0);
    float frame_screen_width = 20.0f;
    int primary_divider = 5;
    int skip_divider = 1;
    
    int getFrameAtScreenPos(float x, float y) {
        return ((x + content_offset.x - 10.0f + (frame_screen_width * .5f)) / frame_screen_width);
    }
public:
    gfxm::vec2 content_offset = gfxm::vec2(0, 0);
    void setCursor(int frame, bool send_notification = true) {
        cursor_frame = frame;
        cursor_frame = std::max(0, cursor_frame);
        assert(getOwner());
        if (getOwner() && send_notification) {
            getOwner()->sendMessage(GUI_MSG::NOTIFY, (uint64_t)GUI_NOTIFICATION::TIMELINE_JUMP, cursor_frame);
        }
    }
    void setContentOffset(float x, float y) {
        x = gfxm::_max(.0f, x);
        content_offset.x = x;
        content_offset.y = y;
        for (auto& t : tracks) {
            t->content_offset_x = x;
        }
        if (getOwner()) {
            getOwner()->notify(GUI_NOTIFICATION::TIMELINE_PAN_X, content_offset.x);
            getOwner()->notify(GUI_NOTIFICATION::TIMELINE_PAN_Y, content_offset.y);
        }
    }
    void setFrameScale(float x) {
        float fsw_old = frame_screen_width;
        frame_screen_width = x;
        frame_screen_width = gfxm::_max(1.0f, frame_screen_width);
        frame_screen_width = (int)gfxm::_min(20.0f, frame_screen_width);
        float diff = frame_screen_width - fsw_old;
        if (fsw_old != frame_screen_width) {
            if (diff > .0f) {
                //content_offset.x += (guiGetMousePosLocal(client_area).x - client_area.min.x - 10.f) * (1.f / frame_screen_width);
            } else if (diff < .0f) {
                //content_offset.x -= (guiGetMousePosLocal(client_area).x - client_area.min.x - 10.f) * (1.f / frame_screen_width);
            }
            content_offset.x *= (frame_screen_width / fsw_old);
            setContentOffset(content_offset.x, content_offset.y);
        }
        for (auto& t : tracks) {
            t->frame_screen_width = frame_screen_width;
        }
        int mid;
        guiTimelineCalcDividers(frame_screen_width, primary_divider, mid, skip_divider);
        assert(getOwner());
        if (getOwner()) {
            getOwner()->sendMessage(GUI_MSG::NOTIFY, (uint64_t)GUI_NOTIFICATION::TIMELINE_ZOOM, frame_screen_width);
        }
    }
    GuiTimelineTrackView() {}
    GuiTimelineTrackViewEventTrack* addEventTrack() {
        auto ptr = new GuiTimelineTrackViewEventTrack;
        addChild(ptr);
        ptr->setOwner(this);
        tracks.push_back(std::unique_ptr<GuiTimelineTrackViewTrackBase>(ptr));
        return ptr;
    }
    GuiTimelineTrackViewBlockTrack* addBlockTrack() {
        auto ptr = new GuiTimelineTrackViewBlockTrack;
        addChild(ptr);
        ptr->setOwner(this);
        tracks.push_back(std::unique_ptr<GuiTimelineTrackViewTrackBase>(ptr));
        return ptr;
    }
    GuiHitResult hitTest(int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }
        for (auto& i : tracks) {
            GuiHitResult hit = i->hitTest(x, y);
            if (hit.hasHit()) {
                return hit;
            }
        }
        return GuiHitResult{ GUI_HIT::CLIENT, this };
    }
    void onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::MOUSE_SCROLL: {
            float diff = params.getA<int32_t>() / 100;
            setFrameScale(frame_screen_width + diff);
            } break;
        case GUI_MSG::LBUTTON_DOWN:
            is_pressed = true;
            setCursor((guiGetMousePosLocal(client_area).x - client_area.min.x - 10.f + frame_screen_width * .5f + content_offset.x) / frame_screen_width);
            guiCaptureMouse(this);
            break;
        case GUI_MSG::LBUTTON_UP:
            is_pressed = false;
            guiCaptureMouse(0);
            break;
        case GUI_MSG::MBUTTON_DOWN:
            is_panning = true;
            guiCaptureMouse(this);
            break;
        case GUI_MSG::MBUTTON_UP:
            is_panning = false;
            guiCaptureMouse(0);
            break;
        case GUI_MSG::MOUSE_MOVE:
            if (is_pressed) {
                setCursor((guiGetMousePosLocal(client_area).x - client_area.min.x - 10.f + frame_screen_width * .5f + content_offset.x) / frame_screen_width);
            } else if(is_panning) {
                gfxm::vec2 offs = last_mouse_pos - gfxm::vec2(params.getA<int32_t>(), params.getB<int32_t>());
                //offs /= frame_screen_width;
                content_offset += offs;
                content_offset = gfxm::vec2(gfxm::_max(.0f, content_offset.x), gfxm::_max(.0f, content_offset.y));
                setContentOffset(content_offset.x, content_offset.y);
            }
            last_mouse_pos = gfxm::vec2(params.getA<int32_t>(), params.getB<int32_t>());
            break;
        case GUI_MSG::NOTIFY:
            switch (params.getA<GUI_NOTIFICATION>()) {
            case GUI_NOTIFICATION::TIMELINE_DRAG_EVENT:
                for (auto& t : tracks) {
                    t->notify(GUI_NOTIFICATION::TIMELINE_DRAG_EVENT_CROSS_TRACK, params.getB<GuiTimelineEventItem*>());
                }
                break;
            case GUI_NOTIFICATION::TIMELINE_DRAG_BLOCK:
                for (auto& t : tracks) {
                    t->notify(GUI_NOTIFICATION::TIMELINE_DRAG_BLOCK_CROSS_TRACK, params.getB<GuiTimelineBlockItem*>());
                }
                break;
            }
            break;
        }
    }
    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) override {
        bounding_rect = rc;
        client_area = bounding_rect;
        float track_height = 30.0f;
        float track_margin = 1.0f;
        int track_count = 3;
        for (int i = 0; i < tracks.size(); ++i) {
            float y_offs = i * (track_height + track_margin);
            gfxm::rect rc(
                client_area.min + gfxm::vec2(.0f, y_offs),
                gfxm::vec2(client_area.max.x, client_area.min.y + y_offs + track_height)
            );
            tracks[i]->layout(rc.min, rc, flags);
        }
    }
    void onDraw() override {
        guiDrawRect(client_area, GUI_COL_BG_INNER);
        guiDrawPushScissorRect(client_area);

        float bar_width = frame_screen_width * primary_divider * 2.f;
        float bar_width2 = bar_width * 2.f;
        float client_width = client_area.max.x - client_area.min.x;
        int bar_count = (ceilf(client_width / (float)bar_width) + 1) / 2 + 1;
        for (int i = 0; i < bar_count; ++i) {
            gfxm::vec2 offs_rem = gfxm::vec2(fmodf(content_offset.x, bar_width), fmodf(content_offset.y, bar_width));
            int a = content_offset.x / bar_width;
            float offs = bar_width;
            if ((a % 2) == 0) {
                offs = .0f;
            }
            guiDrawRect(
                gfxm::rect(
                    client_area.min + gfxm::vec2(i * bar_width2 + 10.0f - offs_rem.x + offs, .0f),
                    gfxm::vec2(client_area.min.x + bar_width + i * bar_width2 + 10.0f - offs_rem.x + offs, client_area.max.y)
                ), GUI_COL_BG_INNER_ALT
            );
        }
        int v_line_count = client_width / frame_screen_width + 1;
        for (int i = 0; i < v_line_count; ++i) {
            uint64_t color = GUI_COL_BG;
            gfxm::vec2 offs_rem = gfxm::vec2(fmodf(content_offset.x, frame_screen_width), fmodf(content_offset.y, frame_screen_width));
            
            int frame_id = i + (int)(content_offset.x / frame_screen_width);
            if ((frame_id % skip_divider)) {
                continue;
            }
            if ((frame_id % primary_divider) == 0) {
                color = GUI_COL_BUTTON;
            }
            guiDrawLine(
                gfxm::rect(
                    gfxm::vec2(client_area.min.x + i * frame_screen_width + 10.0f - offs_rem.x, client_area.min.y),
                    gfxm::vec2(client_area.min.x + i * frame_screen_width + 10.0f - offs_rem.x, client_area.max.y)
                ), color
            );
        }
        float track_height = 30.0f;
        float track_margin = 1.0f;
        for (int i = 0; i < tracks.size(); ++i) {
            guiDrawLine(gfxm::rect(
                gfxm::vec2(client_area.min.x + 10.0f, client_area.min.y + (i + 1) * (track_height + track_margin)),
                gfxm::vec2(client_area.max.x + 10.0f, client_area.min.y + (i + 1) * (track_height + track_margin))
            ), GUI_COL_BUTTON);
        }

        for (int i = 0; i < tracks.size(); ++i) {
            tracks[i]->draw();
        }

        // Draw timeline cursor
        guiDrawLine(gfxm::rect(
            gfxm::vec2(client_area.min.x + 10.0f + cursor_frame * frame_screen_width - content_offset.x, client_area.min.y),
            gfxm::vec2(client_area.min.x + 10.0f + cursor_frame * frame_screen_width - content_offset.x, client_area.max.y)
        ), GUI_COL_TIMELINE_CURSOR);

        guiDrawPopScissorRect();/*
        if (isHovered()) {
            gfxm::vec2 m = guiGetMousePosLocal(client_area);
            guiDrawText(
                m + gfxm::vec2(20.0f, 20.0f),
                MKSTR(getFrameAtScreenPos(m.x - getClientArea().min.x, .0f)).c_str(),
                guiGetCurrentFont(),
                100.0f,
                GUI_COL_TEXT
            );
        }*/
    }
};

class GuiTimelineBar : public GuiElement {
    int cursor_frame = 0;
    bool is_pressed = false;
    float frame_screen_width = 20.0f;
    int numbered_frame_divider = 5;
    int mid_frame_divider = 0;
    int skip_divider = 1;
public:
    gfxm::vec2 content_offset = gfxm::vec2(0, 0);
    void setFrameWidth(float w) {
        frame_screen_width = w;
        guiTimelineCalcDividers(w, numbered_frame_divider, mid_frame_divider, skip_divider);
    }
    void setCursor(int frame, bool send_notification = true) {
        cursor_frame = frame;
        cursor_frame = std::max(0, cursor_frame);
        assert(getOwner());
        if (getOwner() && send_notification) {
            getOwner()->sendMessage(GUI_MSG::NOTIFY, (uint64_t)GUI_NOTIFICATION::TIMELINE_JUMP, cursor_frame);
        }
    }
    GuiHitResult hitTest(int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }
        return GuiHitResult{ GUI_HIT::CLIENT, this };
    }
    void onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::LBUTTON_DOWN:
            is_pressed = true;
            setCursor((guiGetMousePosLocal(client_area).x - client_area.min.x - 10.f + frame_screen_width * .5f + content_offset.x) / frame_screen_width);
            guiCaptureMouse(this);
            break;
        case GUI_MSG::LBUTTON_UP:
            is_pressed = false;
            guiCaptureMouse(0);
            break;
        case GUI_MSG::MOUSE_MOVE:
            if (is_pressed) {
                setCursor((guiGetMousePosLocal(client_area).x - client_area.min.x - 10.f + frame_screen_width * .5f + content_offset.x) / frame_screen_width);
            }
            break;
        }
    }
    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) override {
        bounding_rect = rc;
        client_area = bounding_rect;
    }
    void onDraw() override {
        guiDrawRect(client_area, GUI_COL_BG_INNER);
        guiDrawPushScissorRect(client_area);

        float bar_width = frame_screen_width * numbered_frame_divider * 2.f;
        float bar_width2 = bar_width * 2.f;
        float client_width = client_area.max.x - client_area.min.x;
        int bar_count = (ceilf(client_width / (float)bar_width) + 1) / 2 + 1;
        for (int i = 0; i < bar_count; ++i) {
            gfxm::vec2 offs_rem = gfxm::vec2(fmodf(content_offset.x, bar_width), fmodf(content_offset.y, bar_width));
            int a = content_offset.x / bar_width;
            float offs = bar_width;
            if ((a % 2) == 0) {
                offs = .0f;
            }
            guiDrawRect(
                gfxm::rect(
                    client_area.min + gfxm::vec2(i * bar_width2 + 10.0f - offs_rem.x + offs, .0f),
                    gfxm::vec2(client_area.min.x + bar_width + i * bar_width2 + 10.0f - offs_rem.x + offs, client_area.max.y)
                ), GUI_COL_BG_INNER_ALT
            );
        }
        GuiTextBuffer text(guiGetCurrentFont());
        int v_line_count = client_width / frame_screen_width + 1;
        for (int i = 0; i < v_line_count; ++i) {
            int frame_id = i + (int)(content_offset.x / frame_screen_width);
            if ((frame_id % skip_divider)) {
                continue;
            }
            gfxm::vec2 offs_rem = gfxm::vec2(fmodf(content_offset.x, frame_screen_width), fmodf(content_offset.y, frame_screen_width));
            float offs_x = client_area.min.x + 10.0f + i * frame_screen_width - offs_rem.x;
            float bar_height = (client_area.max.y - client_area.min.y) * .5f;
            
            if (mid_frame_divider > 0 && (frame_id % mid_frame_divider) == 0) {
                bar_height = (client_area.max.y - client_area.min.y) * .75f;
            }

            uint64_t color = GUI_COL_BG;
            if ((frame_id % numbered_frame_divider) == 0) {
                bar_height = (client_area.max.y - client_area.min.y) * 1.f;

                color = GUI_COL_BUTTON;
                std::string snum = MKSTR(frame_id);
                text.replaceAll(snum.c_str(), snum.length());
                text.prepareDraw(guiGetCurrentFont(), false);
                
                float text_pos_x = offs_x;
                if (client_area.max.x < text_pos_x + text.getBoundingSize().x) {
                    text_pos_x -= text.getBoundingSize().x;
                }
                text.draw(gfxm::vec2(text_pos_x, client_area.min.y), GUI_COL_TEXT, GUI_COL_TEXT);
            }
            guiDrawLine(
                gfxm::rect(
                    gfxm::vec2(offs_x, client_area.max.y),
                    gfxm::vec2(offs_x, client_area.max.y - bar_height)
                ), color
            );
        }
        guiDrawLine(
            gfxm::rect(
                gfxm::vec2(client_area.min.x, client_area.max.y),
                gfxm::vec2(client_area.max.x, client_area.max.y)
            ), GUI_COL_BUTTON
        );

        // Draw timeline cursor
        guiDrawDiamond(
            gfxm::vec2(client_area.min.x + 10.0f + cursor_frame * frame_screen_width - content_offset.x, client_area.min.y),
            5.f, GUI_COL_TIMELINE_CURSOR, GUI_COL_TIMELINE_CURSOR, GUI_COL_TIMELINE_CURSOR
        );
        guiDrawLine(gfxm::rect(
            gfxm::vec2(client_area.min.x + 10.0f + cursor_frame * frame_screen_width - content_offset.x, client_area.min.y),
            gfxm::vec2(client_area.min.x + 10.0f + cursor_frame * frame_screen_width - content_offset.x, client_area.max.y)
        ), GUI_COL_TIMELINE_CURSOR);

        guiDrawPopScissorRect();
    }
};

class GuiSplitterGrid4 : public GuiElement {
    GuiElement* elem_top_left = 0;
    GuiElement* elem_top_right = 0;
    GuiElement* elem_bottom_left = 0;
    GuiElement* elem_bottom_right = 0;
    gfxm::rect rc_top_left;
    gfxm::rect rc_top_right;
    gfxm::rect rc_bottom_left;
    gfxm::rect rc_bottom_right;
public:
    void setElemTopLeft(GuiElement* elem) {
        if (elem_top_left) {
            removeChild(elem_top_left);
        }
        elem_top_left = elem;
        addChild(elem);
    }
    void setElemTopRight(GuiElement* elem) {
        if (elem_top_right) {
            removeChild(elem_top_right);
        }
        elem_top_right = elem;
        addChild(elem);
    }
    void setElemBottomLeft(GuiElement* elem) {
        if (elem_bottom_left) {
            removeChild(elem_bottom_left);
        }
        elem_bottom_left = elem;
        addChild(elem);
    }
    void setElemBottomRight(GuiElement* elem) {
        if (elem_bottom_right) {
            removeChild(elem_bottom_right);
        }
        elem_bottom_right = elem;
        addChild(elem);
    }
    GuiHitResult hitTest(int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }
        GuiHitResult hit;
        if (elem_top_left) {
            hit = elem_top_left->hitTest(x, y);
            if (hit.hasHit()) {
                return hit;
            }
        }
        if (elem_top_right) {
            hit = elem_top_right->hitTest(x, y);
            if (hit.hasHit()) {
                return hit;
            }
        }
        if (elem_bottom_left) {
            hit = elem_bottom_left->hitTest(x, y);
            if (hit.hasHit()) {
                return hit;
            }
        }
        if (elem_bottom_right) {
            hit = elem_bottom_right->hitTest(x, y);
            if (hit.hasHit()) {
                return hit;
            }
        }
        return GuiHitResult{ GUI_HIT::CLIENT, this };
    }
    void onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
    }
    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) override {
        bounding_rect = rc;
        client_area = bounding_rect;

        float offs_x = 200.0f;
        float offs_y = 30.0f;
        guiLayoutSplitRectX(rc, rc_top_left, rc_top_right, offs_x);
        guiLayoutSplitRectY(rc_top_left, rc_top_left, rc_bottom_left, offs_y);
        guiLayoutSplitRectY(rc_top_right, rc_top_right, rc_bottom_right, offs_y);


        if (elem_top_left) {
            elem_top_left->layout(rc_top_left.min, rc_top_left, flags);
        }
        if (elem_top_right) {
            elem_top_right->layout(rc_top_right.min, rc_top_right, flags);
        }
        if (elem_bottom_left) {
            elem_bottom_left->layout(rc_bottom_left.min, rc_bottom_left, flags);
        }
        if (elem_bottom_right) {
            elem_bottom_right->layout(rc_bottom_right.min, rc_bottom_right, flags);
        }
    }
    void onDraw() override {
        if (elem_top_left) {
            elem_top_left->draw();
        }
        if (elem_top_right) {
            elem_top_right->draw();
        }
        if (elem_bottom_left) {
            elem_bottom_left->draw();
        }
        if (elem_bottom_right) {
            elem_bottom_right->draw();
        }
    }
};
class GuiTimelineContainer : public GuiElement {
    bool is_playing = false;

    gfxm::rect rc_left;
    gfxm::rect rc_right;
    gfxm::rect rc_splitter;

    std::unique_ptr<GuiSplitterGrid4> splitter;
    std::unique_ptr<GuiTimelineBar> track_bar;
    std::unique_ptr<GuiTimelineTrackList> track_list;
    std::unique_ptr<GuiTimelineTrackView> track_view;
    // TODO:
    std::unique_ptr<GuiButton> button;

    std::unique_ptr<GuiScrollBarV> scroll_v;
    std::unique_ptr<GuiScrollBarH> scroll_h;
public:
    std::function<void(void)> on_play;
    std::function<void(void)> on_pause;
    std::function<void(int)> on_cursor;

    void togglePlay() {
        if (!is_playing) {
            is_playing = true;
            button->setIcon(guiLoadIcon("svg/entypo/controller-paus.svg"));
            if (on_play) { on_play(); }
        } else {
            is_playing = false;
            button->setIcon(guiLoadIcon("svg/entypo/controller-play.svg"));
            if (on_pause) { on_pause(); }
        }
    }

    void setCursor(int frame, bool send_notification = true) {
        frame = std::max(frame, 0);
        track_bar->setCursor(frame, send_notification);
        track_view->setCursor(frame, send_notification);
        if (on_cursor) { on_cursor(frame); }
    }
    void setCursorSilent(int frame) {
        frame = std::max(frame, 0);
        track_bar->setCursor(frame, false);
        track_view->setCursor(frame, false);
    }
    GuiTimelineContainer() {
        splitter.reset(new GuiSplitterGrid4);
        addChild(splitter.get());

        scroll_v.reset(new GuiScrollBarV);
        scroll_h.reset(new GuiScrollBarH);

        track_bar.reset(new GuiTimelineBar);

        track_list.reset(new GuiTimelineTrackList);
        track_list->addItem("HitboxTrack");
        track_list->addItem("AudioTrack");
        track_list->addItem("EventTrack");
        track_list->addItem("EventTrack2");

        track_view.reset(new GuiTimelineTrackView);
        track_view->addBlockTrack()->addItem(0, 10);
        track_view->addBlockTrack()->addItem(15, 5);
        track_view->addEventTrack()->addItem(7);
        track_view->addEventTrack();

        button.reset(new GuiButton(""));
        button->setIcon(guiLoadIcon("svg/entypo/controller-play.svg"));
        button->setOwner(this);

        scroll_v->setOwner(this);
        scroll_h->setOwner(this);
        track_bar->setOwner(this);
        track_list->setOwner(this);
        track_view->setOwner(this);

        splitter->setElemTopLeft(button.get());
        splitter->setElemTopRight(track_bar.get());
        splitter->setElemBottomLeft(track_list.get());
        splitter->setElemBottomRight(track_view.get());
        /*
        addChild(track_list.get());
        addChild(track_view.get());
        addChild(scroll_v.get());
        addChild(scroll_h.get());*/
    }
    GuiHitResult hitTest(int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }
        GuiHitResult hit = splitter->hitTest(x, y);
        if (hit.hasHit()) {
            return hit;
        }
        return GuiHitResult{ GUI_HIT::CLIENT, this };
    }
    void onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::NOTIFY:
            if (GUI_NOTIFICATION::TIMELINE_JUMP == params.getA<GUI_NOTIFICATION>()) {
                setCursor(params.getB<int>(), false);
            } else if (GUI_NOTIFICATION::TIMELINE_ZOOM == params.getA<GUI_NOTIFICATION>()) {
                track_bar->setFrameWidth(params.getB<float>());
            } else if(GUI_NOTIFICATION::TIMELINE_PAN_X == params.getA<GUI_NOTIFICATION>()) {
                track_bar->content_offset.x = params.getB<float>();
                track_view->content_offset.x = params.getB<float>();
            } else if(GUI_NOTIFICATION::TIMELINE_PAN_Y == params.getA<GUI_NOTIFICATION>()) {
                track_bar->content_offset.y = params.getB<float>();
                track_view->content_offset.y = params.getB<float>();
            } else if(GUI_NOTIFICATION::BUTTON_CLICKED == params.getA<GUI_NOTIFICATION>()) {
                togglePlay();
            }

            break;
        }
    }
    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) override {
        bounding_rect = rc;
        client_area = bounding_rect;

        splitter->layout(client_area.min, client_area, flags);
    }
    void onDraw() override {
        splitter->draw();
    }
};