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
public:
    GuiButton(Font* fnt)
    : font(fnt) {
        this->size = gfxm::vec2(130.0f, 30.0f);
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
        this->bounding_rect = gfxm::rect(
            rc.min,
            rc.min + size + gfxm::vec2(GUI_MARGIN, GUI_MARGIN) * 2.0f
        );
        this->client_area = gfxm::rect(
            bounding_rect.min + gfxm::vec2(GUI_MARGIN, GUI_MARGIN),
            bounding_rect.max - gfxm::vec2(GUI_MARGIN, GUI_MARGIN)
        );
    }

    void onDraw() override {
        uint32_t col = GUI_COL_BUTTON;
        if (pressed) {
            col = GUI_COL_ACCENT;
        } else if (hovered) {
            col = GUI_COL_BUTTON_HOVER;
        }
        guiDrawRect(client_area, col);

        gfxm::vec2 sz = guiCalcTextRect(caption.c_str(), font, .0f);
        gfxm::vec2 pn = client_area.min - sz / 2 + size / 2;
        pn.y -= sz.y * 0.25f;
        pn.x = ceilf(pn.x);
        pn.y = ceilf(pn.y);
        guiDrawText(pn, caption.c_str(), font, .0f, GUI_COL_TEXT);
    }
};

class GuiTextBox : public GuiElement {
    Font* font = 0;
    std::string caption = "TextBox";
public:
    GuiTextBox(Font* fnt)
        : font(fnt) {
    }

    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        this->bounding_rect = gfxm::rect(
            rc.min,
            gfxm::vec2(rc.max.x, rc.min.y + 30.0f + GUI_MARGIN * 2.0f)
        );
        this->client_area = gfxm::rect(
            bounding_rect.min + gfxm::vec2(GUI_MARGIN, GUI_MARGIN),
            bounding_rect.max - gfxm::vec2(GUI_MARGIN, GUI_MARGIN)
        );
    }

    void onDraw() override {
        gfxm::vec2 pn = client_area.min;
        pn.x = ceilf(pn.x);
        pn.y = ceilf(pn.y);
        float width = client_area.max.x - client_area.min.x;

        gfxm::vec2 sz = guiCalcTextRect(caption.c_str(), font, .0f);
        guiDrawText(pn, caption.c_str(), font, .0f, GUI_COL_TEXT);
        float box_width = width - sz.x - GUI_MARGIN;
        pn.x += sz.x + GUI_MARGIN;

        uint32_t col = GUI_COL_HEADER;
        guiDrawRect(gfxm::rect(pn, pn + gfxm::vec2(box_width, size.y)), col);

        pn.x += GUI_MARGIN;
        guiDrawText(pn, "The quick brown fox jumps over the lazy dog", font, .0f, GUI_COL_TEXT);
    }
};
