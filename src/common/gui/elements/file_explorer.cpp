#include "file_explorer.hpp"

#include <shellapi.h>
#include <ShObjIdl.h>
#include "IconsForkAwesome.h"



// =====================================
static bool wildcardMatch(std::string_view str, std::string_view pattern) {
    size_t s = 0, p = 0;
    size_t starIdx = std::string_view::npos, matchIdx = 0;

    auto lower = [](char c) {
        return (char)std::tolower((unsigned char)c);
    };

    while (s < str.size()) {
        if (p < pattern.size() && (pattern[p] == '?' || lower(pattern[p]) == lower(str[s]))) {
            ++s; ++p;
        } else if (p < pattern.size() && pattern[p] == '*') {
            starIdx = p++;
            matchIdx = s;
        } else if (starIdx != std::string_view::npos) {
            p = starIdx + 1;
            s = ++matchIdx;
        }
        else {
            return false;
        }
    }

    while (p < pattern.size() && pattern[p] == '*') {
        ++p;
    }

    return p == pattern.size();
}

static bool matchesFilterString(std::string_view filename, std::string_view filterStr) {
    if (filterStr.empty()) {
        return true;
    }

    size_t start = 0;
    while (start <= filterStr.size()) {
        size_t sep = filterStr.find(';', start);
        std::string_view pattern = filterStr.substr(
            start,
            sep == std::string_view::npos ? sep : sep - start
        );

        while (!pattern.empty() && std::isspace((unsigned char)pattern.front())) {
            pattern.remove_prefix(1);
        }
        while (!pattern.empty() && std::isspace((unsigned char)pattern.back())) {
            pattern.remove_suffix(1);
        }

        if (!pattern.empty() && wildcardMatch(filename, pattern)) {
            return true;
        }

        if (sep == std::string_view::npos) {
            break;
        }
        start = sep + 1;
    }
    return false;
}
// ==========================================



void GuiFileExplorer::selectFilter(int i) {
    if (mode == GuiFileExplorerModeBrowse) {
        // Different filter, skip
        return;
    }
    current_filter = filters[i];
    if (current_filter.extensions.empty()) {
        filter = "*";
    } else {
        filter.clear();
        filter += std::format("*.{}", current_filter.extensions[0]);
        for (int i = 1; i < current_filter.extensions.size(); ++i) {
            filter += std::format(";*.{}", current_filter.extensions[i]);
        }
    }
    applyFilter(filter);
}

void GuiFileExplorer::historyNew(const std::filesystem::path& p) {
    if(!nav_history.empty() && nav_history[nav_history_cur] == p) return;
    if (!nav_history.empty()) {
        nav_history.resize(nav_history_cur + 1);
    }
    nav_history.push_back(p);

    if (nav_history.size() > NAV_HISTORY_MAX) {
        nav_history.erase(nav_history.begin());
    } else {
        nav_history_cur = nav_history.size() - 1;
    }
}
bool GuiFileExplorer::historyCanGoBack() const {
    return nav_history_cur > 0;
}
bool GuiFileExplorer::historyCanGoForward() const {
    return nav_history_cur + 1 < nav_history.size();
}
void GuiFileExplorer::historyBack() {
    if (!historyCanGoBack()) {
        return;
    }
    --nav_history_cur;
    openDir(nav_history[nav_history_cur], false);
}
void GuiFileExplorer::historyForward() {
    if (!historyCanGoForward()) {
        return;
    }
    ++nav_history_cur;
    openDir(nav_history[nav_history_cur], false);
}
void GuiFileExplorer::updateHistoryButtons() {
    if (historyCanGoBack()) {
        btn_back->removeFlags(GUI_FLAG_DISABLED);
    } else {
        btn_back->addFlags(GUI_FLAG_DISABLED);
    }

    if (historyCanGoForward()) {
        btn_forward->removeFlags(GUI_FLAG_DISABLED);
    } else {
        btn_forward->addFlags(GUI_FLAG_DISABLED);
    }
}

std::optional<std::filesystem::path> GuiFileExplorer::resolveNavTarget(const std::filesystem::path& requested) {
    std::error_code ec;
    auto canon = std::filesystem::weakly_canonical(requested, ec);
    if(ec) return std::nullopt;

    if (scope == SCOPE_FS) {
        return canon;
    }

    auto rel = std::filesystem::relative(canon, scope_root, ec);
    if(ec) return std::nullopt;

    auto rel_str = rel.generic_string();
    if (rel_str == "." || (rel_str.rfind("..", 0) != 0)) {
        return canon;
    }
    return std::nullopt;
}

void GuiFileExplorer::clearSelected() {
    for (auto itm : selected_items) {
        itm->setSelected(false);
    }
    if (!selected_items.empty() && file_input) {
        file_input->setValue("");
    }
    selected_items.clear();
    if (btn_confirm && mode == GuiFileExplorerModeOpen) {
        btn_confirm->setEnabled(false);
    }
}
void GuiFileExplorer::selectOne(GuiFileListItem* item, bool append) {
    if (select_mode == GuiFileSelectSingle) {
        append = false;
    }

    if (!item) {
        return;
    }
    if(!append) {
        clearSelected();
    }
    selected_items.push_back(item);
    item->setSelected(true);

    container->scrollTo(item);
    
    if(file_input) {
        std::string str_files;
        if (selected_items.size() == 1) {
            str_files = selected_items[0]->getName();
        } else {
            str_files += std::format("\"{}\"", selected_items[0]->getName());
            for (int i = 1; i < selected_items.size(); ++i) {
                str_files += std::format(" \"{}\"", selected_items[i]->getName());
            }
        }
        file_input->setValue(str_files);
    }
    if (btn_confirm && mode == GuiFileExplorerModeOpen) {
        btn_confirm->setEnabled(true);
    }
}
void GuiFileExplorer::confirmOpen() {
    LOG("Open button pressed");
    if (selected_items.empty()) {
        return;
    }
    GuiEvt_FileConfirmed e;
    for (auto item : selected_items) {
        LOG_DBG("Open: " << item->path_canonical);
        e.files.push_back(item->path_canonical);
    }
    invokeBubble(e);
}

static std::vector<std::string> parseFileInputString(const std::string& str) {
    size_t b = str.find_first_not_of(" \t");
    if(b == std::string::npos) return {};

    size_t en = str.find_last_not_of(" \t");
    std::string trimmed = str.substr(b, en - b + 1);

    if (trimmed.find('"') == std::string::npos) {
        return { trimmed }; // no quotes, single name
    }

    std::vector<std::string> result;
    size_t i = 0;
    while (i < trimmed.size()) {
        while (i < trimmed.size() && std::isspace((unsigned char)trimmed[i])) {
            ++i;
        }

        if (i >= trimmed.size()) {
            break;
        }

        std::string token;
        if (trimmed[i] == '"') {
            ++i;
            while (i < trimmed.size() && trimmed[i] != '"') {
                token.push_back(trimmed[i++]);
            }
            if (i < trimmed.size()) {
                ++i;
            }
        } else {
            while (i < trimmed.size() && !std::isspace((unsigned char)trimmed[i])) {
                token.push_back(trimmed[i++]);
            }
        }
        if (!token.empty()) {
            result.push_back(token);
        }
    }
    return result;
}

std::optional<std::filesystem::path> GuiFileExplorer::resolveSaveTarget(const std::filesystem::path& requested) {
    std::filesystem::path combined = requested.is_relative() ? current_path / requested : requested;

    std::error_code ec;
    auto canonParent = std::filesystem::weakly_canonical(combined.parent_path(), ec);
    if(ec) return std::nullopt;

    std::filesystem::path full = canonParent / combined.filename();

    if (scope == SCOPE_FS) {
        return full;
    }

    auto rel = std::filesystem::relative(canonParent, scope_root, ec);
    if(ec) return std::nullopt;

    auto relStr = rel.generic_string();
    if (relStr == "." || relStr.rfind("..", 0) != 0) {
        return full;
    }
    return std::nullopt;
}
void GuiFileExplorer::confirmSave() {
    LOG("Save button pressed");
    if (!file_input) {
        assert(false);
        return;
    }
    std::string str_files = file_input->getValue();
    if (str_files.empty()) {
        return;
    }
    auto names = parseFileInputString(str_files);
    if (names.empty()) {
        assert(false);
        return;
    }

    std::vector<std::filesystem::path> resolved_paths;
    resolved_paths.reserve(names.size());
    for (auto& name : names) {
        // TODO: if no extension, append the first filter
        auto resolved = resolveSaveTarget(name);
        if (!resolved) {
            LOG_ERR("Invalid save path: " << name);
            return;
        }
        resolved_paths.push_back(resolved.value());
    }

    if(!current_filter.extensions.empty()) {
        for (auto& p : resolved_paths) {
            if (p.has_extension()) {
                continue;
            }
            p += "." + current_filter.extensions[0];
        }
    }

    std::vector<std::filesystem::path> existing;
    for (auto& p : resolved_paths) {
        std::error_code ec;
        if (std::filesystem::exists(p, ec)) {
            existing.push_back(p);
        }
    }
    if (!existing.empty()) {
        // TODO: Overwrite confirmation needed
        MessageBoxA(NULL, "Overwrite existing files?", "Overwrite warning", MB_ICONWARNING | MB_OKCANCEL);
    }

    GuiEvt_FileConfirmed e;
    for (auto& p : resolved_paths) {
        LOG_DBG("Save: " << p.generic_string());
        e.files.push_back(p.generic_string());
    }
    invokeBubble(e);
}

static std::filesystem::path cached_current_dir;

GuiFileExplorer::GuiFileExplorer(const GuiFileExplorerParams& params)
    : GuiWindow("FileExplorer")
    , mode(params.mode)
    , filters(params.filters) {
    setSize(800, 600);
    std::string sfname;
    sfname.resize(MAX_PATH);
    GetFullPathName(".", MAX_PATH, &sfname[0], 0);
    current_path = sfname;

    // =======================
    scope = SCOPE_ROOT;
    scope_root = current_path;
    // =======================

    {
        auto toolbar = pushBack(guiCreate<GuiElement>());
        toolbar->primary_axis = GUI_PRIMARY_AXIS::X;
        toolbar->setSize(gui::fill(), gui::content());
        toolbar->setStyleClasses({ "control", "container" });
            
        btn_back = toolbar->pushBack(guiCreate<GuiTextElement>());
        btn_back->setStyleClasses({ "button", "icon" });
        btn_back->setContent(ICON_FK_CHEVRON_LEFT);
        btn_back->setSize(gui::em(1.70), gui::em(1.70));
        btn_back->addFlags(GUI_FLAG_DISABLED);
        btn_back->subscribe<GuiEvt_LClick>([this](const GuiEvt_LClick& e) {
            historyBack();
        });

        btn_forward = toolbar->pushBack(guiCreate<GuiTextElement>());
        btn_forward->setStyleClasses({ "button", "icon" });
        btn_forward->setContent(ICON_FK_CHEVRON_RIGHT);
        btn_forward->setSize(gui::em(1.70), gui::em(1.70));
        btn_forward->addFlags(GUI_FLAG_DISABLED);
        btn_forward->subscribe<GuiEvt_LClick>([this](const GuiEvt_LClick& e) {
            historyForward();
        });

        auto btn_up = toolbar->pushBack(guiCreate<GuiTextElement>());
        btn_up->setStyleClasses({ "button", "icon" });
        btn_up->setContent(ICON_FK_ARROW_UP);
        btn_up->setSize(gui::em(1.70), gui::em(1.70));
        btn_up->subscribe<GuiEvt_LClick>([this](const GuiEvt_LClick& e) {
            goUp();
        });

        address_bar = toolbar->pushBack(guiCreate<GuiTextElement>());
        address_bar->setStyleClasses({ "input-box", "input-box-editable", "input-box-string" });
        address_bar->setSize(gui::fill(), gui::fill()); // fill height, relies on buttons holding it up

        auto btn_refresh = toolbar->pushBack(guiCreate<GuiTextElement>());
        btn_refresh->setStyleClasses({ "button", "icon" });
        btn_refresh->setContent(ICON_FK_REFRESH);
        btn_refresh->setSize(gui::em(1.70), gui::em(1.70));
        btn_refresh->subscribe<GuiEvt_LClick>([this](const GuiEvt_LClick& e) {
            openDir(current_path);
        });
    }

    {
        auto inner_box = pushBack(guiCreate<GuiElement>());
        inner_box->primary_axis = GUI_PRIMARY_AXIS::X;
        inner_box->setSize(gui::fill(), gui::fill());
        inner_box->setStyleClasses({ "container" });

        tree_view = guiCreate<GuiTreeView>();
        tree_view->setOwner(this);
        tree_view->setMinSize(100, 0);
        tree_view->setSize(300, gui::fill());
        tree_view->addFlags(GUI_FLAG_RESIZE_X);
        tree_view->setStyleClasses({ "file-dir-tree" });
        inner_box->addChild(tree_view);
        updateDirTree(fsGetCurrentDirectory().c_str());

        auto right_container = guiCreate<GuiElement>();
        right_container->primary_axis = GUI_PRIMARY_AXIS::Y;
        right_container->setSize(gui::fill(), gui::fill());
        right_container->setStyleClasses({ "control", "container" });
        inner_box->pushBack(right_container);

        container = guiCreate<GuiFileContainer>();
        container->setOwner(this);
        right_container->pushBack(container);
        container->addFlags(GUI_FLAG_FOCUSABLE);
        container->subscribe<GuiEvt_Focus>([this](const GuiEvt_Focus& e) {
            e.new_focused = container;
        });
        container->subscribe<GuiEvt_LClick>([this](const GuiEvt_LClick& e) {
            clearSelected();
        });
        container->subscribe<GuiEvt_KeyDown>([this](const GuiEvt_KeyDown& e) {
            GuiElement* current = selected_items.empty() ? nullptr : selected_items.back();
            bool append = guiIsModifierKeyPressed(GUI_KEY_SHIFT);
            switch (e.vkey) {
            case VK_ESCAPE:
                clearSelected();
                break;
            case VK_F5:
                openDir(current_path);
                break;
            case VK_LEFT:
                if (auto next = container->findNextInDirection(current, GUI_NAV_LEFT)) {
                    selectOne(dynamic_cast<GuiFileListItem*>(next), append);
                }
                break;
            case VK_RIGHT:
                if (auto next = container->findNextInDirection(current, GUI_NAV_RIGHT)) {
                    selectOne(dynamic_cast<GuiFileListItem*>(next), append);
                }
                break;
            case VK_UP:
                if (auto next = container->findNextInDirection(current, GUI_NAV_UP)) {
                    selectOne(dynamic_cast<GuiFileListItem*>(next), append);
                }
                break;
            case VK_DOWN:
                if (auto next = container->findNextInDirection(current, GUI_NAV_DOWN)) {
                    selectOne(dynamic_cast<GuiFileListItem*>(next), append);
                }
                break;
            default: e.consume = false;
            }
        });
        container->subscribe<GuiEvt_Unichar>([this](const GuiEvt_Unichar& e) {
            e.invoke_next();
            //LOG_ERR("e.ch == " << e.ch);
            if (e.ch == 13) {
                if (!selected_items.empty()) {
                    auto item = selected_items[0];
                    if (item->is_directory) {
                        openDir(item->path_canonical);
                    } else {
                        invokeBubble(GuiEvt_FileConfirmed(item->path_canonical));
                    }
                }
                return;
            }

            if (e.ch == 9) {
                GuiElement* current = selected_items.empty() ? nullptr : selected_items[0];
                if (auto next = container->findNext(current)) {
                    selectOne(dynamic_cast<GuiFileListItem*>(next));
                }
                return;
            }
                
            if(e.ch == 8) {
                goUp();
            } else {
                bool is_repeat = !search_buffer.empty()
                    && search_buffer.find_first_not_of(search_buffer[0]) == std::string::npos
                    && e.ch == search_buffer[0];

                if (is_repeat) {
                    auto item = container->findNextMatch(
                        selected_items.empty() ? nullptr : selected_items[0],
                        search_buffer, true
                    );
                    if (item) {
                        selectOne(item);
                    }
                } else {
                    std::string new_search_buffer = search_buffer;
                    new_search_buffer.push_back(e.ch);
                    auto item = container->findFirstMatch(new_search_buffer);
                    if (!item) {
                        return;
                    }
                    selectOne(item);
                    search_buffer = new_search_buffer;
                }

                search_buffer_hint->setContent(search_buffer);
                search_buffer_hint->setHidden(false);
                gfxm::vec2 pos = guiConvertPosition(container, guiGetRoot()->getPopupLayer(), gfxm::vec2(0,0));
                search_buffer_hint->setPosition(pos.x, pos.y);
                guiCancelTick(this);
                guiScheduleTick(this, 1.0, GUI_TICK_TIMEOUT);
            }
        });

        auto bottom_container = right_container->pushBack(guiCreate<GuiElement>());
        bottom_container->primary_axis = GUI_PRIMARY_AXIS::X;
        bottom_container->setStyleClasses({ "control", "container" });
        bottom_container->setSize(gui::fill(), gui::content());
        auto bottom_left_container = bottom_container->pushBack(guiCreate<GuiElement>());
        bottom_left_container->primary_axis = GUI_PRIMARY_AXIS::Y;
        bottom_left_container->setStyleClasses({ "control", "container" });
        bottom_left_container->setSize(gui::fill(), gui::content());

        // Filters
        {
            filters.push_back({ "All", {} });
            selectFilter(0);
        }

        if(mode == GuiFileExplorerModeBrowse) {
            filter_input = bottom_left_container->pushBack(guiCreate<GuiInputString>("filter"));
            filter_input->subscribe<GuiEvt_Changed>([this](const GuiEvt_Changed&) {
                auto filter = filter_input->getValue();
                applyFilter(filter);
            });
            filter_input->setValue(filter);
        } else {
            filter_combo = bottom_left_container->pushBack(guiCreate<GuiComboBox>("filter"));
            for(int j = 0; j < filters.size(); ++j) {
                const auto& f = filters[j];
                std::string label = f.label;
                if (f.extensions.empty()) {
                    label += " (*.*)";
                } else {
                    label += std::format(" (*.{}", f.extensions[0]);
                    for (int i = 1; i < f.extensions.size(); ++i) {
                        label += std::format(", *.{}", f.extensions[i]);
                    }
                    label += ")";
                }
                filter_combo->addItem(label, j);
            }
            filter_combo->subscribe<GuiEvt_MenuCmd>([this](const GuiEvt_MenuCmd& e) {
                selectFilter(e.id);
            });
        }

        if(mode == GuiFileExplorerModeBrowse) {
            // --
        } else {
            file_input = bottom_left_container->pushBack(guiCreate<GuiInputString>("file"));
            file_input->subscribe<GuiEvt_Changed>([this](const GuiEvt_Changed&) {
                if (mode == GuiFileExplorerModeSave) {
                    bool enabled = !file_input->getValue().empty();
                    LOG_DBG("file_input changed: " << file_input->getValue());
                    btn_confirm->setEnabled(enabled);
                    return;
                }
            });

            btn_confirm = bottom_container->pushBack(guiCreate<GuiTextElement>());
            btn_confirm->setStyleClasses({ "button" });
            btn_confirm->setSize(gui::content(), gui::fill());
            if (mode == GuiFileExplorerModeOpen) {
                btn_confirm->setContent("Open");
                btn_confirm->setEnabled(false);
                btn_confirm->subscribe<GuiEvt_LClick>([this](const GuiEvt_LClick& e) {
                    confirmOpen();
                });
            } else if(mode == GuiFileExplorerModeSave) {
                btn_confirm->setContent("Save");
                btn_confirm->setEnabled(false);
                btn_confirm->subscribe<GuiEvt_LClick>([this](const GuiEvt_LClick& e) {
                    confirmSave();
                });
            }

            auto btn_cancel = bottom_container->pushBack(guiCreate<GuiTextElement>("Cancel"));
            btn_cancel->setStyleClasses({ "button" });
            btn_cancel->setSize(gui::content(), gui::fill());
        }

        search_buffer_hint = guiCreate<GuiTextElement>();
        search_buffer_hint->setStyleClasses({ "window", "paragraph" });
        guiGetRoot()->getPopupLayer()->pushBack(search_buffer_hint);
        search_buffer_hint->setHidden(true);
    }

    if (!cached_current_dir.empty()) {
        current_path = cached_current_dir;
    }

    openDir(current_path);

    tree_view->subscribe<GuiEvt_Selected>([this](const GuiEvt_Selected& e) {
        e.invoke_next(); // Let the tree view update it's internal state first

        GuiTreeItem* item = dynamic_cast<GuiTreeItem*>(e.elem);
        if (!item) {
            return;
        }
        openDir(item->user_string);
    });
}
GuiFileExplorer::~GuiFileExplorer() {
    guiCancelTick(this);
}

void GuiFileExplorer::updateDirTreeItem(GuiTreeItem* item, const std::filesystem::path& path) {
    //item->clearChildren();

    current_path = std::filesystem::absolute(path);

    struct file_t {
        std::string name;
        std::string absolute_path;
        bool is_dir;
    };
    std::vector<file_t> files;
    {
        HANDLE hFind = INVALID_HANDLE_VALUE;
        WIN32_FIND_DATA ffd = { 0 };
        hFind = FindFirstFile(MKSTR(current_path.string() << "\\*").c_str(), &ffd);
        if (hFind != INVALID_HANDLE_VALUE) {
            while (FindNextFile(hFind, &ffd) != 0) {
                file_t f;
                f.name = ffd.cFileName;
                f.absolute_path = MKSTR(current_path.string() << "\\" << ffd.cFileName);
                if (f.name == ".." || f.name == ".") {
                    continue;
                }
                f.is_dir = false;
                if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    f.is_dir = true;
                }
                files.push_back(f);
            }
            FindClose(hFind);
        }
    }

    for (int i = 0; i < files.size(); ++i) {
        auto& f = files[i];
        if (!f.is_dir) {
            continue;
        }
        auto child = item->addItem(f.name.c_str());
        child->user_string = f.absolute_path;
        updateDirTreeItem(child, path / f.name);
    }
}

void GuiFileExplorer::updateDirTree(const std::filesystem::path& path) {
    tree_view->clearChildren();

    current_path = std::filesystem::absolute(path);

    struct file_t {
        std::string name;
        std::string absolute_path;
        bool is_dir;
    };
    std::vector<file_t> files;
    {
        HANDLE hFind = INVALID_HANDLE_VALUE;
        WIN32_FIND_DATA ffd = { 0 };
        hFind = FindFirstFile(MKSTR(current_path.string() << "\\*").c_str(), &ffd);
        if (hFind != INVALID_HANDLE_VALUE) {
            while (FindNextFile(hFind, &ffd) != 0) {
                file_t f;
                f.name = ffd.cFileName;
                f.absolute_path = MKSTR(current_path.string() << "\\" << ffd.cFileName);
                if (f.name == ".." || f.name == ".") {
                    continue;
                }
                f.is_dir = false;
                if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    f.is_dir = true;
                }
                files.push_back(f);
            }
            FindClose(hFind);
        }
    }

    for (int i = 0; i < files.size(); ++i) {
        auto& f = files[i];
        if (!f.is_dir) {
            continue;
        }
        auto itm = tree_view->addItem(f.name.c_str());
        itm->user_string = f.absolute_path;
        updateDirTreeItem(itm, path / f.name);
    }
}

void GuiFileExplorer::goUp() {
    openDir(current_path.parent_path());
}
void GuiFileExplorer::openDir(const std::filesystem::path& path_, bool history_new) {
    auto resolved = resolveNavTarget(path_);
    if (!resolved) {
        return;
    }
    std::filesystem::path path = resolved.value();

    clearSelected();

    search_buffer.clear();

    current_path = std::filesystem::absolute(path);
    cached_current_dir = current_path;
    local_path = std::filesystem::relative(current_path);
    address_bar->setContent(local_path.string());
    container->clearItems();
    {
        struct file_t {
            std::string name;
            bool is_dir;
        };
        std::vector<file_t> files;
        {
            HANDLE hFind = INVALID_HANDLE_VALUE;
            WIN32_FIND_DATA ffd = { 0 };
            hFind = FindFirstFileA(MKSTR(current_path.string() << "\\*").c_str(), &ffd);
            if (hFind != INVALID_HANDLE_VALUE) {
                while (FindNextFileA(hFind, &ffd) != 0) {
                    file_t f;
                    f.name = ffd.cFileName;
                    f.is_dir = false;
                    if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                        f.is_dir = true;
                    }
                    files.push_back(f);
                }
                FindClose(hFind);
            } else {
                DWORD err = GetLastError();
                LOG_ERR("FindFirstFile error: 0x" << std::hex << err);
            }
        }

        if(files.size() > 0) {
            std::sort(files.begin() + 1, files.end(), [](const file_t& a, const file_t& b) {
                if (a.is_dir && b.is_dir) {
                    return a.name < b.name;
                } else if(a.is_dir || b.is_dir) {
                    return a.is_dir > b.is_dir;
                } else {
                    return a.name < b.name;
                }
                });
        }
        for (auto& f : files) {                
            std::filesystem::path absolute_path;
            if (strncmp(f.name.c_str(), "..", 3) == 0) {
                absolute_path = path.parent_path();
            } else {
                absolute_path = path / f.name.c_str();
            }

            //LOG(absolute_path.string());

            const guiFileThumbnail* thumb = 0;
            // TODO: Async thumb loading, caching
            if (absolute_path != path.root_path()) {
                thumb = guiFileThumbnailLoad(
                    absolute_path.parent_path().string().c_str(),
                    absolute_path.filename().string().c_str(),
                    54
                );
            } else {
                thumb = guiFileThumbnailLoad(
                    absolute_path.string().c_str(),
                    0,
                    54
                );
            }
            auto itm = container->addItem(f.name.c_str(), f.is_dir, thumb);
            itm->subscribe<GuiEvt_LClick>([this, itm](const GuiEvt_LClick& e){
                if (e.is_double) {
                    //notifyOwner<GuiFileListItem*>(GUI_NOTIFY::FILE_ITEM_DOUBLE_CLICK, this);
                    auto& name = itm->getName();
                    if(itm->is_directory) {
                        std::filesystem::path path_new = current_path;
                        if (name == std::string("..")) {
                            path_new = current_path.parent_path();
                        } else if(name == std::string(".")) {
                            path_new = current_path;
                        } else {
                            path_new /= name;
                        }
                        openDir(path_new);
                    } else {
                        invokeBubble(GuiEvt_FileConfirmed(itm->path_canonical));
                    }
                } else {
                    selectOne(itm, guiIsModifierKeyPressed(GUI_KEY_SHIFT));
                }
            });
            itm->path_canonical = std::filesystem::canonical(absolute_path).string();
        }
    }

    applyFilter(filter);
    if (history_new) {
        historyNew(current_path);
    }
    updateHistoryButtons();
}

void GuiFileExplorer::applyFilter(const std::string& filter) {
    this->filter = filter;
    for (int i = 0; i < container->itemCount(); ++i) {
        auto item = container->getItem(i);
        if (item->is_directory) {
            continue;
        }
        if (mode == GuiFileExplorerModeBrowse) {
            item->setHidden(item->getName().rfind(filter) == std::string::npos);
        } else {
            item->setHidden(!matchesFilterString(item->getName(), filter));
        }
    }
    clearSelected();
    container->resetScroll();
}

void GuiFileExplorer::onTick(float dt, GUI_TICK_ID id) {
    search_buffer = "";
    search_buffer_hint->setHidden(true);
}