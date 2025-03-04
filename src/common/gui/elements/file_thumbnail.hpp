#pragma once

#include "gui/gui_system.hpp"
#include "gui/elements/element.hpp"
#include "gui/filesystem/gui_file_thumbnail.hpp"


class GuiFileThumbnail : public GuiElement {
    const guiFileThumbnail* thumb = 0;
public:
    GuiFileThumbnail(const guiFileThumbnail* thumb)
    : thumb(thumb) {
        // TODO: Must fill horizontally
        setSize(64 * 1.25, 64 * 1.25);
    }
    ~GuiFileThumbnail() {
        // TODO: 28.01.2024 Right now ref_count is only increased
        // Should decrease, but not delete
        // Then, when new thumbs are created - check if we're over some limit
        // and try to release those with ref_count == 0 to free up resources
        //guiFileThumbnailRelease(thumb);
    }

    void onDraw() override {
        gfxm::rect rc_img = client_area;
        //rc_img.min += gfxm::vec2(10.f, 5.f);
        //rc_img.max -= gfxm::vec2(10.f, 5.f);
        //rc_img.max.y = rc_img.min.y + (rc_img.max.x - rc_img.min.x);
        if (thumb) {
            guiFileThumbnailDraw(rc_img, thumb);
        } else {
            guiDrawRect(rc_img, GUI_COL_BUTTON_HOVER);
        }
    }
};