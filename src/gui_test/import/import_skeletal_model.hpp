#pragma once

#include "import/import_window.hpp"
#include "import/import_settings_fbx.hpp"

class GuiImportFbxWnd : public GuiImportWindow {
    ImportSettingsFbx settings;
    void initControls() {
        addChild(new GuiButton("Import", 0, [this]() {
            settings.write_import_file();
            settings.do_import();
            //sendCloseMessage();
        }));
        auto save_btn = new GuiButton("Save import file", 0, [this]() {
            settings.write_import_file();
        });
        save_btn->addFlags(GUI_FLAG_SAME_LINE);
        addChild(save_btn);
        auto cancel_btn = new GuiButton("Cancel", 0, [this]() {
            //sendCloseMessage();
        });
        cancel_btn->addFlags(GUI_FLAG_SAME_LINE);
        addChild(cancel_btn);

        fs_path current_dir = fsGetCurrentDirectory();

        addChild(new GuiInputFilePath("Source", &settings.source_path, GUI_INPUT_FILE_READ, "fbx", current_dir.c_str()));
        addChild(new GuiInputFilePath("Import file", &settings.import_file_path, GUI_INPUT_FILE_WRITE, "fbx.import", current_dir.c_str()));

        addChild(new GuiInputFloat("Scale factor", &settings.scale_factor, 3));
        
        auto model = new GuiCollapsingHeader("Model");
        addChild(model);
        model->addChild(new GuiCheckBox("Import model", &settings.import_model));
        model->addChild(new GuiInputFilePath("Output file", &settings.model_path, GUI_INPUT_FILE_WRITE, "skeletal_model", current_dir.c_str()));

        auto skeleton = new GuiCollapsingHeader("Skeleton");
        addChild(skeleton);
        skeleton->addChild(new GuiCheckBox("Overwrite skeleton", &settings.overwrite_skeleton));
        skeleton->addChild(new GuiInputFilePath("Skeleton path", &settings.skeleton_path, GUI_INPUT_FILE_WRITE, "skeleton", current_dir.c_str()));

        auto animations = new GuiCollapsingHeader("Animation");
        addChild(animations);
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
        
        auto materials = new GuiCollapsingHeader("Materials");
        addChild(materials);
        materials->addChild(new GuiCheckBox("Import materials", &settings.import_materials));
        {
            for (int i = 0; i < settings.materials.size(); ++i) {
                auto mat = new GuiCollapsingHeader(settings.materials[i].name.c_str(), false, false);
                materials->addChild(mat);
                mat->addChild(new GuiCheckBox("Overwrite", &settings.materials[i].overwrite));
                mat->addChild(new GuiInputFilePath("File path", &settings.materials[i].output_path, GUI_INPUT_FILE_WRITE, "material", current_dir.c_str()));
            }
        }
        /*
        auto meshes = new GuiCollapsingHeader("Meshes");
        addChild(meshes);
        meshes->addChild(new GuiCheckBox("Import meshes"));
        meshes->addChild(new GuiCollapsingHeader("Body"));
        meshes->addChild(new GuiCollapsingHeader("Head"));
        meshes->addChild(new GuiCollapsingHeader("Weapon"));
        */
    }
public:
    GuiImportFbxWnd()
    : GuiImportWindow("Import skeletal model") {
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