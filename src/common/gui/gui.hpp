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

#include "gui/gui_layout_helpers.hpp"


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
            notifyOwner(GUI_NOTIFY::BUTTON_CLICKED, this);
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

class GuiMenuList;
class GuiMenuListItem : public GuiElement {
    GuiTextBuffer caption;
    bool is_open = false;
    std::unique_ptr<GuiMenuList> menu_list;
    GuiIcon* icon_arrow = 0;
public:
    int id = 0;
    int command_identifier = 0;

    void open();
    void close();

    bool hasList() { return menu_list.get() != nullptr; }

    GuiMenuListItem(const char* cap = "MenuListItem", int cmd = 0)
        : caption(guiGetDefaultFont()), command_identifier(cmd) {
        caption.replaceAll(cap, strlen(cap));
        icon_arrow = guiLoadIcon("svg/entypo/triangle-right.svg");
    }
    GuiMenuListItem(const char* cap, const std::initializer_list<GuiMenuListItem*>& child_items);
    void onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) {
        switch (msg) {
        case GUI_MSG::MOUSE_ENTER:
            notifyOwner(GUI_NOTIFY::MENU_ITEM_HOVER, id);
            break;
        case GUI_MSG::CLICKED:
        case GUI_MSG::DBL_CLICKED:
            if (hasList()) {
                notifyOwner(GUI_NOTIFY::MENU_ITEM_CLICKED, id);
            } else {
                notifyOwner(GUI_NOTIFY::MENU_COMMAND, command_identifier);
            }
            break;
        case GUI_MSG::NOTIFY:
            switch (params.getA<GUI_NOTIFY>()) {
            case GUI_NOTIFY::MENU_COMMAND:
                close();
                forwardMessageToOwner(msg, params);
                break;
            }
            break;
        }

        GuiElement::onMessage(msg, params);
    }
    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) override {
        caption.prepareDraw(guiGetCurrentFont(), false);
        const float h = guiGetCurrentFont()->font->getLineHeight() * 1.5f;
        const float w = gfxm::_max(rc.max.x - rc.min.x, caption.getBoundingSize().x + GUI_MARGIN * 2.f);
        bounding_rect = gfxm::rect(cursor, gfxm::vec2(cursor.x + w, cursor.y + h));
        client_area = bounding_rect;
    }
    void onDraw() override {
        if (isHovered()) {
            guiDrawRect(client_area, GUI_COL_BUTTON);
        }
        caption.draw(client_area.min + gfxm::vec2(GUI_MARGIN, guiGetCurrentFont()->font->getLineHeight() * .25f), GUI_COL_TEXT, GUI_COL_ACCENT);
        float fontH = guiGetCurrentFont()->font->getLineHeight();
        if (hasList() && icon_arrow) {
            icon_arrow->draw(guiLayoutPlaceRectInsideRect(client_area, gfxm::vec2(fontH, fontH), GUI_RIGHT | GUI_VCENTER, gfxm::rect(GUI_MARGIN, GUI_MARGIN, GUI_MARGIN, GUI_MARGIN)), GUI_COL_TEXT);
        }
    }
};
class GuiMenuList : public GuiElement {
    std::vector<std::unique_ptr<GuiMenuListItem>> items;
    GuiMenuListItem* open_elem = 0;
public:
    void open() {
        is_hidden = false;
    }
    void close() {
        is_hidden = true;
        if (open_elem) {
            open_elem->close();
        }
    }

    GuiMenuList() {
        setFlags(getFlags() | GUI_FLAG_TOPMOST);
    }
    GuiMenuList* addItem(GuiMenuListItem* item) {
        item->id = items.size();
        items.push_back(std::unique_ptr<GuiMenuListItem>(item));
        addChild(item);
        item->setOwner(this);
        return this;
    }

    GuiHitResult hitTest(int x, int y) override {
        if (is_hidden) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }
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
        case GUI_MSG::NOTIFY:
            switch (params.getA<GUI_NOTIFY>()) {
            case GUI_NOTIFY::MENU_ITEM_HOVER: {
                int id = params.getB<int>();
                if (open_elem && open_elem->id != params.getA<int>()) {
                    open_elem->close();
                    open_elem = nullptr;
                }
                if (!open_elem && items[id]->hasList()) {
                    open_elem = items[id].get();
                    open_elem->open();
                }
                }break;
            case GUI_NOTIFY::MENU_COMMAND:
                forwardMessageToOwner(msg, params);
                break;
            }
            break;
        }
    }
    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) override {
        bounding_rect = gfxm::rect(
            pos, pos + size
        );
        client_area = bounding_rect;
        
        gfxm::vec2 cur = client_area.min;
        gfxm::rect rc_content = rc;
        cur.y += GUI_PADDING;
        rc_content.max.x = rc_content.min.x + 200.0f;
        float min_width = .0f;
        for (int i = 0; i < childCount(); ++i) {
            auto ch = getChild(i);
            ch->layout(cur, rc_content, flags);
            cur.y += ch->getBoundingRect().max.y - ch->getBoundingRect().min.y;
            if (ch->getBoundingRect().max.x - ch->getBoundingRect().min.x > min_width) {
                min_width = ch->getBoundingRect().max.x - ch->getBoundingRect().min.x;
            }
        }
        bounding_rect.max.x = bounding_rect.min.x + min_width;
        client_area = bounding_rect;
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
inline GuiMenuListItem::GuiMenuListItem(const char* cap, const std::initializer_list<GuiMenuListItem*>& child_items)
    : caption(guiGetDefaultFont()) {
    caption.replaceAll(cap, strlen(cap));
    menu_list.reset(new GuiMenuList);
    menu_list->setOwner(this);
    menu_list->is_hidden = true;
    guiGetRoot()->addChild(menu_list.get());
    for (auto ch : child_items) {
        menu_list->addItem(ch);
    }
    icon_arrow = guiLoadIcon("svg/entypo/triangle-right.svg");
}
inline void GuiMenuListItem::open() {
    menu_list->open();
    menu_list->pos = gfxm::vec2(client_area.max.x, client_area.min.y);
    menu_list->size = gfxm::vec2(200, 200);
    is_open = true;
}
inline void GuiMenuListItem::close() {
    menu_list->close();
    is_open = false;
}

class GuiComboBox : public GuiElement {
    GuiTextBuffer text_content;

    gfxm::vec2 pos_content;

    gfxm::vec2 mouse_pos;
    bool pressing = false;

    bool is_open = false;
    GuiIcon* current_icon = 0;
public:
    GuiComboBox()
    : text_content(guiGetDefaultFont()) {
        text_content.putString("ComboBox", strlen("ComboBox"));

        current_icon = guiLoadIcon("svg/entypo/triangle-down.svg");
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
        const float client_area_height = text_box_height + font->getLineHeight() * .5f;
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
        float fontH = guiGetCurrentFont()->font->getLineHeight();
        float fontLG = guiGetCurrentFont()->font->getLineGap();
        if (current_icon) {
            current_icon->draw(gfxm::rect(gfxm::vec2(client_area.max.x - fontH * 2.f, pos_content.y), gfxm::vec2(client_area.max.x - fontH, pos_content.y + fontH)), GUI_COL_TEXT);
        }
        text_content.draw(pos_content, GUI_COL_TEXT, GUI_COL_ACCENT);
    }
};

class GuiCollapsingHeader : public GuiElement {
    GuiTextBuffer caption;
    gfxm::vec2 pos_caption;
    bool is_open = false;

    GuiIcon* current_icon = 0;
public:
    GuiCollapsingHeader()
        : caption(guiGetDefaultFont()) {
        caption.putString("CollapsingHeader", strlen("CollapsingHeader"));
        current_icon = guiLoadIcon("svg/entypo/triangle-right.svg");
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
                current_icon = guiLoadIcon("svg/entypo/triangle-down.svg");
            } else {
                is_open = false;
                current_icon = guiLoadIcon("svg/entypo/triangle-right.svg");
            }
            break;
        }

        GuiElement::onMessage(msg, params);
    }
    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) override {
        Font* font = guiGetCurrentFont()->font;

        const float text_box_height = font->getLineHeight() + GUI_PADDING * 2.0f;
        const float client_area_height = text_box_height + font->getLineHeight() * .5f;
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
        float fontH = guiGetCurrentFont()->font->getLineHeight();
        float fontLG = guiGetCurrentFont()->font->getLineGap();
        if (current_icon) {
            current_icon->draw(gfxm::rect(pos_caption, pos_caption + gfxm::vec2(fontH, fontH)), GUI_COL_TEXT);
        }
        caption.draw(pos_caption + gfxm::vec2(fontH + GUI_MARGIN, .0f), GUI_COL_TEXT, GUI_COL_ACCENT);
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

class GuiFileContainer : public GuiElement {
    int visible_items_begin = 0;
    int visible_items_end = 0;
    gfxm::vec2 smooth_scroll = gfxm::vec2(.0f, .0f);

    std::unique_ptr<GuiScrollBarV> scroll_bar_v;
    gfxm::rect rc_scroll_v;
    gfxm::rect rc_scroll_h;
public:
    gfxm::vec2 scroll_offset = gfxm::vec2(.0f, .0f);

    GuiFileContainer() {
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
class GuiContainer : public GuiElement {
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
        for (int i = 0; i < childCount(); ++i) {
            auto ch = getChild(i);
            hit = ch->hitTest(x, y);
            if (hit.hasHit()) {
                return hit;
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

        for (int i = 0; i < childCount(); ++i) {
            auto ch = getChild(i);
            ch->layout(rc_child.min, rc_child, flags);

            rc_child.min.y += ch->getBoundingRect().max.y - ch->getBoundingRect().min.y;
        }

        gfxm::vec2 content_size(rc_content.max.x - rc_content.min.x, rc_child.min.y);
        if (content_size.y > rc_content.max.y - rc_content.min.y) {
            scroll_bar_v->setEnabled(true);
            //rc_content.max.x -= 10.0f;
            //content_size = updateContentLayout();
        } else {
            scroll_bar_v->setEnabled(false);
        }
        
        scroll_bar_v->layout(cursor, client_area, 0);
    }
    void onDraw() override {
        guiDrawRect(client_area, GUI_COL_HEADER);
        guiDrawPushScissorRect(client_area);
        for (int i = 0; i < childCount(); ++i) {
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

        auto container = new GuiFileContainer();
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
                getOwner()->notify<GuiNodeInput*>(GUI_NOTIFY::NODE_INPUT_CLICKED, this);
            }
            break;
        case GUI_MSG::RBUTTON_DOWN:
            assert(getOwner());
            if (getOwner()) {
                getOwner()->notify<GuiNodeInput*>(GUI_NOTIFY::NODE_INPUT_BREAK, this);
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
                getOwner()->notify<GuiNodeOutput*>(GUI_NOTIFY::NODE_OUTPUT_CLICKED, this);
            }
            break;
        case GUI_MSG::RBUTTON_DOWN:
            assert(getOwner());
            if (getOwner()) {
                getOwner()->notify<GuiNodeOutput*>(GUI_NOTIFY::NODE_OUTPUT_BREAK, this);
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
            getOwner()->sendMessage(GUI_MSG::NOTIFY, (uint64_t)GUI_NOTIFY::NODE_CLICKED, (uint64_t)this);
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
            switch (params.getA<GUI_NOTIFY>()) {
            case GUI_NOTIFY::NODE_CLICKED: {
                GuiNode* node = params.getB<GuiNode*>();
                setSelectedNode(node);
                }break;
            case GUI_NOTIFY::NODE_INPUT_CLICKED: {
                GuiNodeInput* inp = params.getB<GuiNodeInput*>();
                if (link_preview.is_active && link_preview.is_output) {
                    link(link_preview.out, inp);
                    stopLinkPreview();
                } else {
                    startLinkPreview(0, inp, inp->getCirclePosition(), false);
                }
                }break;
            case GUI_NOTIFY::NODE_OUTPUT_CLICKED: {
                GuiNodeOutput* out = params.getB<GuiNodeOutput*>();
                if (link_preview.is_active && !link_preview.is_output) {
                    link(out, link_preview.in);
                    stopLinkPreview();
                } else {
                    startLinkPreview(out, 0, out->getCirclePosition(), true);
                }
                }break;
            case GUI_NOTIFY::NODE_INPUT_BREAK: {
                GuiNodeInput* in = params.getB<GuiNodeInput*>();
                linkBreak(in);
                }break;
            case GUI_NOTIFY::NODE_OUTPUT_BREAK: {
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

#include "gui/elements/animation/gui_timeline_editor.hpp"