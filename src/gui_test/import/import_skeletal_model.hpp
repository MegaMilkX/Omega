#pragma once

#include "import/import_window.hpp"
#include "import/import_settings_fbx.hpp"

class GuiImportFbxWnd : public GuiImportWindow {
    ImportSettingsFbx settings;
    void initControls() {
        clearChildren();

        pushBack(new GuiButton("Import", 0, [this]() {
            settings.write_import_file();
            settings.do_import();
            //sendCloseMessage();
        }));
        auto save_btn = new GuiButton("Save import file", 0, [this]() {
            settings.write_import_file();
        });
        save_btn->addFlags(GUI_FLAG_SAME_LINE);
        pushBack(save_btn);
        auto cancel_btn = new GuiButton("Cancel", 0, [this]() {
            //sendCloseMessage();
        });
        cancel_btn->addFlags(GUI_FLAG_SAME_LINE);
        pushBack(cancel_btn);

        fs_path current_dir = fsGetCurrentDirectory();

        pushBack(new GuiInputFilePath("Source", &settings.source_path, GUI_INPUT_FILE_READ, "fbx", current_dir.c_str()));
        pushBack(new GuiInputFilePath("Import file", &settings.import_file_path, GUI_INPUT_FILE_WRITE, "fbx.import", current_dir.c_str()));

        pushBack(new GuiInputFloat("Scale factor", &settings.scale_factor, 3));

        GuiButton* btn_add_skeletal_model = new GuiButton("Skeletal model", guiLoadIcon("svg/entypo/plus.svg"));
        GuiCollapsingHeader* header_skeletal = new GuiCollapsingHeader("Skeletal model", true);
        
        btn_add_skeletal_model->on_click = [this, header_skeletal, btn_add_skeletal_model]() {
            settings.import_skeletal_model = true;
            header_skeletal->setHidden(!settings.import_skeletal_model);
            btn_add_skeletal_model->setHidden(settings.import_skeletal_model);
        };
        btn_add_skeletal_model->setStyleClasses({ "control", "button", "button-important" });
        pushBack(btn_add_skeletal_model);

        {
            header_skeletal->on_remove = [this, header_skeletal, btn_add_skeletal_model](){
                settings.import_skeletal_model = false;
                header_skeletal->setHidden(!settings.import_skeletal_model);
                btn_add_skeletal_model->setHidden(settings.import_skeletal_model);
            };
            pushBack(header_skeletal);

            auto model = new GuiCollapsingHeader("Model");
            header_skeletal->pushBack(model);
            model->addChild(new GuiCheckBox("Import model", &settings.import_model));
            model->addChild(new GuiInputFilePath("Output file", &settings.model_path, GUI_INPUT_FILE_WRITE, "skeletal_model", current_dir.c_str()));

            auto skeleton = new GuiCollapsingHeader("Skeleton");
            header_skeletal->pushBack(skeleton);
            skeleton->addChild(new GuiCheckBox("Overwrite skeleton", &settings.overwrite_skeleton));
            skeleton->addChild(new GuiInputFilePath("Skeleton path", &settings.skeleton_path, GUI_INPUT_FILE_WRITE, "skeleton", current_dir.c_str()));

            auto animations = new GuiCollapsingHeader("Animation");
            header_skeletal->pushBack(animations);
            animations->addChild(new GuiCheckBox("Import animations", &settings.import_animations));
            {
                for (int i = 0; i < settings.tracks.size(); ++i) {
                    auto& track = settings.tracks[i];
                    auto anim = new GuiCollapsingHeader(track.source_track_name.c_str(), false, false);
                    animations->addChild(anim);
                    anim->addChild(new GuiComboBox("Source track", track.source_track_name.c_str()));
                    anim->addChild(new GuiInputFilePath("Output file", &track.output_path, GUI_INPUT_FILE_WRITE, "animation", current_dir.c_str()));
                    anim->addChild(new GuiInputInt32_2("Range", (int*)&track.range));
                    anim->addChild(new GuiCheckBox("Root motion"));
                    anim->addChild(new GuiComboBox("Reference bone"));
                }
            }
        }

        header_skeletal->setHidden(!settings.import_skeletal_model);
        btn_add_skeletal_model->setHidden(settings.import_skeletal_model);


        GuiButton* btn_add_static_model = new GuiButton("Static model", guiLoadIcon("svg/entypo/plus.svg"));
        GuiCollapsingHeader* header_static = new GuiCollapsingHeader("Static model", true);

        btn_add_static_model->on_click = [this, header_static, btn_add_static_model]() {
            settings.import_static_model = true;
            header_static->setHidden(!settings.import_static_model);
            btn_add_static_model->setHidden(settings.import_static_model);
        };
        btn_add_static_model->setStyleClasses({ "control", "button", "button-important" });
        pushBack(btn_add_static_model);

        {
            header_static->on_remove = [this, header_static, btn_add_static_model](){
                settings.import_static_model = false;
                header_static->setHidden(!settings.import_static_model);
                btn_add_static_model->setHidden(settings.import_static_model);
            };
            pushBack(header_static);

            header_static->pushBack(new GuiInputFilePath("Output file", &settings.static_model_path, GUI_INPUT_FILE_WRITE, "static_model", current_dir.c_str()));

        }

        header_static->setHidden(!settings.import_static_model);
        btn_add_static_model->setHidden(settings.import_static_model);

        
        auto materials = new GuiCollapsingHeader("Materials");
        pushBack(materials);
        materials->addChild(new GuiCheckBox("Import materials", &settings.import_materials));
        {
            for (int i = 0; i < settings.materials.size(); ++i) {
                auto mat = new GuiCollapsingHeader(settings.materials[i].name.c_str(), false, false);
                materials->addChild(mat);
                mat->addChild(new GuiCheckBox("Overwrite", &settings.materials[i].overwrite));
                mat->addChild(new GuiInputFilePath("File path", &settings.materials[i].output_path, GUI_INPUT_FILE_WRITE, "material", current_dir.c_str()));
            }
        }
    }
public:
    GuiImportFbxWnd()
    : GuiImportWindow("Import model") {
        addFlags(GUI_FLAG_BLOCKING);
        setSize(600, 800);
        setPosition(800, 200);
    }
    ~GuiImportFbxWnd() {}

    bool createImport(const std::string& spath) override {
        if (!settings.from_source(spath)) {
            return false;
        }
        initControls();
        return true;
    }
    bool loadImport(const std::string& spath) override {
        if (!settings.read_import_file(spath)) {
            return false;
        }
        initControls();
        return true;
    }
    
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::NOTIFY: {
            switch (params.getA<GUI_NOTIFY>()) {
            case GUI_NOTIFY::BUTTON_CLICKED:/*
                std::wstring import_file_name = importer->source_path;
                size_t offset = import_file_name.find_first_of(L'\0', 0);
                if (std::string::npos != offset) {
                    import_file_name.resize(offset);
                }
                import_file_name = import_file_name + std::wstring(L".import");
                HANDLE f = CreateFileW(import_file_name.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
                if (f != INVALID_HANDLE_VALUE) {
                    nlohmann::json json = {};
                    importer->get_type().serialize_json(json, importer);
                    std::string buf = json.dump(2);
                    DWORD written = 0;
                    if (!WriteFile(f, buf.c_str(), buf.size(), &written, 0)) {
                        LOG_ERR("Failed to write import file");
                    }
                    CloseHandle(f);
                } else {
                    // TODO: Report an error to the user
                }*/
                return true;
            }
            return false;
        }
        }
        return GuiWindow::onMessage(msg, params);
    }
};