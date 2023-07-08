#pragma once

#include "gui/elements/gui_window.hpp"


class GuiImportWindow : public GuiWindow {
public:
    GuiImportWindow(const char* title)
        : GuiWindow(title) {}
    virtual ~GuiImportWindow() {}

    virtual bool createImport(const std::string& spath) = 0;
    virtual bool loadImport(const std::string& spath) = 0;
};
