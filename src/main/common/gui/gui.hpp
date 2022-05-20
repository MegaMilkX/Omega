#pragma once

#include <assert.h>
#include <stack>
#include "platform/platform.hpp"
#include "common/math/gfxm.hpp"
#include "common/render/gpu_pipeline.hpp"
#include "common/render/gpu_text.hpp"

#include "common/gui/gui_draw.hpp"

#include "common/gui/gui_values.hpp"
#include "common/gui/gui_color.hpp"

#include "common/gui/gui_system.hpp"

#include "common/gui/elements/gui_dock_space.hpp"
#include "common/gui/elements/gui_text.hpp"


class GuiText : public GuiElement {
    Font* font = 0;

    const char* string = R"(Quidem quidem sapiente voluptates necessitatibus tenetur sed consequatur ex. Quidem ducimus eos suscipit sapiente aut. Eveniet molestias deserunt corrupti ea et rerum. Quis autem labore est aut doloribus eum aut sed. Non temporibus quos debitis autem cum.

Quae qui quis assumenda et dolorem. Id molestiae nesciunt error eos. Id ea perferendis non doloribus sint modi nisi recusandae. Sit laboriosam minus id. Dolore et laboriosam aut nisi omnis. Laudantium nisi fugiat iure.

Non sint tenetur reiciendis tempore sequi ipsam. Eum numquam et consequatur voluptatem. Aut et delectus aspernatur. Amet suscipit quia qui consequatur. Libero tenetur rerum ipsa et.

Adipisci dicta impedit similique a laborum beatae quas ea. Nulla fugit dicta enim odit tempore. Distinctio totam adipisci amet et quos.

Ea corrupti nesciunt blanditiis molestiae occaecati praesentium asperiores voluptatem. Odit nisi error provident autem quasi. Minus ipsa in nesciunt enim at.
        )";

public:
    GuiText(Font* fnt)
    : font(fnt) {
        
    }

    void onMessage(GUI_MSG msg, uint64_t a_param, uint64_t b_param) override {
        switch (msg) {
        case GUI_MSG::PAINT: {
            } break;
        }

        GuiElement::onMessage(msg, a_param, b_param);
    }

    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        gfxm::vec2 sz = guiCalcTextRect(string, font, rc.max.x - rc.min.x - GUI_MARGIN * 2.0f);
        this->bounding_rect = gfxm::rect(
            rc.min,
            rc.min + gfxm::vec2(rc.max.x - rc.min.x, sz.y + GUI_MARGIN * 2.0f)
        );
        this->client_area = gfxm::rect(
            bounding_rect.min + gfxm::vec2(GUI_MARGIN, GUI_MARGIN),
            bounding_rect.max - gfxm::vec2(GUI_MARGIN, GUI_MARGIN)
        );
    }

    void onDraw() override {
        guiDrawText(client_area.min, string, font, client_area.max.x - client_area.min.x, GUI_COL_TEXT);
        //guiDrawRectLine(bounding_rect);
        //guiDrawRectLine(client_area);
    }
};

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
    Font* font = 0;

    std::string caption = "Button1";
    bool hovered = false;
    bool pressed = false;

    gfxm::vec2 text_bb_size;
    gfxm::vec2 text_pos;
public:
    GuiButton(Font* fnt)
    : font(fnt) {
        this->size = gfxm::vec2(130.0f, 30.0f);
    }

    GuiHitResult hitTest(int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }

        return GuiHitResult{ GUI_HIT::CLIENT, this };
    }

    void onMessage(GUI_MSG msg, uint64_t a_param, uint64_t b_param) override {
        switch (msg) {
        case GUI_MSG::MOUSE_ENTER:
            hovered = true;
            break;
        case GUI_MSG::MOUSE_LEAVE: {
            hovered = false;
            } break;
        case GUI_MSG::LBUTTON_DOWN:
            if (hovered) {
                pressed = true;
                guiCaptureMouse(this);
            }
            break;
        case GUI_MSG::LBUTTON_UP:
            if (pressed) {
                guiCaptureMouse(0);
                if (hovered) {
                    // TODO: Button clicked
                    LOG_WARN("Button clicked!");
                }
            }
            pressed = false;
            break;
        }

        GuiElement::onMessage(msg, a_param, b_param);
    }

    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        text_bb_size = guiCalcTextRect(caption.c_str(), font, .0f);
        size.y = font->getLineHeight() * 2.0f;

        bounding_rect = gfxm::rect(
            rc.min,
            rc.min + size + gfxm::vec2(GUI_MARGIN, GUI_MARGIN) * 2.0f
        );
        client_area = gfxm::rect(
            bounding_rect.min + gfxm::vec2(GUI_MARGIN, GUI_MARGIN),
            bounding_rect.max - gfxm::vec2(GUI_MARGIN, GUI_MARGIN)
        );
        text_pos = guiCalcTextPosInRect(gfxm::rect(gfxm::vec2(0, 0), text_bb_size), client_area, 0, gfxm::rect(0, 0, 0, 0), font);
    }

    void onDraw() override {
        uint32_t col = GUI_COL_BUTTON;
        if (pressed) {
            col = GUI_COL_ACCENT;
        } else if (hovered) {
            col = GUI_COL_BUTTON_HOVER;
        }
        guiDrawRect(client_area, col);

        guiDrawText(text_pos, caption.c_str(), font, .0f, GUI_COL_TEXT);
    }
};


class GuiInputTextLine : public GuiElement {
    Font* font = 0;
    GuiTextBuffer text_content;

    gfxm::vec2 pos_content;

    gfxm::vec2 mouse_pos;
    bool pressing = false;
public:
    GuiInputTextLine(Font* font)
    : font(font), text_content(font) {
        text_content.putString("MyTextString", strlen("MyTextString"));
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
                if (IsClipboardFormatAvailable(CF_TEXT) && OpenClipboard(0)) {
                    HGLOBAL hglb = { 0 };
                    hglb = GetClipboardData(CF_TEXT);
                    if (hglb != NULL) {
                        LPTSTR lptstr = (LPTSTR)GlobalLock(hglb);
                        if (lptstr != NULL) {
                            std::string str = lptstr;
                            text_content.putString(str.c_str(), str.size());
                            GlobalUnlock(hglb);
                        }
                    }
                    CloseClipboard();
                }
            } break;
            // CTRL+C, CTRL+X
            case 0x0043: // C
            case 0x0058: /* X */ if (guiGetModifierKeysState() & GUI_KEY_CONTROL) {
                std::string copied;
                if (text_content.copySelected(copied, a_param == 0x0058)) {
                    if (OpenClipboard(0)) {
                        HGLOBAL hglbCopy = GlobalAlloc(GMEM_MOVEABLE, copied.size() + 1);
                        if (hglbCopy) {
                            EmptyClipboard();
                            LPTSTR lptstrCopy = (LPTSTR)GlobalLock(hglbCopy);
                            memcpy(lptstrCopy, copied.data(), copied.size());
                            lptstrCopy[copied.size() + 1] = '\0';
                            GlobalUnlock(hglbCopy);
                            SetClipboardData(CF_TEXT, hglbCopy);
                            CloseClipboard();
                        } else {
                            CloseClipboard();
                            assert(false);
                        }
                    }
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
        const float text_box_height = font->getLineHeight() + GUI_PADDING * 2.0f;
        const float client_area_height = text_box_height + GUI_MARGIN;
        bounding_rect = gfxm::rect(
            rc.min,
            gfxm::vec2(rc.max.x, rc.min.y + text_box_height + GUI_MARGIN * 2.f)
        );
        client_area = bounding_rect;
        gfxm::expand(client_area, -GUI_MARGIN);

        text_content.prepareDraw();

        const float content_y_offset = text_box_height * .5f - (font->getAscender() + font->getDescender()) * .5f;
        //pos_content = client_area.min + gfxm::vec2(GUI_PADDING, content_y_offset);
        pos_content = client_area.min + gfxm::vec2(.0f, content_y_offset);
        pos_content.x = client_area.min.x + (client_area.max.x - client_area.min.x) * .5f - text_content.getBoundingSize().x * .5f;
    }
    void onDraw() override {
        guiDrawRect(client_area, GUI_COL_HEADER);
        text_content.draw(pos_content, GUI_COL_TEXT, GUI_COL_ACCENT);
    }
};
