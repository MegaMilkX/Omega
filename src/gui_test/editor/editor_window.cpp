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

GuiEditorWindow::GuiEditorWindow(const char* title, const char* file_ext)
    : GuiWindow(title), file_extension(file_ext) {
    
    auto keydown_hdl = getHandler<GuiEvt_KeyDown>();
    subscribe<GuiEvt_KeyDown>([this, keydown_hdl](const GuiEvt_KeyDown& e) {
        if (guiIsModifierKeyPressed(GUI_KEY_CONTROL) && e.vkey == 0x53) { // CTRL + S
            onSave();
            return;
        }
        if (!keydown_hdl.invoke(e)) {
            e.consume = false;
        }
    });
}
GuiEditorWindow::~GuiEditorWindow() {
    editorUnregisterEditorWindow(file_path);
}