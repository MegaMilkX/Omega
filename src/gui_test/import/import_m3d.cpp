#include "import_m3d.hpp"

#include "mesh3d/generate_primitive.hpp"

#include "gui_engine/inspector.hpp"


// TODO: REMOVE THIS
extern std::set<GameRenderInstance*> game_render_instances;


void GuiImportM3dWindow::initFromSource(const std::string& path) {
    m3d_proj.initFromSource(path);
    m3d = ResourceManager::get()->create<m3dModel>("imported_model");
    m3d_proj.import(*m3d.get());
    m3d_inst.reset(new m3dSkeletalInstance);
    m3d_inst->init(m3d);
}
void GuiImportM3dWindow::initFromProject(const std::string& path) {
    m3d_proj.load(path);
    m3d = ResourceManager::get()->create<m3dModel>("imported_model");
    m3d_proj.import(*m3d.get());
    m3d_inst.reset(new m3dSkeletalInstance);
    m3d_inst->init(m3d);
}

void GuiImportM3dWindow::initControls() {
    auto container = new GuiElement();
    container->setSize(gui::perc(40), gui::perc(100));
    container->setStyleClasses({ "fbx-import-container" });
    pushBack(container);

    {
        auto toolbar = container->pushBack(guiCreate<GuiElement>());
        toolbar->primary_axis = GUI_PRIMARY_AXIS::X;
        toolbar->setSize(gui::fill(), gui::content());
        toolbar->setStyleClasses({ "container" });

        auto btn_import = new GuiButton(
            "Import",
            guiLoadIcon("svg/Entypo/arrow-bold-down.svg")            
        );
        btn_import->subscribe<GuiEvt_LClick>([this](const GuiEvt_LClick&) {
            m3d_proj.save_m3d();
            // TMP: instantly check loading
            loadResource<m3dModel>(
                std::filesystem::path(m3d_proj.source_path).replace_extension().string()
            );
        });
        auto btn_save = new GuiButton(
            "Save project",
            guiLoadIcon("svg/Entypo/save.svg")            
        );
        btn_save->subscribe<GuiEvt_LClick>([this](const GuiEvt_LClick&) {
            m3d_proj.save(project_path);
        });
        toolbar->pushBack(btn_import);
        toolbar->pushBack(btn_save);
    }

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

        auto combo_mode = new GuiComboBox("import mode");
        combo_mode->addItem("Embedded", 0);
        combo_mode->addItem("Separate file", 1);
        combo_mode->addItem("External file", 2);
        head->pushBack(combo_mode);

        auto inp_res_id = new GuiInputString("resource id");
        inp_res_id->setValue(m3d_proj.out_skeleton_resource_id);
        head->pushBack(inp_res_id);
    }

    {
        auto head = new GuiCollapsingHeader("Materials");
        container->pushBack(head);

        auto inspector = guiCreate<GuiInspector>();
        inspector->setSize(gui::fill(), gui::content());

        auto mat_list = head->pushBack(guiCreate<GuiTreeView>());
        mat_list->clearChildren();
        for (int i = 0; i < m3d_proj.materials.size(); ++i) {
            // TODO: NAME
            std::string name = m3d_proj.material_names[i];
            auto item = mat_list->addItem(name.c_str());

            gpuMaterial* mat = m3d_proj.materials[i].get();
            rtti::PropSnapshot* snap = &m3d_proj.material_snaps[i];
            rtti::PropSnapshot* snap_delta = &m3d_proj.material_deltas[i];
            item->subscribe<GuiEvt_Selected>([this, inspector, mat, snap, snap_delta](const GuiEvt_Selected& e) {
                e.invoke_next();
                inspector->init(mat, snap, snap_delta);
            });
        }

        head->pushBack(inspector);
    }

    {
        auto head = new GuiCollapsingHeader("Animation clips");
        container->pushBack(head);
    }

    auto viewport = new GuiViewport();
    viewport->setOwner(this);
    viewport->setSize(gui::fill(), gui::perc(100));
    pushBack(viewport);

    render_instance.render_view = gpuGetPipeline()->createOffscreenView(RendererType::Default, 640, 480);
    gfxm::vec3 cam_pos = gfxm::vec3(3, 1.5, 3);
    gfxm::mat4 view = gfxm::lookAt(cam_pos, gfxm::vec3(), gfxm::vec3(0, 1, 0));
    render_instance.render_view->setView(view);
    game_render_instances.insert(&render_instance);
    viewport->render_instance = &render_instance;

    gpuGetPipeline()->enableTechnique("Skybox", false);
    gpuGetPipeline()->enableTechnique("Posteffects/MotionBlur", false);
    gpuGetPipeline()->enableTechnique("Fog", false);
}

GuiImportM3dWindow::GuiImportM3dWindow(const std::string& path) {
    primary_axis = GUI_PRIMARY_AXIS::X;
    addFlags(GUI_FLAG_BLOCKING);
    setSize(1200, 800);
    setPosition(800, 200);

    guiScheduleTick(this, .0f, GUI_TICK_UPDATE_CONTENT);

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
GuiImportM3dWindow::~GuiImportM3dWindow() {
    gizmoReleaseContext(gizmo_ctx);
}

void GuiImportM3dWindow::onTick(float dt, GUI_TICK_ID id) {
    if (id != GUI_TICK_UPDATE_CONTENT) {
        return;
    }

    guiScheduleTick(this, .0f, GUI_TICK_UPDATE_CONTENT);

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
    render_instance.render_view->getRenderBucket()->add(ref_plane.renderable.get());
    if (m3d_inst) {
        m3d_inst->submit(render_instance.render_view->getRenderBucket());
    }

    // gizmos
    gizmoClearContext(gizmo_ctx);
    //gizmoCircle(gizmo_ctx, gfxm::mat4(1.f), .5f, 3.f, GIZMO_COLOR_RED);
    gizmoPushDrawCommands(gizmo_ctx, render_instance.render_view->getRenderBucket());
}

