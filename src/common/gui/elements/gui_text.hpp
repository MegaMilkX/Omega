#pragma once

#include "gui/elements/gui_element.hpp"

#include "gui/gui_text_buffer.hpp"

class GuiTextBox : public GuiElement {    
    GuiTextBuffer text_cache;
    gfxm::ivec2 selected_lines;
    
    std::string caption = "TextBox";
    std::string text = R"(Then Fingolfin beheld (as it seemed to him) the utter ruin of the Noldor,
and the defeat beyond redress of all their houses;
and filled with wrath and despair he mounted upon Rochallor his great horse and rode forth alone,
and none might restrain him. He passed over Dor-nu-Fauglith like a wind amid the dust,
and all that beheld his onset fled in amaze, thinking that Orome himself was come:
for a great madness of rage was upon him, so that his eyes shone like the eyes of the Valar.
Thus he came alone to Angband's gates, and he sounded his horn,
and smote once more upon the brazen doors,
and challenged Morgoth to come forth to single combat. And Morgoth came.)";
    gfxm::rect rc_caption;
    gfxm::rect rc_box;
    gfxm::rect rc_text;
    
    gfxm::vec2 pressed_pos;
    gfxm::vec2 mouse_pos;
    bool pressing = false;
public:
    GuiTextBox()
    : text_cache(guiGetDefaultFont()) {

        text_cache.replaceAll(text.c_str(), text.size());
    }

    void onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::KEYDOWN:
            // TODO: Remove winapi usage
            switch (params.getA<uint16_t>()) {
            case VK_RIGHT: text_cache.advanceCursor(1, !(guiGetModifierKeysState() & GUI_KEY_SHIFT)); break;                
            case VK_LEFT: text_cache.advanceCursor(-1, !(guiGetModifierKeysState() & GUI_KEY_SHIFT)); break;
            case VK_UP: text_cache.advanceCursorLine(-1, !(guiGetModifierKeysState() & GUI_KEY_SHIFT)); break;
            case VK_DOWN: text_cache.advanceCursorLine(1, !(guiGetModifierKeysState() & GUI_KEY_SHIFT)); break;
            case VK_DELETE:
                text_cache.del();
                break;
            // CTRL+V key
            case 0x0056: if (guiGetModifierKeysState() & GUI_KEY_CONTROL) {
                std::string str;
                if (guiClipboardGetString(str)) {
                    text_cache.putString(str.c_str(), str.size());
                }
            } break;
            // CTRL+C, CTRL+X
            case 0x0043: // C
            case 0x0058: /* X */ if (guiGetModifierKeysState() & GUI_KEY_CONTROL) {
                std::string copied;
                if (text_cache.copySelected(copied, params.getA<uint16_t>() == 0x0058)) {
                    guiClipboardSetString(copied);
                }
            } break;
            // CTRL+A
            case 0x41: if (guiGetModifierKeysState() & GUI_KEY_CONTROL) { // A
                text_cache.selectAll();
            } break;
            }
            break;
        case GUI_MSG::UNICHAR:
            switch (params.getA<GUI_CHAR>()) {
            case GUI_CHAR::BACKSPACE:
                text_cache.backspace();
                break;
            case GUI_CHAR::ESCAPE:
                break;
            case GUI_CHAR::LINEFEED:
                text_cache.newline();
                break;
            case GUI_CHAR::RETURN:
                text_cache.newline();;
                break;
            case GUI_CHAR::TAB:
                text_cache.putString("\t", 1);
                break;
            default: {
                char ch = (char)params.getA<GUI_CHAR>();
                if (ch > 0x1F || ch == 0x0A) {
                    text_cache.putString(&ch, 1);
                }                
            }
            }
            break;
        case GUI_MSG::MOUSE_MOVE:
            mouse_pos = gfxm::vec2(params.getA<int32_t>(), params.getB<int32_t>());
            if (pressing) {
                text_cache.pickCursor(mouse_pos - rc_text.min, false);
            }
            break;
        case GUI_MSG::LBUTTON_DOWN:
            guiCaptureMouse(this);
            pressed_pos = mouse_pos;
            pressing = true;

            text_cache.pickCursor(mouse_pos - rc_text.min, true);
            break;
        case GUI_MSG::LBUTTON_UP:
            pressing = false;
            break;
        }

        GuiElement::onMessage(msg, params);
    }

    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) override {
        const float box_height = rc.max.y - rc.min.y - GUI_MARGIN * 2.0f;// font->getLineHeight() * 12.0f;
        this->bounding_rect = gfxm::rect(
            rc.min,
            gfxm::vec2(rc.max.x, rc.min.y + box_height + GUI_MARGIN * 2.0f)
        );
        this->client_area = gfxm::rect(
            bounding_rect.min + gfxm::vec2(GUI_MARGIN, GUI_MARGIN),
            bounding_rect.max - gfxm::vec2(GUI_MARGIN, GUI_MARGIN)
        );
        
        gfxm::vec2 sz = gfxm::vec2(.0f, .0f);//guiCalcTextRect(caption.c_str(), font, .0f);
        rc_caption = gfxm::rect(
            client_area.min, client_area.min + sz
        );
        rc_box = gfxm::rect(
            client_area.min + gfxm::vec2(sz.x, .0f),
            client_area.max
        );
        rc_text = gfxm::rect(
            rc_box.min + gfxm::vec2(GUI_MARGIN, GUI_MARGIN),
            rc_box.max - gfxm::vec2(GUI_MARGIN, GUI_MARGIN)
        );
        //text_cache.setMaxLineWidth(rc_text.max.x - rc_text.min.x);
    }

    void onDraw() override {
        //guiDrawText(rc_caption.min, caption.c_str(), font, .0f, GUI_COL_TEXT);
        
        uint32_t col = GUI_COL_HEADER;
        guiDrawRect(rc_box, col);
        if (guiGetFocusedWindow() == this) {
            guiDrawRectLine(rc_box, GUI_COL_BUTTON_HOVER);
        }

        guiDrawPushScissorRect(rc_box);
        text_cache.prepareDraw(guiGetCurrentFont(), true);
        text_cache.draw(rc_text.min, GUI_COL_TEXT, GUI_COL_TEXT_HIGHLIGHT);
        guiDrawPopScissorRect();
    }
};
