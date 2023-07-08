#include "editor_window.hpp"

static std::map<std::string, GuiEditorWindow*> s_active_editors;

void editorRegisterEditorWindow(const std::string& file_name, GuiEditorWindow* window) {
    s_active_editors[file_name] = window;
}
GuiEditorWindow* editorFindEditorWindow(const std::string& file_name) {
    auto it = s_active_editors.find(file_name);
    if (it != s_active_editors.end()) {
        return it->second;
    }
    return 0;
}
void editorUnregisterEditorWindow(const std::string& file_name) {
    s_active_editors.erase(file_name);
}

GuiEditorWindow::GuiEditorWindow(const char* title)
    : GuiWindow(title) {}
GuiEditorWindow::~GuiEditorWindow() {
    editorUnregisterEditorWindow(file_path);
}