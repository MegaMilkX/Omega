#pragma once

#include <filesystem>
#include "gui/filesystem/gui_file_thumbnail.hpp"
#include "gui/elements/window.hpp"
#include "gui/elements/tree_view.hpp"
#include "gui/elements/file_container.hpp"
#include "gui/elements/input_string.hpp"
#include "gui/elements/combo_box.hpp"

enum GuiFileExplorerMode {
    GuiFileExplorerModeBrowse,
    GuiFileExplorerModeOpen,
    GuiFileExplorerModeSave
};

struct GuiFileFilter {
    std::string label;
    std::vector<std::string> extensions;
};

enum GuiFileSelectMode {
    GuiFileSelectSingle,
    GuiFileSelectMultiple
};

struct GuiFileExplorerParams {
    GuiFileExplorerMode mode = GuiFileExplorerModeBrowse;
    std::vector<GuiFileFilter> filters;      // empty = no filtering, show everything
};

class GuiFileExplorer : public GuiWindow {
public:
    enum SCOPE {
        SCOPE_ROOT, // navigation constrained to scope_root
        SCOPE_FS    // can navigate the whole file system
    };
private:
    GuiFileExplorerMode mode = GuiFileExplorerModeBrowse;
    GuiFileSelectMode select_mode = GuiFileSelectSingle;
    std::vector<GuiFileFilter> filters;
    GuiFileFilter current_filter;
    std::filesystem::path scope_root;
    SCOPE scope;

    GuiTreeView* tree_view = nullptr;
    GuiFileContainer* container = nullptr;

    std::filesystem::path current_path;
    std::filesystem::path local_path;
    std::vector<GuiFileListItem*> selected_items;

    GuiTextElement* btn_back = nullptr;
    GuiTextElement* btn_forward = nullptr;
    GuiTextElement* address_bar = nullptr;
    GuiInputString* file_input = nullptr;
    GuiComboBox*    filter_combo = nullptr;
    GuiInputString* filter_input = nullptr;
    GuiTextElement* btn_confirm = nullptr;

    std::string filter = "";
    std::string search_buffer = "";
    GuiTextElement* search_buffer_hint = nullptr;

    static constexpr int NAV_HISTORY_MAX = 32;
    std::vector<std::filesystem::path> nav_history;
    int nav_history_cur = 0;

    void selectFilter(int i);

    void historyNew(const std::filesystem::path& p);
    bool historyCanGoBack() const;
    bool historyCanGoForward() const;
    void historyBack();
    void historyForward();
    void updateHistoryButtons();

    std::optional<std::filesystem::path> resolveNavTarget(const std::filesystem::path& requested);

    void clearSelected();
    void selectOne(GuiFileListItem* item, bool append = false);
    
    void confirmOpen();
    std::optional<std::filesystem::path> resolveSaveTarget(const std::filesystem::path& requested);
    void confirmSave();
public:
    GuiFileExplorer(const GuiFileExplorerParams& params = {});
    ~GuiFileExplorer();

    void updateDirTreeItem(GuiTreeItem* item, const std::filesystem::path& path);
    void updateDirTree(const std::filesystem::path& path);

    void goUp();
    void openDir(const std::filesystem::path& path, bool history_new = true);

    void applyFilter(const std::string& filter);

    void onTick(float dt, GUI_TICK_ID id) override;
};

