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
    GuiLabel label;
    GuiInputTextLine box;
    GuiButton btn_browse;
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
    ) : label(caption), output(output), box(output), type(type), filter(filter), root_dir(root_dir), btn_browse("", guiLoadIcon("svg/entypo/folder.svg")) {
        setSize(0, 0);
        setMaxSize(0, 0);
        setMinSize(0, 0);

        label.setOwner(this);
        label.setParent(this);
        box.setOwner(this);
        box.setParent(this);
        btn_browse.setOwner(this);
        btn_browse.setParent(this);
    }
    GuiInputFilePath(
        const char* caption,
        std::function<void(const std::string&)> set_cb,
        std::function<std::string(void)> get_cb,
        GUI_INPUT_FILE_TYPE type = GUI_INPUT_FILE_READ,
        const char* filter = "",
        const char* root_dir = ""
    ) :label(caption), output(0), set_path_cb(set_cb), get_path_cb(get_cb), box((std::string*)0), type(type), filter(filter), root_dir(root_dir), btn_browse("", guiLoadIcon("svg/entypo/folder.svg"))  {
        setSize(0, 0);
        setMaxSize(0, 0);
        setMinSize(0, 0);

        label.setOwner(this);
        label.setParent(this);
        box.setOwner(this);
        box.setParent(this);
        btn_browse.setOwner(this);
        btn_browse.setParent(this);
    }

    void setPath(const std::string& path_) {
        std::string path = path_;
        if (!root_dir.empty()) {
            path = fsMakeRelativePath(root_dir, path);
        }
        if (output) {
            *output = path;
            box.setText((*output).c_str());
        }
        if (set_path_cb) {
            set_path_cb(path);
        }
        if (get_path_cb) {
            box.setText(get_path_cb().c_str());
        }
    }

    void onHitTest(GuiHitResult& hit, int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return;
        }

        box.onHitTest(hit, x, y);
        if (hit.hasHit()) {
            return;
        }
        btn_browse.onHitTest(hit, x, y);
        if (hit.hasHit()) {
            return;
        }

        hit.add(GUI_HIT::CLIENT, this);
        return;
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
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        Font* font = guiGetCurrentFont()->font;

        const float text_box_height = font->getLineHeight() * 2.0f;
        setHeight(text_box_height);
        rc_bounds = gfxm::rect(
            rc.min,
            gfxm::vec2(rc.max.x, rc.min.y + text_box_height)
        );
        client_area = rc_bounds;

        gfxm::rect rc_left, rc_right;
        rc_left = client_area;
        guiLayoutSplitRect2XRatio(rc_left, rc_right, .25f);
        gfxm::rect rc_path;
        guiLayoutSplitRect2X(rc_right, rc_path, (rc_right.max.y - rc_right.min.y));

        label.layout(rc_left, flags);
        btn_browse.layout(rc_right, flags);
        rc_path.min.x = btn_browse.getClientArea().max.x;
        box.layout(rc_path, flags);
    }
    void onDraw() override {
        label.draw();
        box.draw();
        btn_browse.draw();
    }
};