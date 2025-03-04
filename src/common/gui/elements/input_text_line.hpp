#pragma once

#include "gui/gui_system.hpp"
#include "element.hpp"
#include "gui/gui_text_buffer.hpp"


class GuiInputTextLine : public GuiElement {
    GuiTextBuffer text_content;

    gfxm::vec2 pos_content;

    gfxm::vec2 mouse_pos;
    bool pressing = false;
    std::string* output = 0;
    std::function<void(const std::string&)> set_cb = nullptr;
    std::function<std::string(void)>        get_cb = nullptr;

    void updateOutput() {
        if (output) {
            output->clear();
            text_content.getWholeText(*output);
        }
        if (set_cb) {
            std::string text;
            text_content.getWholeText(text);
            set_cb(text);
        }
    }
public:
    GuiInputTextLine(std::string* output) {
        setSize(0, gui::em(2));
        setMaxSize(0, 0);
        setMinSize(250, 0);
        setStyleClasses({ "control" });
        if (output) {
            text_content.replaceAll(getFont(), output->c_str(), output->size());
        }
    }
    GuiInputTextLine(const std::function<void(const std::string&)>& set_cb, const std::function<std::string(void)>& get_cb)
        : output(0), set_cb(set_cb), get_cb(get_cb) {
        setSize(0, 0);
        setMaxSize(0, 0);
        setMinSize(250, 0);
        
        std::string text = get_cb();
        text_content.replaceAll(getFont(), text.data(), text.size());
    }
    GuiInputTextLine(const char* text = "Text") {
        setSize(0, 0);
        setMaxSize(0, 0);
        setMinSize(250, 0);
        text_content.putString(getFont(), text, strlen(text));
    }

    void setText(const char* text) {
        text_content.selectAll();
        text_content.eraseSelected();
        text_content.putString(getFont(), text, strlen(text));
    }
    std::string getText() {
        std::string out;
        text_content.getWholeText(out);
        return out;
    }

    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::FOCUS:
            return true;
        case GUI_MSG::UNFOCUS:
            text_content.clearSelection();
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
                updateOutput();
                break;
            // CTRL+V key
            case 0x0056: if (guiGetModifierKeysState() & GUI_KEY_CONTROL) {
                std::string str;
                if (guiClipboardGetString(str)) {
                    text_content.putString(getFont(), str.c_str(), str.size());
                    updateOutput();
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
                updateOutput();
                break;
            case GUI_CHAR::ESCAPE:
                break;
            case GUI_CHAR::TAB:
                text_content.putString(getFont(), "\t", 1);
                updateOutput();
                break;
            default: {
                char ch = (char)params.getA<GUI_CHAR>();
                if (ch > 0x1F || ch == 0x0A) {
                    text_content.putString(getFont(), &ch, 1);
                    updateOutput();
                }                
            }
            }
            return true;
        case GUI_MSG::MOUSE_MOVE:
            mouse_pos = gfxm::vec2(params.getA<int32_t>(), params.getB<int32_t>());
            if (pressing) {
                text_content.pickCursor(getFont(), mouse_pos - pos_content, false);
            }
            return true;
        case GUI_MSG::LBUTTON_DOWN:
            guiCaptureMouse(this);
            pressing = true;

            text_content.pickCursor(getFont(), mouse_pos - pos_content, true);
            return true;
        case GUI_MSG::LBUTTON_UP:
            pressing = false;
            return true;
        }

        return GuiElement::onMessage(msg, params);
    }
    void onLayout(const gfxm::vec2& extents, uint64_t flags) override {
        Font* font = getFont();
        const float text_box_height = font->getLineHeight() * 2.f;

        setHeight(text_box_height);
        rc_bounds = gfxm::rect(
            gfxm::vec2(0, 0),
            gfxm::vec2(extents.x, text_box_height)
        );
        client_area = rc_bounds;

        text_content.prepareDraw(guiGetDefaultFont(), true);

        gfxm::rect text_rc = guiLayoutPlaceRectInsideRect(
            client_area, text_content.getBoundingSize(), GUI_LEFT | GUI_VCENTER,
            gfxm::rect(GUI_MARGIN, 0, 0, 0)
        );
        pos_content = text_rc.min;
    }
    void onDraw() override {
        guiDrawRectRound(client_area, GUI_PADDING * 2.f, GUI_COL_HEADER);
        if (guiGetFocusedWindow() == this) {
            guiDrawRectRoundBorder(client_area, GUI_PADDING * 2.f, 2.f, GUI_COL_ACCENT, GUI_COL_ACCENT);
        }
        text_content.draw(getFont(), pos_content, GUI_COL_TEXT, GUI_COL_ACCENT);
    }
};
