#pragma once

#include "gui/elements/gui_label.hpp"


inline void guiLayoutSplitRect2XRatio(gfxm::rect& rc, gfxm::rect& reminder, float ratio) {
    const float w = rc.max.x - rc.min.x;
    const float w0 = w * ratio;
    const float w1 = w - w0;

    reminder = rc;
    rc.max.x = rc.min.x + w0;
    reminder.min.x = rc.max.x;
}
inline void guiLayoutSplitRect2X(gfxm::rect& rc, gfxm::rect& reminder, float left_size) {
    const float w = rc.max.x - rc.min.x;
    const float w0 = gfxm::_min(left_size, w);
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
        rects[i].max.x = rects[i].min.x + w_single;
        rects[i].max.y = total.max.y;
    }
}

enum GUI_INPUT_TYPE {
    GUI_INPUT_TEXT,
    GUI_INPUT_INT,
    GUI_INPUT_FLOAT
};

class GuiInputBox_ : public GuiElement {
    GUI_INPUT_TYPE type = GUI_INPUT_TEXT;
    GuiTextBuffer text_content;
    gfxm::rect rc_text;

    gfxm::vec2 mouse_pos;
    bool pressing = false;
    bool dragging = false;
    bool editing = false;

    union {
        int     _int = 0;
        float   _float;
    };

    void updateValue() {
        if (type == GUI_INPUT_TEXT) {
            return;
        }

        std::string str;
        text_content.getWholeText(str);

        if(type == GUI_INPUT_INT) {
            char* e = 0;
            errno = 0;
            long val = strtol(str.c_str(), &e, 0);
            if (errno != 0) {
                setInt(val);
            } else {
                setInt(val);
            }
        } else if(type == GUI_INPUT_FLOAT) {
            char* e = 0;
            errno = 0;
            float val = strtof(str.c_str(), &e);
            if (errno != 0) {
                setFloat(val);
            } else {
                setFloat(val);
            }
        }
    }
public:
    GuiInputBox_(GUI_INPUT_TYPE type = GUI_INPUT_TYPE::GUI_INPUT_TEXT)
    : text_content(guiGetDefaultFont()) {
        setSize(0, 0);
        setMaxSize(0, 0);
        setMinSize(0, 0);

        setType(type);
    }

    void setType(GUI_INPUT_TYPE t) {
        type = t;
        switch (t) {
        case GUI_INPUT_TEXT:
            setText("");
            break;
        case GUI_INPUT_INT:
            setInt(0);
            break;
        case GUI_INPUT_FLOAT:
            setFloat(.0f);
            break;
        }
    }

    void setText(const char* str) {
        text_content.replaceAll(str, strnlen_s(str, 1024));
    }
    void setInt(int val) { 
        _int = val;
        char buf[64];
        int len = snprintf(buf, 64, "%i", _int);
        text_content.replaceAll(buf, len);
    }
    void setFloat(float val) { 
        _float = val;
        char buf[64];
        int len = snprintf(buf, 64, "%.2f", _float);
        text_content.replaceAll(buf, len);
    }
    std::string getText() { 
        std::string str;
        text_content.getWholeText(str);
        return str;
    }
    int getInt() const { return _int; }
    int getFloat() const { return _float; }

    GuiHitResult onHitTest(int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }
        return GuiHitResult{ GUI_HIT::CLIENT, this };
    }
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::FOCUS:
            return true;
        case GUI_MSG::UNFOCUS:
            if (editing) {
                updateValue();
                editing = false;
                text_content.clearSelection();
            }
            return true;
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
            return true;
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
                    text_content.clearSelection();
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
            return true;
        case GUI_MSG::MOUSE_MOVE: {
            gfxm::vec2 new_mouse_pos = gfxm::vec2(params.getA<int32_t>(), params.getB<int32_t>());
            if (pressing && guiGetFocusedWindow() == this) {
                if (editing) {
                    text_content.pickCursor(new_mouse_pos - rc_text.min, false);
                } else if(type == GUI_INPUT_INT) {
                    dragging = true;
                    setInt(getInt() + (new_mouse_pos.x - mouse_pos.x));                    
                } else if(type == GUI_INPUT_FLOAT) {
                    dragging = true;
                    setFloat(getFloat() + (new_mouse_pos.x - mouse_pos.x) * 0.01f);                    
                }
            }
            mouse_pos = new_mouse_pos;
            } return true;
        case GUI_MSG::LBUTTON_DOWN:
            guiCaptureMouse(this);
            pressing = true;
            if (type == GUI_INPUT_TEXT) {
                editing = true;
            }
            if (editing) {
                text_content.pickCursor(mouse_pos - rc_text.min, true);
            }
            return true;
        case GUI_MSG::LBUTTON_UP:
            pressing = false;
            dragging = false;
            return true;
        case GUI_MSG::LCLICK:
            if (type == GUI_INPUT_TEXT) {
                editing = true;
            }
            return true;
        case GUI_MSG::DBL_LCLICK:
            editing = true;
            return true;
        }

        return GuiElement::onMessage(msg, params);
    }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        Font* font = guiGetCurrentFont()->font;

        const float text_box_height = font->getLineHeight() * 2.0f;
        setHeight(text_box_height);
        rc_bounds = gfxm::rect(
            rc.min,
            gfxm::vec2(rc.max.x, rc.min.y + text_box_height)
        );
        client_area = rc_bounds;

        text_content.prepareDraw(guiGetDefaultFont(), true);
        if (type == GUI_INPUT_TEXT) {
            gfxm::rect rc = client_area;
            rc.min.x += GUI_PADDING * 2.f;
            rc_text = text_content.calcTextRect(rc, GUI_VCENTER | GUI_LEFT, 0);
        } else {
            rc_text = text_content.calcTextRect(client_area, GUI_VCENTER | GUI_HCENTER, 0);
        }
    }
    void onDraw() override {
        guiDrawPushScissorRect(client_area);
        
        uint32_t col_box = GUI_COL_HEADER;
        if (isHovered() && !editing) {
            col_box = GUI_COL_BUTTON;
        }
        guiDrawRectRound(client_area, GUI_PADDING * 2.f, col_box);
        if (editing) {
            guiDrawRectRoundBorder(client_area, GUI_PADDING * 2.f, 2.f, GUI_COL_ACCENT, GUI_COL_ACCENT);
        }
        text_content.draw(rc_text.min, GUI_COL_TEXT, GUI_COL_TEXT);

        guiDrawPopScissorRect();
    }
};

class GuiInput : public GuiElement {
    std::unique_ptr<GuiLabel> label;
    std::unique_ptr<GuiInputBox_> input_box;
public:
    GuiInput(const char* caption = "Input", GUI_INPUT_TYPE type = GUI_INPUT_TYPE::GUI_INPUT_TEXT) {
        setSize(0, 0);
        setMaxSize(0, 0);
        setMinSize(0, 0);

        label.reset(new GuiLabel(caption));
        input_box.reset(new GuiInputBox_(type));
        
        addChild(label.get());
        addChild(input_box.get());
    }

    void setType(GUI_INPUT_TYPE t) {
        input_box->setType(t);
    }
    void setCaption(const char* str) {
        label->setCaption(str);
    }
    void setText(const char* str) {
        input_box->setText(str);
    }
    void setInt(int value) {
        input_box->setInt(value);
    }
    void setFloat(float value) {
        input_box->setFloat(value);
    }

    GuiHitResult onHitTest(int x, int y) override {
        return input_box->onHitTest(x, y);
    }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        setHeight(guiGetCurrentFont()->font->getLineHeight() * 2.f);
        gfxm::rect rc_label = rc;
        gfxm::rect rc_inp;
        guiLayoutSplitRect2XRatio(rc_label, rc_inp, .25f);

        label->layout(rc_label, flags);
        input_box->layout(rc_inp, flags);

        rc_bounds = label->getBoundingRect();
        gfxm::expand(rc_bounds, input_box->getBoundingRect());
    }
    void onDraw() override {
        label->draw();
        input_box->draw();
    }
};