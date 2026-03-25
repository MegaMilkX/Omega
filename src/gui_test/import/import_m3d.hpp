#pragma once

#include "gui/elements/window.hpp"
#include "gui/elements/viewport/gui_viewport.hpp"
#include "gizmo/gizmo.hpp"

#include "m3d/m3d_project.hpp"
#include "m3d/skeletal_instance.hpp"


class GuiImportM3dWindow : public GuiWindow {
    GameRenderInstance render_instance;
    gpuRenderBucket render_bucket;
    gpuRenderTarget render_target;

    GizmoContext* gizmo_ctx = nullptr;

    m3dpProject m3d_proj;
    ResourceRef<m3dModel> m3d;
    std::unique_ptr<m3dSkeletalInstance> m3d_inst;

    std::string project_path;

    // preview
    struct {
        int current_anim = 0;
        float t_anim = .0f;
    };

    // reference plane
    struct {
        gpuMesh mesh;
        std::unique_ptr<gpuRenderable> renderable;
        gpuTransformBlock* transform_block = nullptr;
        ResourceRef<gpuMaterial> material;
    } ref_plane;

    void initFromSource(const std::string& path) {
        m3d_proj.initFromSource(path);
        m3d = ResourceManager::get()->create<m3dModel>("imported_model");
        m3d_proj.import(*m3d.get());
        m3d_inst.reset(new m3dSkeletalInstance);
        m3d_inst->init(m3d);
    }
    void initFromProject(const std::string& path) {
        m3d_proj.load(path);
        m3d = ResourceManager::get()->create<m3dModel>("imported_model");
        m3d_proj.import(*m3d.get());
        m3d_inst.reset(new m3dSkeletalInstance);
        m3d_inst->init(m3d);
    }

    void initControls() {
        auto container = new GuiElement();
        container->setSize(gui::perc(40), gui::perc(100));
        container->setStyleClasses({ "fbx-import-container" });
        pushBack(container);

        auto btn_import = new GuiButton(
            "Import",
            guiLoadIcon("svg/Entypo/arrow-bold-down.svg"),
            [this]() {
                m3d_proj.save_m3d();
                // TMP: instantly check loading
                loadResource<m3dModel>(
                    std::filesystem::path(m3d_proj.source_path).replace_extension().string()
                );
            }
        );
        btn_import->addFlags(GUI_FLAG_SAME_LINE);
        auto btn_save = new GuiButton(
            "Save project",
            guiLoadIcon("svg/Entypo/save.svg"),
            [this]() {
                m3d_proj.save(project_path);
            }
        );
        btn_save->addFlags(GUI_FLAG_SAME_LINE);
        container->pushBack(btn_import);
        container->pushBack(btn_save);

        fs_path current_dir = fsGetCurrentDirectory();
        auto inp_source_path = new GuiInputFilePath(
            "source",
            [this](const std::string& path){
                initFromSource(path);
            },
            [this]()->std::string {
                return m3d_proj.source_path;
            },
            GUI_INPUT_FILE_READ, "fbx,glb,gltf", current_dir.c_str());
        /*
        auto inp_project_path = new GuiInputFilePath("Project", &project_path, GUI_INPUT_FILE_WRITE, "m3dp", current_dir.c_str());
        auto inp_m3d_path = new GuiInputFilePath("Output model", &m3d_proj.out_model_path, GUI_INPUT_FILE_WRITE, "m3d", current_dir.c_str());
        auto inp_skl_path = new GuiInputFilePath("Output skeleton", &m3d_proj.out_skeleton_path, GUI_INPUT_FILE_WRITE, "skl", current_dir.c_str());
        */
        container->pushBack(inp_source_path);
        /*container->pushBack(inp_project_path);
        container->pushBack(inp_m3d_path);
        container->pushBack(inp_skl_path);*/

        {
            auto inp_res_id = new GuiInputString("resource id");
            inp_res_id->setValue(m3d_proj.out_model_resource_id);
            container->pushBack(inp_res_id);
        }

        {
            auto head = new GuiCollapsingHeader("Skeleton");
            container->pushBack(head);

            auto combo_mode = new GuiComboBox("import mode", "Embed/Separate/External");
            head->pushBack(combo_mode);

            auto inp_res_id = new GuiInputString("resource id");
            inp_res_id->setValue(m3d_proj.out_skeleton_resource_id);
            head->pushBack(inp_res_id);
        }

        {
            auto head = new GuiCollapsingHeader("Materials");
            container->pushBack(head);
        }

        {
            auto head = new GuiCollapsingHeader("Animation clips");
            container->pushBack(head);
        }

        auto viewport = new GuiViewport();
        viewport->setOwner(this);
        viewport->setSize(gui::fill(), gui::perc(100));
        viewport->addFlags(GUI_FLAG_SAME_LINE);
        pushBack(viewport);
        
        gpuGetPipeline()->initRenderTarget(&render_target);
        render_bucket = gpuRenderBucket(gpuGetPipeline(), 0);
        render_instance.render_target = &render_target;
        render_instance.render_bucket = &render_bucket;
        gfxm::vec3 cam_pos = gfxm::vec3(3, 1.5, 3);
        gfxm::mat4 view = gfxm::lookAt(cam_pos, gfxm::vec3(), gfxm::vec3(0, 1, 0));
        render_instance.view_transform = view;
        game_render_instances.insert(&render_instance);
        viewport->render_instance = &render_instance;

        gpuGetPipeline()->enableTechnique("Skybox", false);
        gpuGetPipeline()->enableTechnique("Posteffects/MotionBlur", false);
        gpuGetPipeline()->enableTechnique("Fog", false);
    }
public:
    GuiImportM3dWindow(const std::string& path) {
        addFlags(GUI_FLAG_BLOCKING);
        setSize(1200, 800);
        setPosition(800, 200);
        guiEnableUpdate(this);

        gizmo_ctx = gizmoCreateContext();

        std::filesystem::path fspath(path);
        std::string ext = fspath.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), [](char c) { return std::tolower(c); });
        if (ext == ".m3dp") {
            initFromProject(path);
            project_path = fspath.string();
        } else {
            initFromSource(path);
            fspath.replace_extension("m3dp");
            project_path = fspath.string();
        }

        initControls();

        {
            Mesh3d mesh_ram;
            meshGenerateCheckerPlane(&mesh_ram);
            ref_plane.mesh.setData(&mesh_ram);
            ref_plane.mesh.setDrawMode(MESH_DRAW_MODE::MESH_DRAW_TRIANGLES);
            ref_plane.material = ResourceManager::get()->create<gpuMaterial>("");
            ref_plane.material->setFragmentExtension(loadResource<gpuShaderSet>("core/shaders/modular/basic.frag"));
            ref_plane.material->compile();
            ref_plane.renderable.reset(new gpuRenderable);
            ref_plane.renderable->setMeshDesc(ref_plane.mesh.getMeshDesc());
            ref_plane.renderable->setMaterial(ref_plane.material.get());
            ref_plane.renderable->setRole(GPU_Role_Geometry);
            ref_plane.renderable->compile();
            ref_plane.transform_block = gpuGetDevice()->createParamBlock<gpuTransformBlock>();
        }
    }
    ~GuiImportM3dWindow() {
        gizmoReleaseContext(gizmo_ctx);
        guiDisableUpdate(this);
    }

    void onUpdate(float dt) override {
        // anim preview
        if(m3d_inst) {
            auto model = m3d_inst->getModel();
            if (model && !model->animations.empty()) {
                if (current_anim >= model->animations.size()) {
                    current_anim = current_anim % model->animations.size();
                }
                auto skl_inst = m3d_inst->getSkeletonInstance();
                auto& anim = model->animations[current_anim];
                animSampler sampler(skl_inst->getSkeletonMaster(), const_cast<Animation*>(anim.get()));
                animSampleBuffer buf;
                buf.init(skl_inst->getSkeletonMaster());
                sampler.sample(buf.data(), buf.count(), t_anim);
                buf.applySamples(skl_inst);
                t_anim += dt * anim->fps;
                if (t_anim > anim->length) {
                    ++current_anim;
                    t_anim -= anim->length;
                }
            }
        }

        // 
        render_bucket.add(ref_plane.renderable.get());
        if (m3d_inst) {
            m3d_inst->submit(&render_bucket);
        }

        // gizmos
        gizmoClearContext(gizmo_ctx);
        //gizmoCircle(gizmo_ctx, gfxm::mat4(1.f), .5f, 3.f, GIZMO_COLOR_RED);
        gizmoPushDrawCommands(gizmo_ctx, &render_bucket);
    }
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        }
        return GuiWindow::onMessage(msg, params);
    }
};