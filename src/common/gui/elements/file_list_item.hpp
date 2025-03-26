#pragma once

#include "filesystem/filesystem.hpp"
#include "gui/gui_system.hpp"
#include "gui/elements/element.hpp"
#include "gui/elements/text_element.hpp"
#include "gui/elements/file_thumbnail.hpp"


class GuiFileListItem : public GuiElement {
    std::string item_name;
    bool is_selected = false;

    // TODO: Figure out ownership
    std::unique_ptr<GuiFileThumbnail> thumb;
public:
    bool is_directory = false;
    std::string path_canonical;

    GuiFileListItem(const char* cap = "FileListItem", const guiFileThumbnail* thumb = 0)
    : item_name(cap), thumb(new GuiFileThumbnail(thumb)) {
        setSize(74 * 1.25, 96 * 1.25);
        addFlags(GUI_FLAG_SAME_LINE);
        setStyleClasses({ "file-item" });

        this->thumb->setStyleClasses({ "file-thumbnail" });
        this->thumb->addFlags(GUI_FLAG_NO_HIT);
        pushBack(this->thumb.get());
        auto caption_elem = new GuiTextElement(cap);
        caption_elem->setReadOnly(true);
        caption_elem->addFlags(GUI_FLAG_NO_HIT);
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
    void onDraw() override {
        if (isHovered()) {
            if (is_selected) {
                guiDrawRect(rc_bounds, GUI_COL_ACCENT);
                guiDrawRectLine(rc_bounds, GUI_COL_ACCENT2);
            } else {
                guiDrawRect(rc_bounds, GUI_COL_BUTTON);
            }
        } else if(is_selected) {
            guiDrawRect(rc_bounds, GUI_COL_ACCENT_DIM);
            guiDrawRectLine(rc_bounds, GUI_COL_ACCENT2);
        }
        GuiElement::onDraw();
    }
};
