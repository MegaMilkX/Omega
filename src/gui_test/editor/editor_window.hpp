#pragma once

#include "gui/elements/window.hpp"
#include "filesystem/filesystem.hpp"
#include "gui/lib/nativefiledialog/nfd.h"


class GuiEditorWindow : public GuiWindow {
    std::string file_path;
    std::string file_extension;

public:

    GuiEditorWindow(const char* title, const char* file_extension);
    ~GuiEditorWindow();

    virtual bool onSaveCommand(const std::string& path) {
        LOG_ERR("onSaveCommand() not implemented!");
        return false;
    }
    bool onSave() {
        if (file_path.empty()) {
            nfdchar_t* out_path = 0;
            nfdresult_t result = NFD_ERROR;
            result = NFD_SaveDialog(file_extension.c_str(), 0, &out_path);
            if (result == NFD_OKAY) {
                file_path = out_path;
                std::experimental::filesystem::path path = file_path;
                if (!path.has_extension()) {
                    path += std::string(".") + file_extension;
                }
                file_path = path.string();
            } else if(result == NFD_CANCEL) {
                return false;
            } else {
                LOG_ERR("Unexpected browse dialog error: " << result);
                return false;
            }
        }
        if (onSaveCommand(file_path)) {
            setTitle(fsMakeRelativePath(fsGetCurrentDirectory(), file_path).c_str());
            return true;
        }
        return false;
    }

    virtual bool onOpenCommand(const std::string& path) {
        LOG_ERR("onOpenCommand() not implemented");
        return false;
    }
    bool open(const std::string& path) {
        if (!onOpenCommand(path)) {
            return false;
        }
        file_path = path;
        setTitle(fsMakeRelativePath(fsGetCurrentDirectory(), file_path).c_str());
        return true;
    }

    const std::string& getFilePath() const {
        return file_path;
    }
    virtual bool loadFile(const std::string& spath) {
        file_path = spath;
        return true;
    }

    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::KEYDOWN: {
            if (guiIsModifierKeyPressed(GUI_KEY_CONTROL) && params.getA<uint16_t>() == 0x53) { // CTRL + S
                onSave();
                return true;
            }
            break;
        }
        }
        return GuiWindow::onMessage(msg, params);
    }
};

void editorRegisterEditorWindow(const std::string& file_name, GuiEditorWindow* window);
GuiEditorWindow* editorFindEditorWindow(const std::string& file_name);
void editorUnregisterEditorWindow(const std::string& file_name);