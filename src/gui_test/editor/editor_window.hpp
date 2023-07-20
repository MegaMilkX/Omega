#pragma once

#include "gui/elements/window.hpp"


class GuiEditorWindow : public GuiWindow {
    std::string file_path;
public:

    GuiEditorWindow(const char* title);
    ~GuiEditorWindow();

    const std::string& getFilePath() const {
        return file_path;
    }
    virtual bool loadFile(const std::string& spath) {
        file_path = spath;
        return true;
    }
};

void editorRegisterEditorWindow(const std::string& file_name, GuiEditorWindow* window);
GuiEditorWindow* editorFindEditorWindow(const std::string& file_name);
void editorUnregisterEditorWindow(const std::string& file_name);