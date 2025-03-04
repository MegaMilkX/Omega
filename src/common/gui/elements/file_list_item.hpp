#pragma once

#include "filesystem/filesystem.hpp"
#include "gui/gui_system.hpp"
#include "gui/elements/element.hpp"
#include "gui/elements/text_element.hpp"
#include "gui/elements/file_thumbnail.hpp"


class GuiFileListItem : public GuiElement {
    std::string item_name;
    //GuiTextBuffer caption;
    //GuiTextElement* head_text = 0;
    bool is_selected = false;
    //const guiFileThumbnail* thumb = 0;

    // TODO: Figure out ownership
    std::unique_ptr<GuiFileThumbnail> thumb;
public:
    bool is_directory = false;
    std::string path_canonical;

    GuiFileListItem(const char* cap = "FileListItem", const guiFileThumbnail* thumb = 0)
    : item_name(cap), thumb(new GuiFileThumbnail(thumb)) {
        setSize(74 * 1.25, 96 * 1.25);
        //setMinSize(74 * 1.25, 96 * 1.25);
        //setMaxSize(74 * 1.25, 96 * 1.25f * 2.f);
        addFlags(GUI_FLAG_SAME_LINE);
        setStyleClasses({ "file-item" });
        overflow = GUI_OVERFLOW_FIT;
        //caption.replaceAll(getFont(), cap, strlen(cap));
        //head_text = new GuiTextElement;
        //head_text->setContent(cap);
        //head_text->addFlags(GUI_FLAG_SAME_LINE);
        //pushBack(head_text);
        this->thumb->setStyleClasses({ "file-thumbnail" });
        pushBack(this->thumb.get());
        auto caption_elem = new GuiTextElement(cap);
        caption_elem->setReadOnly(true);
        pushBack(caption_elem);
    }
    ~GuiFileListItem() {
        // TODO: 28.01.2024 Right now ref_count is only increased
        // Should decrease, but not delete
        // Then, when new thumbs are created - check if we're over some limit
        // and try to release those with ref_count == 0 to free up resources
        //guiFileThumbnailRelease(thumb);
    }

    const std::string& getName() const {
        return item_name;
    }

    void setSelected(bool is_selected) {
        this->is_selected = is_selected;
    }

    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::LCLICK:
            notifyOwner<GuiFileListItem*>(GUI_NOTIFY::FILE_ITEM_CLICK, this);
            return true;
        case GUI_MSG::DBL_LCLICK:
            notifyOwner<GuiFileListItem*>(GUI_NOTIFY::FILE_ITEM_DOUBLE_CLICK, this);
            return true;
        case GUI_MSG::PULL_START: {
            auto rel_path = fsMakeRelativePath(fsGetCurrentDirectory(), path_canonical);
            guiDragStartFile(rel_path.c_str(), this);
            return true;
        }
        case GUI_MSG::PULL_STOP:
            guiDragStop();
            return true;
        }
        return false;
    }
    void onLayout(const gfxm::vec2& extents, uint64_t flags) override {/*
        Font* font = getFont();
        const float h = font->getLineHeight();
        gfxm::vec2 px_size = gui_to_px(size, font, gfxm::rect_size(rc));
        rc_bounds = gfxm::rect(rc.min, rc.min + px_size);
        client_area = rc_bounds;*/

        //caption.setMaxLineWidth(font, 74 - 10);
        //caption.prepareDraw(font, false);
        //head_text->layout(rc_bounds, flags);
        GuiElement::onLayout(extents, flags);
    }
    void onDraw() override {
        if (isHovered()) {
            if (is_selected) {
                guiDrawRect(rc_bounds, GUI_COL_ACCENT);
            } else {
                guiDrawRect(rc_bounds, GUI_COL_BUTTON);
            }
        } else if(is_selected) {
            guiDrawRect(rc_bounds, GUI_COL_ACCENT_DIM);
        }
        /*
        gfxm::rect rc_img = client_area;
        rc_img.min += gfxm::vec2(10.f, 5.f);
        rc_img.max -= gfxm::vec2(10.f, 5.f);
        rc_img.max.y = rc_img.min.y + (rc_img.max.x - rc_img.min.x);
        if (thumb) {
            guiFileThumbnailDraw(rc_img, thumb);
        } else {
            guiDrawRect(rc_img, GUI_COL_BUTTON_HOVER);
        }
        //caption.draw(getFont(), gfxm::vec2(rc_img.min.x - 5.f, rc_img.max.y + GUI_MARGIN), GUI_COL_TEXT, GUI_COL_ACCENT);
        for (auto ch : children) {
            ch->draw();
        }
        */
        GuiElement::onDraw();
    }
};
