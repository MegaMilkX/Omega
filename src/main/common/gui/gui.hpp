#pragma once

#include "platform/platform.hpp"
#include "common/math/gfxm.hpp"
#include "common/render/gpu_pipeline.hpp"
#include "common/render/gpu_text.hpp"

#include "common/gui/gui_draw.hpp"

enum class GUI_MSG {
    PAINT,
    MOUSE_MOVE,
    MOUSE_ENTER,
    MOUSE_LEAVE,
    LBUTTON_DOWN,
    LBUTTON_UP,

    // SCROLL BAR
    SB_THUMB_TRACK
};

enum class GUI_HIT {
    NOWHERE,

    BORDER, // In the border of a window that does not have a sizing border.
    BOTTOM, // In the lower-horizontal border of a resizable window (the user can click the mouse to resize the window vertically).
    BOTTOMLEFT, // In the lower-left corner of a border of a resizable window (the user can click the mouse to resize the window diagonally).
    BOTTOMRIGHT, // In the lower-right corner of a border of a resizable window (the user can click the mouse to resize the window diagonally).
    CAPTION, // In a title bar.
    CLIENT, // In a client area.
    CLOSE, // In a Close button.
    GROWBOX, // In a size box
    HELP,  // In a Help button.
    HSCROLL, // In a horizontal scroll bar.
    LEFT, // In the left border of a resizable window (the user can click the mouse to resize the window horizontally).
    MENU, // In a menu.
    MAXBUTTON, // In a Maximize button.
    MINBUTTON, // In a Minimize button.
    REDUCE, // In a Minimize button.
    RIGHT, // In the right border of a resizable window(the user can click the mouse to resize the window horizontally).
    SIZE, // In a size box(same as GROWBOX).
    SYSMENU, // In a window menu or in a Close button in a child window.
    TOP, // In the upper - horizontal border of a window.
    TOPLEFT, //In the upper - left corner of a window border.
    TOPRIGHT, // In the upper - right corner of a window border.
    VSCROLL, // In the vertical scroll bar.
    ZOOM, // In a Maximize button.

    BOUNDING_RECT // subject for removal
};

const float GUI_MARGIN = 10.0f;
const float GUI_PADDING = 10.0f;

const uint32_t GUI_COL_WHITE        = 0xFFFFFFFF;
const uint32_t GUI_COL_BLACK        = 0xFF000000;
const uint32_t GUI_COL_TEXT         = 0xFFEEEEEE;
const uint32_t GUI_COL_BG           = 0xFF202020;
const uint32_t GUI_COL_HEADER       = 0xFF101010;
const uint32_t GUI_COL_BUTTON       = 0xFF323232;
const uint32_t GUI_COL_BUTTON_HOVER = 0xFF4F4F4F;

const uint32_t GUI_COL_ACCENT   = 0xFF47756C;


class GuiElement {
    GuiElement* parent = 0;

    bool is_enabled = true;
protected:
    std::vector<GuiElement*> children;

    gfxm::rect bounding_rect = gfxm::rect(0, 0, 0, 0);
    gfxm::rect client_area = gfxm::rect(0, 0, 0, 0);
protected:
    GuiElement* getParent() {
        return parent;
    }
    const GuiElement* getParent() const {
        return parent;
    }
public:
    const gfxm::rect& getBoundingRect() const {
        return bounding_rect;
    }
    const gfxm::rect& getClientArea() const {
        return client_area;
    }
    bool isEnabled() const {
        return is_enabled;
    }

    void setEnabled(bool enabled) {
        is_enabled = enabled;
    }
public:
    gfxm::vec2 pos = gfxm::vec2(100.0f, 100.0f);
    gfxm::vec2 size = gfxm::vec2(150.0f, 100.0f);

    GuiElement();
    virtual ~GuiElement();

    virtual void init() {
        // TODO
    }

    virtual GUI_HIT hitTest(int x, int y) const {
        if (gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GUI_HIT::CLIENT;
        } else if(gfxm::point_in_rect(bounding_rect, gfxm::vec2(x, y))) {
            return GUI_HIT::BOUNDING_RECT;
        } else {
            return GUI_HIT::NOWHERE;
        }
    }

    virtual void onMessage(GUI_MSG msg, uint32_t a_param, uint32_t b_param) {
        switch (msg) {
        case GUI_MSG::PAINT: {
        } break;
        }
    }

    virtual void onLayout(const gfxm::rect& rect) {
        this->bounding_rect = rect;
        this->client_area = bounding_rect;

        for (auto& ch : children) {
            ch->onLayout(rect);
        }
    }

    virtual void onDraw() {
        int sw = 0, sh = 0;
        platformGetWindowSize(sw, sh);
        for (auto& ch : children) {
            glScissor(
                client_area.min.x,
                sh - client_area.max.y,
                client_area.max.x - client_area.min.x,
                client_area.max.y - client_area.min.y
            );
            ch->onDraw();
        }

        //guiDrawRectLine(bounding_rect);
    }

    void addChild(GuiElement* elem);
    size_t childCount() const;
    GuiElement* getChild(int i);
};

void guiPostMessage(GUI_MSG msg);
void guiPostMouseMove(int x, int y);

void guiCaptureMouse(GuiElement* e);


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

    void onMessage(GUI_MSG msg, uint32_t a_param, uint32_t b_param) override {
        switch (msg) {
        case GUI_MSG::PAINT: {
            } break;
        }

        GuiElement::onMessage(msg, a_param, b_param);
    }

    void onLayout(const gfxm::rect& rc) override {
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
    void onLayout(const gfxm::rect& rc) override {
        this->bounding_rect = gfxm::rect(
            rc.min,
            rc.min + gfxm::vec2(texture->getWidth(), texture->getHeight()) + gfxm::vec2(GUI_MARGIN, GUI_MARGIN) * 2.0f
        );
        this->client_area = bounding_rect;
    }

    void onDraw() override {
        //guiDrawRectTextured(client_area, texture, GUI_COL_WHITE);
        guiDrawColorWheel(client_area);
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

    void onMessage(GUI_MSG msg, uint32_t a_param, uint32_t b_param) override {
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

    void onLayout(const gfxm::rect& rc) override {
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

    void onLayout(const gfxm::rect& rc) override {
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

class GuiScrollBarV : public GuiElement {
    gfxm::rect rc_thumb;

    bool hovered = false;
    bool hovered_thumb = false;
    bool pressed_thumb = false;

    float thumb_h = 10.0f;
    float current_scroll = .0f;
    float max_scroll = .0f;

    float owner_content_max_scroll = .0f;

    gfxm::vec2 mouse_pos;
public:
    GuiScrollBarV() {
    }

    void setScrollData(float page_height, float total_content_height) {
        int full_page_count = (int)(total_content_height / page_height);
        int page_count = full_page_count + 1;
        float diff = page_height * page_count - total_content_height;
        owner_content_max_scroll = full_page_count * page_height - diff;

        float ratio = page_height / total_content_height;
        thumb_h = (client_area.max.y - client_area.min.y) * ratio;
        thumb_h = gfxm::_max(10.0f, thumb_h);
        thumb_h = gfxm::_min((client_area.max.y - client_area.min.y), thumb_h);
    }

    void onMessage(GUI_MSG msg, uint32_t a_param, uint32_t b_param) override {
        switch (msg) {
        case GUI_MSG::MOUSE_MOVE: {
            gfxm::vec2 cur_mouse_pos = gfxm::vec2(a_param, b_param);
            if (gfxm::point_in_rect(rc_thumb, gfxm::vec2(a_param, b_param))) {
                hovered_thumb = true;
            } else {
                hovered_thumb = false;
            }

            if (pressed_thumb) {
                current_scroll += cur_mouse_pos.y - mouse_pos.y;
                current_scroll = gfxm::_min(max_scroll, current_scroll);
                current_scroll = gfxm::_max(.0f, current_scroll);
                if (getParent()) {
                    getParent()->onMessage(GUI_MSG::SB_THUMB_TRACK, owner_content_max_scroll * (current_scroll / max_scroll), 0);
                }
            }
            mouse_pos = cur_mouse_pos;
            } break;
        case GUI_MSG::MOUSE_ENTER:
            hovered = true;
            break;
        case GUI_MSG::MOUSE_LEAVE: {
            hovered = false;
            hovered_thumb = false;
        } break;
        case GUI_MSG::LBUTTON_DOWN:
            if (hovered_thumb) {
                pressed_thumb = true;
                guiCaptureMouse(this);
            }
            break;
        case GUI_MSG::LBUTTON_UP:
            if (pressed_thumb) {
                guiCaptureMouse(0);
                if (hovered_thumb) {
                    // TODO: Button clicked
                    LOG_WARN("Thumb released while hovering");
                }
            }
            pressed_thumb = false;
            break;
        }

        GuiElement::onMessage(msg, a_param, b_param);
    }
    void onLayout(const gfxm::rect& rc) override {
        this->bounding_rect = gfxm::rect(
            gfxm::vec2(rc.max.x - 10.0f, rc.min.y),
            rc.max
        );
        this->client_area = this->bounding_rect;

        max_scroll = (client_area.max.y - client_area.min.y) - thumb_h;

        gfxm::vec2 rc_thumb_min = client_area.min + gfxm::vec2(.0f, current_scroll);
        rc_thumb = gfxm::rect(
            rc_thumb_min,
            gfxm::vec2(client_area.max.x, rc_thumb_min.y + thumb_h)
        );
    }
    void onDraw() override {
        if (!isEnabled()) {
            return;
        }
        guiDrawRect(client_area, GUI_COL_HEADER);

        uint32_t col = GUI_COL_BUTTON;
        if (pressed_thumb) {
            col = GUI_COL_ACCENT;
        } else if (hovered_thumb) {
            col = GUI_COL_BUTTON_HOVER;
        }
        guiDrawRect(rc_thumb, col);
    }
};

class GuiTitleBarBtnClose : public GuiElement {
    bool hovered = false;
    bool pressed = false;
public:
    GuiTitleBarBtnClose() {

    }

    void onMessage(GUI_MSG msg, uint32_t a_param, uint32_t b_param) override {
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

    void onLayout(const gfxm::rect& rc) override {
        this->bounding_rect = gfxm::rect(
            gfxm::vec2(rc.max.x - 30.0f, rc.min.y),
            gfxm::vec2(rc.max.x, rc.max.y)
        );
        this->client_area = gfxm::rect(
            bounding_rect.min,
            bounding_rect.max
        );
    }

    void onDraw() override {
        uint32_t col = GUI_COL_BUTTON;
        if (pressed) {
            col = GUI_COL_ACCENT;
        }
        else if (hovered) {
            col = GUI_COL_BUTTON_HOVER;
        }
        guiDrawRect(client_area, col);
    }
};
class GuiWindowTitleBar : public GuiElement {
    std::string title = "MyWindow";

    Font* font = 0;

    bool hovered = false;
    bool pressed = false;
    gfxm::vec2 mouse_pos;
    gfxm::vec2 pressed_mouse_pos_lcl;

    GuiTitleBarBtnClose* btn_close = 0;
public:
    GuiWindowTitleBar(Font* fnt)
    : font(fnt) {
        btn_close = new GuiTitleBarBtnClose();
        addChild(btn_close);
    }

    void onMessage(GUI_MSG msg, uint32_t a_param, uint32_t b_param) override {
        switch (msg) {
        case GUI_MSG::MOUSE_MOVE:
            mouse_pos = gfxm::vec2(a_param, b_param);
            if (pressed && getParent()) {
                getParent()->pos = mouse_pos - pressed_mouse_pos_lcl;
            }
            break;
        case GUI_MSG::MOUSE_ENTER:
            hovered = true;
            break;
        case GUI_MSG::MOUSE_LEAVE: {
            hovered = false;
        } break;
        case GUI_MSG::LBUTTON_DOWN:
            if (hovered) {
                pressed = true;
                pressed_mouse_pos_lcl = mouse_pos - getParent()->pos;
                guiCaptureMouse(this);
            }
            break;
        case GUI_MSG::LBUTTON_UP:
            if (pressed) {
                guiCaptureMouse(0);
            }
            pressed = false;
            break;
        }
        GuiElement::onMessage(msg, a_param, b_param);
    }

    void onLayout(const gfxm::rect& rc) override {
        const int title_bar_height = 30.0f;

        this->bounding_rect = gfxm::rect(
            rc.min,
            rc.min + gfxm::vec2(rc.max.x - rc.min.x, title_bar_height)
        );
        this->client_area = gfxm::rect(
            bounding_rect.min,
            bounding_rect.max
        );

        btn_close->onLayout(client_area);
    }

    void onDraw() override {
        uint32_t col = GUI_COL_HEADER;
        if (hovered || pressed) {
            col = GUI_COL_BUTTON_HOVER;
        }
        guiDrawRect(client_area, col);
        guiDrawText(client_area.min + gfxm::vec2(GUI_MARGIN, .0f), title.c_str(), font, .0f, GUI_COL_TEXT);

        btn_close->onDraw();
    }
};
class GuiWindow : public GuiElement {
    enum RESERVED_CHILD_IDS {
        CHILD_TITLE_BAR_ID = 0,
        CHILD_SCROLLBAR_V,
        RESERVED_CHILD_COUNT
    };
    
    std::string title = "MyWindow";

    Font* font = 0;

    gfxm::rect rc_window;
    gfxm::rect rc_header;
    gfxm::rect rc_body;
    gfxm::rect rc_client;
    gfxm::rect rc_content;

    GuiWindowTitleBar* title_bar = 0;
    GuiScrollBarV* scroll_bar_v = 0;

    gfxm::vec2 content_offset;

    gfxm::vec2 mouse_pos;
    bool hovered = false;

    gfxm::vec2 updateContentLayout() {
        gfxm::rect current_content_rc = gfxm::rect(
            rc_content.min - content_offset,
            rc_content.max - content_offset
        );
        float total_content_height = .0f;
        float total_content_width = .0f;
        for (int i = RESERVED_CHILD_COUNT; i < children.size(); ++i) {
            children[i]->onLayout(current_content_rc);
            auto& r = children[i]->getBoundingRect();
            current_content_rc.min.y = r.max.y;

            total_content_width = gfxm::_max(total_content_width, r.max.x - r.min.x);
            total_content_height += r.max.y - r.min.y;
        }
        gfxm::vec2 content_size(
            total_content_width,
            total_content_height
        );
        return content_size;
    }
public:
    GuiWindow(Font* fnt);
    ~GuiWindow();

    GUI_HIT hitTest(int x, int y) const override {
        if (gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GUI_HIT::CLIENT;
        } else if (gfxm::point_in_rect(rc_header, gfxm::vec2(x, y))) {
            return GUI_HIT::CAPTION;
        } else if(gfxm::point_in_rect(bounding_rect, gfxm::vec2(x, y))) {
            return GUI_HIT::BOUNDING_RECT;
        } else {
            return GUI_HIT::NOWHERE;
        }
    }

    void onMessage(GUI_MSG msg, uint32_t a_param, uint32_t b_param) override {
        switch (msg) {
        case GUI_MSG::MOUSE_MOVE:
            mouse_pos = gfxm::vec2(a_param, b_param);

            break;
        case GUI_MSG::MOUSE_ENTER:
            hovered = true;
            break;
        case GUI_MSG::MOUSE_LEAVE:
            hovered = false;
            break;
        case GUI_MSG::SB_THUMB_TRACK:
            content_offset.y = a_param;
            break;
        }

        GuiElement::onMessage(msg, a_param, b_param);
    }

    void onLayout(const gfxm::rect& rc) override {
        this->bounding_rect = gfxm::rect(
            pos,
            pos + size
        );
        this->client_area = bounding_rect;

        rc_window = gfxm::rect(rc.min + pos, rc.min + pos + size);
        rc_header = gfxm::rect(rc_window.min, rc_window.min + gfxm::vec2(size.x, gfxm::_min(size.y, 30.0f)));
        rc_body = gfxm::rect(rc.min + pos + gfxm::vec2(.0f, rc_header.max.y - rc_header.min.y), rc.min + pos + size);
        rc_client = gfxm::rect(rc_body.min + gfxm::vec2(GUI_PADDING, GUI_PADDING), rc_body.max - gfxm::vec2(GUI_PADDING, GUI_PADDING));
        rc_content = rc_client;

        children[CHILD_TITLE_BAR_ID]->onLayout(rc_window);
        children[CHILD_SCROLLBAR_V]->onLayout(rc_content);

        gfxm::vec2 content_size = updateContentLayout();
        if (content_size.y > rc_content.max.y - rc_content.min.y) {
            scroll_bar_v->setEnabled(true);
            rc_content.max.x -= 10.0f;
            content_size = updateContentLayout();
            scroll_bar_v->setScrollData((rc_content.max.y - rc_content.min.y), content_size.y);
        } else {
            scroll_bar_v->setEnabled(false);
        }
    }

    void onDraw() override {        
        
        children[CHILD_TITLE_BAR_ID]->onDraw();

        guiDrawRect(rc_body, GUI_COL_BG);
        
        /*
        gfxm::rect sb_hori_rect(
            gfxm::vec2(rc_client.min.x, rc_client.max.y - 10.0f),
            gfxm::vec2(rc_client.max.x - 10.0f, rc_client.max.y)
        );
        guiDrawRect(sb_hori_rect, GUI_COL_HEADER);
        */

        children[CHILD_SCROLLBAR_V]->onDraw();

        int sw = 0, sh = 0;
        platformGetWindowSize(sw, sh);
        
        for (int i = RESERVED_CHILD_COUNT; i < children.size(); ++i) {
            glScissor(
                rc_content.min.x,
                sh - rc_content.max.y,
                rc_content.max.x - rc_content.min.x,
                rc_content.max.y - rc_content.min.y
            );
            children[i]->onDraw();
        }

        
    }
};
