#pragma once

#include "import/import_window.hpp"
#include "import/import_settings_fbx.hpp"

class GuiImportFbxWnd : public GuiImportWindow {
    ImportSettingsFbx settings;
    gpuRenderTarget render_target;
    gpuRenderBucket render_bucket;
    GameRenderInstance render_instance;
    RHSHARED<mdlSkeletalModelMaster> preview_model;
    HSHARED<mdlSkeletalModelInstance> preview_model_instance;

    void initPreview() {
        render_bucket.clear();

        if (preview_model) {
            preview_model_instance->despawn(render_instance.world.getRenderScene());
        }

        assimpLoadedResources resources;
        preview_model.reset_acquire();
        assimpImporter importer;
        if (!importer.loadFile(settings.source_path.c_str(), settings.scale_factor)) {
            preview_model_instance.reset();
            preview_model.reset();            
            return;
        }
        importer.loadSkeletalModel(preview_model.get(), &resources);
        preview_model_instance = preview_model->createInstance();
        preview_model_instance->spawn(render_instance.world.getRenderScene());
    }

    void initControls() {
        clearChildren();
        if (game_render_instances.count(&render_instance)) {
            game_render_instances.erase(&render_instance);
        }

        auto container = new GuiElement();
        container->setSize(gui::perc(40), gui::perc(100));
        container->setStyleClasses({ "fbx-import-container" });
        pushBack(container);

        auto viewport = new GuiViewport();
        viewport->setOwner(this);
        viewport->setSize(gui::perc(100), gui::perc(100));
        viewport->addFlags(GUI_FLAG_SAME_LINE);

        gpuGetPipeline()->initRenderTarget(&render_target);
        render_instance.render_target = &render_target;
        render_instance.render_bucket = &render_bucket;
        gfxm::vec3 cam_pos = gfxm::vec3(3, 1.5, 3);
        gfxm::mat4 view = gfxm::lookAt(cam_pos, gfxm::vec3(), gfxm::vec3(0, 1, 0));
        render_instance.view_transform = view;
        game_render_instances.insert(&render_instance);

        viewport->render_instance = &render_instance;

        pushBack(viewport);

        container->pushBack(new GuiButton("Import", 0, [this]() {
            settings.write_import_file();
            settings.do_import();
            //sendCloseMessage();
        }));
        auto save_btn = new GuiButton("Save import file", 0, [this]() {
            settings.write_import_file();
        });
        save_btn->addFlags(GUI_FLAG_SAME_LINE);
        container->pushBack(save_btn);
        auto cancel_btn = new GuiButton("Cancel", 0, [this]() {
            //sendCloseMessage();
        });
        cancel_btn->addFlags(GUI_FLAG_SAME_LINE);
        container->pushBack(cancel_btn);

        fs_path current_dir = fsGetCurrentDirectory();

        container->pushBack(new GuiInputFilePath("Source", &settings.source_path, GUI_INPUT_FILE_READ, "fbx", current_dir.c_str()));
        container->pushBack(new GuiInputFilePath("Import file", &settings.import_file_path, GUI_INPUT_FILE_WRITE, "fbx.import", current_dir.c_str()));

        auto inp_scale_factor = new GuiInputFloat("Scale factor", &settings.scale_factor, 3);
        container->pushBack(inp_scale_factor);
        
        inp_scale_factor->on_change = [this](float value){
            settings.scale_factor = value;
            initPreview();
        };

        GuiButton* btn_add_skeletal_model = new GuiButton("Skeletal model", guiLoadIcon("svg/entypo/plus.svg"));
        GuiCollapsingHeader* header_skeletal = new GuiCollapsingHeader("Skeletal model", true);
        
        btn_add_skeletal_model->on_click = [this, header_skeletal, btn_add_skeletal_model]() {
            settings.import_skeletal_model = true;
            header_skeletal->setHidden(!settings.import_skeletal_model);
            btn_add_skeletal_model->setHidden(settings.import_skeletal_model);
        };
        btn_add_skeletal_model->setStyleClasses({ "control", "button", "button-important" });
        container->pushBack(btn_add_skeletal_model);

        {
            header_skeletal->on_remove = [this, header_skeletal, btn_add_skeletal_model](){
                settings.import_skeletal_model = false;
                header_skeletal->setHidden(!settings.import_skeletal_model);
                btn_add_skeletal_model->setHidden(settings.import_skeletal_model);
            };
            container->pushBack(header_skeletal);

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
        container->pushBack(btn_add_static_model);

        {
            header_static->on_remove = [this, header_static, btn_add_static_model](){
                settings.import_static_model = false;
                header_static->setHidden(!settings.import_static_model);
                btn_add_static_model->setHidden(settings.import_static_model);
            };
            container->pushBack(header_static);

            header_static->pushBack(new GuiInputFilePath("Output file", &settings.static_model_path, GUI_INPUT_FILE_WRITE, "static_model", current_dir.c_str()));

        }

        header_static->setHidden(!settings.import_static_model);
        btn_add_static_model->setHidden(settings.import_static_model);

        
        auto materials = new GuiCollapsingHeader("Materials");
        container->pushBack(materials);
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
    : GuiImportWindow("Import model"), render_bucket(gpuGetPipeline(), 200) {
        addFlags(GUI_FLAG_BLOCKING);
        setSize(1200, 800);
        setPosition(800, 200);
    }
    ~GuiImportFbxWnd() {
        if (game_render_instances.count(&render_instance)) {
            game_render_instances.erase(&render_instance);
        }
    }

    bool createImport(const std::string& spath) override {
        if (!settings.from_source(spath)) {
            return false;
        }
        initControls();
        initPreview();
        return true;
    }
    bool loadImport(const std::string& spath) override {
        if (!settings.read_import_file(spath)) {
            return false;
        }
        initControls();
        initPreview();
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

    void onLayout(const gfxm::vec2& extents, uint64_t flags) override {
        GuiImportWindow::onLayout(extents, flags);
    }
};