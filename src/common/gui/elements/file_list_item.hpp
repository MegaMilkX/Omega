#pragma once

#include "filesystem/filesystem.hpp"
#include "gui/gui_system.hpp"
#include "gui/elements/element.hpp"
#include "gui/elements/text_element.hpp"
#include "gui/filesystem/gui_file_thumbnail.hpp"


class GuiFileListItem : public GuiElement {
    std::string item_name;
    //GuiTextBuffer caption;
    GuiTextElement* head_text = 0;
    bool is_selected = false;
    const guiFileThumbnail* thumb = 0;
public:
    bool is_directory = false;
    std::string path_canonical;

    GuiFileListItem(const char* cap = "FileListItem", const guiFileThumbnail* thumb = 0)
    : item_name(cap), thumb(thumb) {
        setSize(74 * 1.25, 96 * 1.25);
        addFlags(GUI_FLAG_SAME_LINE);
        setStyleClasses({ "file-item" });
        //caption.replaceAll(getFont(), cap, strlen(cap));
        head_text = new GuiTextElement;
        head_text->setContent(cap);
        head_text->addFlags(GUI_FLAG_SAME_LINE);
        pushBack(head_text);
    }
    ~GuiFileListItem() {
        guiFileThumbnailRelease(thumb);
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
            guiDragStartFile(rel_path.c_str());
            return true;
        }
        case GUI_MSG::PULL_STOP:
            guiDragStop();
            return true;
        }
        return false;
    }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {/*
        Font* font = getFont();
        const float h = font->getLineHeight();
        gfxm::vec2 px_size = gui_to_px(size, font, gfxm::rect_size(rc));
        rc_bounds = gfxm::rect(rc.min, rc.min + px_size);
        client_area = rc_bounds;*/

        //caption.setMaxLineWidth(font, 74 - 10);
        //caption.prepareDraw(font, false);
        //head_text->layout(rc_bounds, flags);
        GuiElement::onLayout(rc, flags);
    }
    void onDraw() override {
        if (isHovered()) {
            if (is_selected) {
                guiDrawRect(client_area, GUI_COL_BUTTON_HOVER);
            } else {
                guiDrawRect(client_area, GUI_COL_BUTTON);
            }
        } else if(is_selected) {
            guiDrawRect(client_area, GUI_COL_BUTTON_HOVER);
        }
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
    }
};