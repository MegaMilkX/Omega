#pragma once

#include "gui/lib/nativefiledialog/nfd.h"
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

    void browse() {
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
    }
public:
    GuiInputFilePath(
        const char* caption = "InputFilePath",
        std::string* output = 0,
        GUI_INPUT_FILE_TYPE type = GUI_INPUT_FILE_READ,
        const char* filter = "",
        const char* root_dir = ""
    ) : output(output), type(type), filter(filter), root_dir(root_dir) {
        setSize(gui::fill(), gui::content());
        setStyleClasses({ "control", "container" });
        primary_axis = GUI_PRIMARY_AXIS::X;

        GuiTextElement* label = pushBack(new GuiTextElement(caption));
        label->setReadOnly(true);
        label->setSize(gui::perc(25), gui::em(1.70));
        label->setStyleClasses({"label"});

        box = pushBack(new GuiInputStringBox());
        box->setSize(gui::fill(), gui::em(1.70));
        box->addFlags(GUI_FLAG_SAME_LINE);
        if (output) {
            //setPath(*output);
            box->setValue(*output);
        }

        btn_browse = new GuiButton("", guiLoadIcon("svg/entypo/folder.svg"));
        btn_browse->addFlags(GUI_FLAG_SAME_LINE);
        btn_browse->subscribe<GuiEvt_LClick>([this](const GuiEvt_LClick& e) {
            browse();
        });
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
        setStyleClasses({ "control", "container" });
        primary_axis = GUI_PRIMARY_AXIS::X;

        GuiTextElement* label = pushBack(new GuiTextElement(caption));
        label->setReadOnly(true);
        label->setSize(gui::perc(25), gui::em(1.70));
        label->setStyleClasses({"label"});

        box = pushBack(new GuiInputStringBox());
        box->setSize(gui::fill(), gui::em(1.70));
        box->addFlags(GUI_FLAG_SAME_LINE);
        if (get_cb) {
            //setPath(get_cb());
            box->setValue(get_cb());
        }

        btn_browse = new GuiButton("", guiLoadIcon("svg/entypo/folder.svg"));
        btn_browse->addFlags(GUI_FLAG_SAME_LINE);
        btn_browse->subscribe<GuiEvt_LClick>([this](const GuiEvt_LClick& e) {
            browse();
        });
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
};