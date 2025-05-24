#pragma once

#include "gui/lib/nativefiledialog/nfd.h"
#include "gui/elements/label.hpp"
#include "gui/elements/input_text_line.hpp"
#include "gui/elements/input.hpp"
#include "gui/elements/button.hpp"
#include "filesystem/filesystem.hpp"


enum GUI_INPUT_FILE_TYPE {
    GUI_INPUT_FILE_READ,
    GUI_INPUT_FILE_WRITE
};
class GuiInputFilePath : public GuiElement {
    GUI_INPUT_FILE_TYPE type;

    GuiInputStringBox* box = 0;
    GuiButton* btn_browse = 0;

    std::string filter;
    std::string root_dir;
    std::string* output = 0;
    std::function<void(const std::string&)> set_path_cb = nullptr;
    std::function<std::string(void)>        get_path_cb = nullptr;
public:
    GuiInputFilePath(
        const char* caption = "InputFilePath",
        std::string* output = 0,
        GUI_INPUT_FILE_TYPE type = GUI_INPUT_FILE_READ,
        const char* filter = "",
        const char* root_dir = ""
    ) : output(output), type(type), filter(filter), root_dir(root_dir) {
        setSize(gui::fill(), gui::content());
        setStyleClasses({ "control" });

        GuiTextElement* label = pushBack(new GuiTextElement(caption));
        label->setReadOnly(true);
        label->setSize(gui::perc(25), gui::em(2));
        label->setStyleClasses({"label"});

        box = pushBack(new GuiInputStringBox());
        box->setSize(gui::fill(), gui::em(2));
        box->addFlags(GUI_FLAG_SAME_LINE);
        if (output) {
            //setPath(*output);
            box->setValue(*output);
        }

        btn_browse = new GuiButton("", guiLoadIcon("svg/entypo/folder.svg"));
        btn_browse->addFlags(GUI_FLAG_SAME_LINE);
        pushBack(btn_browse);
    }
    GuiInputFilePath(
        const char* caption,
        std::function<void(const std::string&)> set_cb,
        std::function<std::string(void)> get_cb,
        GUI_INPUT_FILE_TYPE type = GUI_INPUT_FILE_READ,
        const char* filter = "",
        const char* root_dir = ""
    ) : output(0), set_path_cb(set_cb), get_path_cb(get_cb), type(type), filter(filter), root_dir(root_dir)  {
        setSize(gui::fill(), gui::content());
        setStyleClasses({ "control" });

        GuiTextElement* label = pushBack(new GuiTextElement(caption));
        label->setReadOnly(true);
        label->setSize(gui::perc(25), gui::em(2));
        label->setStyleClasses({"label"});

        box = pushBack(new GuiInputStringBox());
        box->setSize(gui::fill(), gui::em(2));
        box->addFlags(GUI_FLAG_SAME_LINE);
        if (get_cb) {
            //setPath(get_cb());
            box->setValue(get_cb());
        }

        btn_browse = new GuiButton("", guiLoadIcon("svg/entypo/folder.svg"));
        btn_browse->addFlags(GUI_FLAG_SAME_LINE);
        pushBack(btn_browse);
    }

    void setPath(const std::string& path_) {
        std::string path = path_;
        if (!root_dir.empty()) {
            path = fsMakeRelativePath(root_dir, path);
        }
        if (output) {
            *output = path;
            box->setContent((*output).c_str());
        }
        if (set_path_cb) {
            set_path_cb(path);
        }
        if (get_path_cb) {
            box->setContent(get_path_cb().c_str());
        }
    }

    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::NOTIFY:
            switch (params.getA<GUI_NOTIFY>()) {
            case GUI_NOTIFY::BUTTON_CLICKED: {
                nfdchar_t* out_path = 0;
                nfdresult_t result = NFD_ERROR;
                if (type == GUI_INPUT_FILE_WRITE) {
                    result = NFD_SaveDialog(filter.c_str(), 0, &out_path);
                } else if(type == GUI_INPUT_FILE_READ) {
                    result = NFD_OpenDialog(filter.c_str(), 0, &out_path);
                }
                if (result == NFD_OKAY) {
                    setPath(out_path);
                } else if(result == NFD_CANCEL) {
                    // User cancelled
                } else {
                    LOG_ERR("Browse dialog error: " << result);
                }
                return true;
            }
            }
            break;
        }
        return false;
    }
    /*
    void onLayout(const gfxm::vec2& extents, uint64_t flags) override {
        Font* font = getFont();

        const float text_box_height = font->getLineHeight() * 2.0f;
        setHeight(text_box_height);
        rc_bounds = gfxm::rect(
            gfxm::vec2(0, 0),
            gfxm::vec2(extents.x, text_box_height)
        );
        client_area = rc_bounds;

        gfxm::rect rc_left, rc_right;
        rc_left = client_area;
        guiLayoutSplitRect2XRatio(rc_left, rc_right, .25f);
        gfxm::rect rc_path;
        guiLayoutSplitRect2X(rc_right, rc_path, (rc_right.max.y - rc_right.min.y));

        label.layout_position = rc_left.min;
        label.layout(gfxm::rect_size(rc_left), flags);
        btn_browse.layout_position = rc_right.min;
        btn_browse.layout(gfxm::rect_size(rc_right), flags);
        rc_path.min.x = btn_browse.getClientArea().max.x;
        box.layout_position = rc_path.min;
        box.layout(gfxm::rect_size(rc_path), flags);
    }
    void onDraw() override {
        label.draw();
        box.draw();
        btn_browse.draw();
    }*/
};