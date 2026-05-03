#pragma once

#include <assert.h>
#include <stack>
#include "platform/platform.hpp"
#include "math/gfxm.hpp"
#include "gpu/gpu_pipeline.hpp"
#include "gpu/gpu_text.hpp"

#include "gui/gui_hit.hpp"
#include "gui/gui_draw.hpp"

#include "gui/gui_values.hpp"
#include "gui/gui_color.hpp"

#include "gui/gui_system.hpp"

#include "gui/elements/dock_space.hpp"


#include "gui/gui_layout_helpers.hpp"

#include "gui/elements/text_element.hpp"
#include "gui/elements/icon.hpp"
#include "gui/elements/label.hpp"
#include "gui/elements/input.hpp"
#include "gui/elements/image.hpp"
#include "gui/elements/button.hpp"
#include "gui/elements/input_numeric.hpp"
#include "gui/elements/input_string.hpp"
#include "gui/elements/input_resource.hpp"
#include "gui/elements/input_file_path.hpp"
#include "gui/elements/title_bar.hpp"


inline void guiLayoutSplitRectX(const gfxm::rect& rc, gfxm::rect& a, gfxm::rect& b, float width_a) {
    const float w = rc.max.x - rc.min.x;
    const float w0 = gfxm::_min(w, width_a);
    const float w1 = w - w0;

    gfxm::rect rc_original = rc;

    a = gfxm::rect(
        gfxm::vec2(rc_original.min.x, rc_original.min.y),
        gfxm::vec2(rc_original.min.x + w0, rc_original.max.y)
    );
    b = gfxm::rect(
        gfxm::vec2(a.max.x, a.min.y),
        gfxm::vec2(rc_original.max.x, rc_original.max.y)
    );
}
inline void guiLayoutSplitRectY(const gfxm::rect& rc, gfxm::rect& a, gfxm::rect& b, float height_a) {
    const float h = rc.max.y - rc.min.y;
    const float h0 = gfxm::_min(h, height_a);
    const float h1 = h - h0;

    gfxm::rect rc_original = rc;

    a = gfxm::rect(
        gfxm::vec2(rc_original.min.x, rc_original.min.y),
        gfxm::vec2(rc_original.max.x, rc_original.min.y + h0)
    );
    b = a;
    b.min.y = a.max.y;
    b.max.y = rc_original.max.y;
}


class GuiCheckBox : public GuiElement {
    GuiTextBuffer caption;
    bool* value = 0;
public:
    GuiCheckBox(const char* cap = "CheckBox", bool* data = 0)
        : value(data) {
        setSize(gui::fill(), gui::em(2));
        setStyleClasses({ "control" });
        caption.replaceAll(getFont(), cap, strlen(cap));
        subscribe<GuiEvt_LClick>([this](const GuiEvt_LClick&) {
            toggle();
        });
    }

    void toggle() {
        if (value) {
            *value = !(*value);
        }
    }

    void onLayout(const gfxm::vec2& extents, uint64_t flags) override {
        Font* font = getFont();

        rc_bounds = gfxm::rect(gfxm::vec2(0, 0), extents);
        rc_bounds.max.y = rc_bounds.min.y + font->getLineHeight() * 2.f;
        client_area = rc_bounds;

        caption.prepareDraw(font, false);

        float box_sz = client_area.max.y - client_area.min.y;
        client_area.max.x = client_area.min.x + box_sz + GUI_MARGIN + caption.getBoundingSize().x;
    }
    void onDraw() override {
        gfxm::rect rc_box = client_area;
        rc_box.max.x = rc_box.min.x + (rc_box.max.y - rc_box.min.y);
        
        guiDrawCheckBox(rc_box, value ? *value : false, isHovered());

        caption.draw(getFont(), gfxm::vec2(rc_box.max.x + GUI_MARGIN, client_area.min.y), GUI_COL_TEXT, GUI_COL_ACCENT);
    }
};

class GuiRadioButton : public GuiElement {
    GuiTextBuffer caption;
public:
    GuiRadioButton(const char* cap = "RadioButton") {
        setSize(gui::fill(), gui::em(2));
        setStyleClasses({ "control" });
        caption.replaceAll(getFont(), cap, strlen(cap));
    }

    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        return false;
    }
    void onLayout(const gfxm::vec2& extents, uint64_t flags) override {
        Font* font = getFont();

        rc_bounds = gfxm::rect(gfxm::vec2(0, 0), extents);
        rc_bounds.max.y = rc_bounds.min.y + font->getLineHeight();
        client_area = rc_bounds;

        caption.prepareDraw(font, false);
    }
    void onDraw() override {
        gfxm::rect rc_box = client_area;
        rc_box.max.x = rc_box.min.x + (rc_box.max.y - rc_box.min.y);
        guiDrawCircle(rc_box.center(), rc_box.size().x * .5f, true, GUI_COL_HEADER);
        guiDrawCircle(rc_box.center(), rc_box.size().x * .24f, true, GUI_COL_TEXT);

        caption.draw(getFont(), gfxm::vec2(rc_box.max.x + GUI_MARGIN, client_area.min.y), GUI_COL_TEXT, GUI_COL_ACCENT);
    }
};

#include "gui/elements/menu_list.hpp"

#include "gui/elements/combo_box.hpp"

#include "gui/elements/collapsing_header.hpp"

#include "gui/elements/file_container.hpp"

class GuiTreeView : public GuiElement {
    GuiTreeItem* selected_item = 0;
public:
    GuiTreeView() {
        setSize(gui::fill(), 250);
        //setMaxSize(0, 0);
        setMinSize(0, 100);
        addFlags(GUI_FLAG_RESIZE_Y);

        setStyleClasses({ "tree-view" });

        auto models = addItem("Models");
        models->addItem("chara_24");
        models->addItem("sword");
        models->addItem("gun");
        auto b = addItem("Textures");
        b->addItem("bricks.png");
        b->addItem("terrain")->addItem("grass.png");
        addItem("Shaders")->addItem("default.glsl");
    }

    GuiTreeItem* addItem(const char* name) {
        auto item = new GuiTreeItem(name);
        addChild(item);
        return item;
    }

    GuiTreeItem* getSelectedItem() { return selected_item; }

    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::NOTIFY: {
            switch (params.getA<GUI_NOTIFY>()) {
            case GUI_NOTIFY::TREE_ITEM_CLICK: {
                auto item = params.getB<GuiTreeItem*>();
                if (selected_item) {
                    selected_item->setSelected(false);
                }
                item->setSelected(true);
                bool should_notify = selected_item != item;
                selected_item = item;
                if (should_notify) {
                    notifyOwner<GuiTreeView*>(GUI_NOTIFY::TREE_VIEW_SELECTED, this);
                }
                return true;
            }
            }
            return false;
        }
        }
        return GuiElement::onMessage(msg, params);
    }
};

class GuiDemoWindow : public GuiWindow {
public:
    GuiDemoWindow()
    : GuiWindow("DemoWindow") {
        setPosition(850, 200);
        setSize(400, 600);

        auto header_input = pushBack(new GuiCollapsingHeader("Inputs"));

        header_input->pushBack(new GuiInputString)->setValue("Text string");
        header_input->pushBack(new GuiInputNumeric);
        header_input->pushBack(new GuiInputNumeric2);
        header_input->pushBack(new GuiInputNumeric3);
        header_input->pushBack(new GuiInputNumeric4);
        header_input->pushBack(new GuiInputResource);

        auto header_old_input = pushBack(new GuiCollapsingHeader("Old Inputs"));

        header_old_input->pushBack(new GuiLabel("Hello, World!"));
        header_old_input->pushBack(new GuiComboBox());
        header_old_input->pushBack(new GuiCollapsingHeader("CollapsingHeader", true))
            ->pushBack("Hello, World!");
        header_old_input->pushBack(new GuiCheckBox());
        header_old_input->pushBack(new GuiRadioButton());
        
        auto header_text = pushBack(new GuiCollapsingHeader("Text"));
        
        header_text->pushBack(R"(Then Fingolfin beheld (as it seemed to him) the utter ruin of the Noldor,
and the defeat beyond redress of all their houses;
and filled with wrath and despair he mounted upon Rochallor his great horse and rode forth alone,
and none might restrain him.)",
            { "paragraph" }
        );
        
        GuiElement* head = new GuiElement;
        head->setSize(gui::perc(100), gui::em(7));
        head->setStyleClasses({ "control", "notification" });
        header_text->pushBack(head);

        header_text->pushBack(R"(He passed over Dor-nu-Fauglith like a wind amid the dust,
and all that beheld his onset fled in amaze, thinking that Orome himself was come:
for a great madness of rage was upon him, so that his eyes shone like the eyes of the Valar.
Thus he came alone to Angband's gates, and he sounded his horn,
and smote once more upon the brazen doors,
and challenged Morgoth to come forth to single combat. And Morgoth came.)",
            { "paragraph" }
        );

        GuiTextElement* text = new GuiTextElement;
        text->setContent("Example notification");
        text->setStyleClasses({ "header" });
        GuiTextElement* text2 = new GuiTextElement;
        text2->setContent("Notification body");
        text2->setStyleClasses({ "paragraph" });
        head->pushBack(text);
        head->pushBack(text2);

        auto header_other = pushBack(new GuiCollapsingHeader("Other"));

        header_other->pushBack(new GuiTreeView(), GUI_FLAG_RESIZE);
        header_other->pushBack(new GuiImage(resGet<gpuTexture2d>("1648920106773.jpg").get()));
        header_other->pushBack(new GuiButton("Button A"));
        header_other->pushBack(new GuiButton("Button B"));
    }
};

#include <shellapi.h>
#include <ShObjIdl.h>
#include "gui/filesystem/gui_file_thumbnail.hpp"
#include <filesystem>
class GuiFileExplorerWindow : public GuiWindow {
    //std::unique_ptr<GuiInputTextLine> dir_path;
    std::unique_ptr<GuiTreeView> tree_view;
    std::unique_ptr<GuiFileContainer> container;

    std::filesystem::path current_path;
    std::vector<GuiFileListItem*> selected_items;
public:
    GuiFileExplorerWindow()
        : GuiWindow("FileExplorer") {
        setSize(800, 600);
        std::string sfname;
        sfname.resize(MAX_PATH);
        GetFullPathName(".", MAX_PATH, &sfname[0], 0);
        current_path = sfname;

        auto btn_back = new GuiIconButton(guiLoadIcon("svg/entypo/arrow-bold-left.svg"));
        addChild(btn_back);
        auto btn_forward = new GuiIconButton(guiLoadIcon("svg/entypo/arrow-bold-right.svg"));
        addChild(btn_forward);
        btn_forward->setFlags(GUI_FLAG_SAME_LINE);
        auto btn_up = new GuiIconButton(guiLoadIcon("svg/entypo/arrow-bold-up.svg"));
        addChild(btn_up);
        btn_up->setFlags(GUI_FLAG_SAME_LINE);
        /*
        dir_path.reset(new GuiInputTextLine);
        addChild(dir_path.get());
        dir_path->setText(sfname.c_str());
        dir_path->setFlags(GUI_FLAG_SAME_LINE);
        */
        tree_view.reset(new GuiTreeView());
        tree_view->setOwner(this);
        tree_view->setMinSize(100, 0);
        tree_view->setSize(300, gui::fill());
        tree_view->addFlags(GUI_FLAG_RESIZE_X);
        tree_view->setStyleClasses({ "file-dir-tree" });
        addChild(tree_view.get());
        updateDirTree(fsGetCurrentDirectory().c_str());

        container.reset(new GuiFileContainer());
        container->setOwner(this);
        container->addFlags(GUI_FLAG_SAME_LINE);
        addChild(container.get());

        openDir(std::filesystem::current_path());
    }

    void updateDirTreeItem(GuiTreeItem* item, const std::filesystem::path& path) {
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
    void updateDirTree(const std::filesystem::path& path) {
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

    void openDir(const std::filesystem::path& path) {
        for (auto itm : selected_items) {
            itm->setSelected(false);
        }
        selected_items.clear();

        current_path = std::filesystem::absolute(path);
        //dir_path->setText(path.string().c_str());
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
                            guiSendMessage(this, GUI_MSG::FILE_EXPL_OPEN_FILE, itm, 0, 0);
                        }
                    } else {
                        //notifyOwner<GuiFileListItem*>(GUI_NOTIFY::FILE_ITEM_CLICK, this);
                        for (auto sel_item : selected_items) {
                            sel_item->setSelected(false);
                        }
                        selected_items.clear();
                        selected_items.push_back(itm);
                        itm->setSelected(true);
                    }
                });
                itm->path_canonical = std::filesystem::canonical(absolute_path).string();
            }
        }
    }

    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::NOTIFY:
            switch (params.getA<GUI_NOTIFY>()) {
            case GUI_NOTIFY::TREE_VIEW_SELECTED: {
                GuiTreeView* tree = params.getB<GuiTreeView*>();
                GuiTreeItem* item = tree->getSelectedItem();
                if (!item) {
                    return true;
                }
                openDir(item->user_string);
                return true;
            }
            }
            break;
        }
        return GuiWindow::onMessage(msg, params);
    }
};

