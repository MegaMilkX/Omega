#pragma once

#include "types.hpp"
#include "gui_font.hpp"

class GUI_BOX {
    friend void guiLayoutBox(GUI_BOX* box, const gfxm::vec2& size);
    friend void guiLayoutPlaceBox(GUI_BOX* box, const gfxm::vec2& px_pos);
    friend void guiDbgDrawLayoutBox(GUI_BOX* box);

    GUI_BOX* parent = 0;
    void* user_ptr = 0;
    std::vector<GUI_BOX*> children;

    int         zorder = 0;
    GuiFont*    font = 0;
    gui_float   line_height = gui::em(1);
    gui_flag_t  flags = 0;
    gui_vec2    position = gui_vec2(gui::px(100), gui::px(100));
    gui_vec2    size = gui_vec2(gui::px(0), gui::px(100));
    gui_vec2    min_size = gui_vec2(gui::px(0), gui::px(0));
    gui_vec2    max_size = gui_vec2(gui::px(FLT_MAX), gui::px(FLT_MAX));
    gui_rect    margin;
    gui_rect    padding = gui_rect(10.0f, 10.0f, 10.0f, 10.0f);
    bool        is_hidden = false;
    std::string innerText;
public:
    //
    gfxm::rect rc;
    gfxm::rect rc_content;
    uint32_t dbg_color = 0xFFFFFFFF;

    GUI_BOX() {
        dbg_color = gfxm::make_rgba32((rand() % 100) * 0.01f, (rand() % 100) * 0.01f, (rand() % 100) * 0.01f, 0.85f);
    }
    ~GUI_BOX() {
        if (parent) {
            parent->children.erase(std::find(parent->children.begin(), parent->children.end(), this));
        }
        for (int i = 0; i < children.size(); ++i) {
            auto& ch = children[i];
            ch->parent = parent;
        }
        children.clear();
    }

    void        setParent(GUI_BOX* p) {
        if (parent) {
            parent->children.erase(
                std::find(parent->children.begin(), parent->children.end(), this)
            );
        }
        parent = p;
        if (parent) {
            parent->children.push_back(this);
        }
    }
    void        addChild(GUI_BOX* ch) {
        if (ch->parent) {
            ch->parent->children.erase(
                std::find(ch->parent->children.begin(), ch->parent->children.end(), ch)
            );
        }
        children.push_back(ch);
        ch->parent = this;
    }

    void        setZOrder(int z) { zorder = z; }
    int         getZOrder() const { return zorder; }

    void        setFont(GuiFont* font) { this->font = font; }
    GuiFont*    getFont() const { return font; }

    bool        hasFlags(gui_flag_t f) const { return (flags & f) == f; }
    gui_flag_t  checkFlags(gui_flag_t f) const { return flags & f; }
    gui_flag_t  getFlags() const { return flags; }
    void        setFlags(gui_flag_t f) { flags = f; }
    void        addFlags(gui_flag_t f) { flags = flags | f; }
    void        removeFlags(gui_flag_t f) { flags = (flags & ~f); }

    void setPosition(gui_float x, gui_float y) { position = gui_vec2(x, y); }
    void setPosition(gui_vec2 p) { position = p; }
    void setWidth(gui_float w) { size.x = w; }
    void setHeight(gui_float h) { size.y = h; }
    void setSize(gui_float w, gui_float h) { size = gui_vec2(w, h); }
    void setSize(gui_vec2 sz) { size = sz; }
    void setMinSize(gui_float w, gui_float h) { min_size = gui_vec2(w, h); }
    void setMaxSize(gui_float w, gui_float h) { max_size = gui_vec2(w, h); }
    void setMargin(gui_float left, gui_float top, gui_float right, gui_float bottom) { margin = gui_rect(left, top, right, bottom); }
    void setPadding(gui_float left, gui_float top, gui_float right, gui_float bottom) { padding = gui_rect(left, top, right, bottom); }
    void setHidden(bool value) { is_hidden = value; }
    bool isHidden() const { return is_hidden; }

    void setInnerText(const std::string& text) { innerText = text; }
    const std::string& getInnerText() const { return innerText; }
};


inline void guiLayoutBox(GUI_BOX* box, const gfxm::vec2& px_parent_size) {
    GuiFont* fnt = guiGetCurrentFont();
    
    gfxm::vec2 px_size = gui_to_px(box->size, fnt, px_parent_size);
    if (px_size.x == .0f) {
        px_size.x = px_parent_size.x;
    }
    if (px_size.y == .0f) {
        px_size.y = px_parent_size.y;
    }
    gfxm::rect px_padding = gfxm::rect(
        gui_to_px(box->padding.min, fnt, px_parent_size),
        gui_to_px(box->padding.max, fnt, px_parent_size)
    );
    gfxm::vec2 px_content_size = px_size;
    px_content_size.x -= px_padding.min.x + px_padding.max.x;
    px_content_size.y -= px_padding.min.y + px_padding.max.y;

    std::sort(box->children.begin(), box->children.end(), [](const GUI_BOX* a, const GUI_BOX* b)->bool {
        if (a->checkFlags(GUI_FLAG_FRAME | GUI_FLAG_FLOATING | GUI_FLAG_TOPMOST | GUI_FLAG_BLOCKING)
            == b->checkFlags(GUI_FLAG_FRAME | GUI_FLAG_FLOATING | GUI_FLAG_TOPMOST | GUI_FLAG_BLOCKING)
        ) {
            return a->getZOrder() < b->getZOrder();
        } else if(a->checkFlags(GUI_FLAG_FRAME | GUI_FLAG_FLOATING | GUI_FLAG_TOPMOST)
            == b->checkFlags(GUI_FLAG_FRAME | GUI_FLAG_FLOATING | GUI_FLAG_TOPMOST)
        ) {
            return a->checkFlags(GUI_FLAG_BLOCKING) < b->checkFlags(GUI_FLAG_BLOCKING);
        } else if(a->checkFlags(GUI_FLAG_FRAME | GUI_FLAG_FLOATING) == b->checkFlags(GUI_FLAG_FRAME | GUI_FLAG_FLOATING)) {
            return a->checkFlags(GUI_FLAG_TOPMOST) < b->checkFlags(GUI_FLAG_TOPMOST);
        } else {
            if (a->hasFlags(GUI_FLAG_FRAME)) {
                return true;
            }
            if (b->hasFlags(GUI_FLAG_FLOATING)) {
                return true;
            }
        }
        return false;
    });

    float x = px_padding.min.x;
    float y = px_padding.min.y;

    // Frame
    int i = 0;
    for (; i < box->children.size(); ++i) {
        auto& ch = box->children[i];
        if (!ch->hasFlags(GUI_FLAG_FRAME)) {
            break;
        }
        if (ch->isHidden()) {
            continue;
        }
        guiLayoutBox(ch, px_content_size);
        px_content_size.y -= ch->rc.max.y;
        y += ch->rc.max.y;
    }

    gfxm::vec2 px_content_origin(x, y);
    float inline_x = x;
    float inline_y = y;
    // Content
    for (; i < box->children.size(); ++i) {
        auto& ch = box->children[i];
        if (ch->hasFlags(GUI_FLAG_FLOATING)) {
            break;
        }
        guiLayoutBox(ch, px_content_size);
        if (ch->hasFlags(GUI_FLAG_SAME_LINE)) {
            ch->rc.min = gfxm::vec2(inline_x, inline_y);
        } else {
            ch->rc.min = gfxm::vec2(x, y);
        }
        inline_x = x + ch->rc.max.x;
        inline_y = y;
        y += ch->rc.max.y;
    }

    // Floating
    for (; i < box->children.size(); ++i) {
        auto& ch = box->children[i];
        if (ch->isHidden()) {
            continue;
        }
        guiLayoutBox(ch, px_content_size);
        ch->rc.min = gui_to_px(ch->position, guiGetCurrentFont(), px_content_size);
    }

    gfxm::rect rc_content;
    bool box_initialized = false;
    if (!box->getInnerText().empty()) {
        const std::string& text = box->getInnerText();
        rc_content.max.x = px_parent_size.x;
        rc_content.max.y = guiGetCurrentFont()->font->getLineHeight();
        box_initialized = true;
    }
    for (int i = 0; i < box->children.size(); ++i) {
        auto& ch = box->children[i];
        if (ch->isHidden()) {
            continue;
        }
        gfxm::rect rc = gfxm::rect(
            ch->rc.min, ch->rc.min + ch->rc.max
        );
        if (!box_initialized) {
            rc_content = rc;
            box_initialized = true;
        } else {
            gfxm::expand(rc_content, rc);
        }
    }
    if (box_initialized) {
        if (px_size.x < .0f) {
            px_size.x = rc_content.max.x;
        }
        if (px_size.y < .0f) {
            px_size.y = rc_content.max.y;
        }
    }

    box->rc.min = gfxm::vec2(0, 0);
    box->rc.max = px_size;
}
inline void guiLayoutPlaceBox(GUI_BOX* box, const gfxm::vec2& px_pos) {
    box->rc.min = px_pos;
    box->rc.max = px_pos + box->rc.max;

    for (int i = 0; i < box->children.size(); ++i) {
        auto& ch = box->children[i];
        guiLayoutPlaceBox(ch, px_pos + ch->rc.min);
    }
}
inline void guiDbgDrawLayoutBox(GUI_BOX* box) {
    guiDrawRectLine(box->rc, box->dbg_color);
    if (!box->getInnerText().empty()) {
        guiDrawText(box->rc, box->getInnerText().c_str(), guiGetCurrentFont(), GUI_LEFT | GUI_TOP, GUI_COL_TEXT);
    }

    for (int i = 0; i < box->children.size(); ++i) {
        auto& ch = box->children[i];
        guiDbgDrawLayoutBox(ch);
    }
}