#include "gui_text_buffer.hpp"

#include "gui_draw.hpp"


gfxm::rect GuiTextBuffer::calcTextRect(const gfxm::rect& rc_container, GUI_ALIGNMENT align, int char_offset) {
    gfxm::vec2 container_sz(rc_container.max.x - rc_container.min.x, rc_container.max.y - rc_container.min.y);
    gfxm::vec2 contained_sz = getBoundingSize();
    gfxm::vec2 pos;
    if ((align & GUI_HCENTER) == GUI_HCENTER) {
        pos.x = rc_container.min.x + container_sz.x * .5f - contained_sz.x * .5f;
    }
    else if ((align & GUI_LEFT) == GUI_LEFT) {
        pos.x = rc_container.min.x;
    }
    else if ((align & GUI_RIGHT) == GUI_RIGHT) {
        pos.x = rc_container.max.x - contained_sz.x;
    }
    if ((align & GUI_VCENTER) == GUI_VCENTER) {
        pos.y = rc_container.min.y + container_sz.y * .5f - contained_sz.y * .5f;
    }
    else if ((align & GUI_TOP) == GUI_TOP) {
        pos.y = rc_container.min.y;
    }
    else if ((align & GUI_BOTTOM) == GUI_TOP) {
        pos.y = rc_container.max.y - contained_sz.y;
    }

    return gfxm::rect(pos, pos + contained_sz);
}

void GuiTextBuffer::draw(const gfxm::vec2& pos, uint32_t col, uint32_t selection_col) {
    if (isDirty()) {
        prepareDraw(guiGetCurrentFont(), selection_col != 0);
    }

    int screen_w = 0, screen_h = 0;
    platformGetWindowSize(screen_w, screen_h);

    gfxm::ivec2 pos_{ (int)pos.x, (int)pos.y };

    // TODO

    // selection rectangles
    if(verts_selection.size() > 0) {
        guiDrawTextHighlight(
            (gfxm::vec3*)verts_selection.data(),
            verts_selection.size() / 3,
            indices_selection.data(), indices_selection.size(),
            selection_col
        ).model_transform = gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(pos_.x, screen_h - pos_.y, .0f));
    }

    if (vertices.size() > 0) {        
        _guiDrawText(
            (gfxm::vec3*)vertices.data(),
            (gfxm::vec2*)uv.data(),
            colors.data(),
            uv_lookup.data(),
            vertices.size() / 3,
            indices.data(), indices.size(),
            col,
            font->atlas->getId(),
            font->lut->getId(),
            font->lut->getWidth()
        ).model_transform = gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(pos_.x, pos_.y, .0f));
    }
}

void GuiTextBuffer::draw(const gfxm::rect& rc, GUI_ALIGNMENT align, uint32_t col, uint32_t selection_col) {
    if (isDirty()) {
        prepareDraw(guiGetCurrentFont(), selection_col != 0);
    }

    gfxm::rect rc_text = calcTextRect(rc, align, 0);

    draw(rc_text.min, col, selection_col);
}