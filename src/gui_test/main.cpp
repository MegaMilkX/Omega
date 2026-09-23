#include <Windows.h>

#include <assert.h>

#include <string>

#include "gui_test_reflect.auto.hpp"

#include "platform/gl/glextutil.h"
#include "log/log.hpp"
#include "util/timer.hpp"

#include <unordered_map>
#include <memory>

#include "math/intersection.hpp"

#include "gpu/gpu.hpp"
#include "gui/gui.hpp"
#include "typeface/font.hpp"
#include "resource/resource.hpp"
#include "input/input.hpp"
#include "mesh3d/generate_primitive.hpp"

#include "skeletal_model/skeletal_model.hpp"
#include "world/world.hpp"
#include "world/node/node_character_capsule.hpp"
#include "world/component/components.hpp"
#include "world/controller/actor_controllers.hpp"

#include "gui/elements/viewport/gui_viewport.hpp"

#include "editor/csg_editor.hpp"
#include "editor/scene_editor.hpp"

#include "import/import_skeletal_model.hpp"
#include "import/import_m3d.hpp"

#include "test/layout_test.hpp"

// TODO: REMOVE THIS !!!
#include "resource_cache/resource_cache.hpp"
#include "static_model/static_model.hpp"

// TODO: REMOVE, temporary fix for this project to not overwrite resource_ref.auto.hpp with lacking data
#include "world/controller/character_controller.hpp"
#include "world/controller/material_controller.hpp"
#include "world/component/skeleton_component.hpp"
#include "world/node/skeleton_node.hpp"
#include "world/node/anim_machine_node.hpp"
#include "world/node/render_proxy_node.hpp"

#include "gui_engine/actor_inspector.hpp"


std::set<GameRenderInstance*> game_render_instances;
static float g_dt = 1.0f / 60.0f;


void guiCenterWindowToParent(GuiWindow* wnd) {
    auto parent = wnd->getParent();
    assert(parent);
    if (!parent) {
        return;
    }
    gfxm::rect rc = parent->getClientArea();
    gfxm::vec2 pos = (rc.min + rc.max) * .5f;
    // TODO: FIX UNITS
    pos.x -= wnd->size.x.value * .5f;
    pos.y -= wnd->size.y.value * .5f;
    wnd->setPosition(pos.x, pos.y);
}

//std::unique_ptr<GuiDockSpace> dock_space;
GuiWindow* tryOpenEditWindow(const std::string& ext, const std::string& spath) {
    GuiEditorWindow* wnd = editorFindEditorWindow(spath);
    if (wnd) {
        guiBringWindowToTop(wnd);
        guiSetActiveWindow(wnd);
        return wnd;
    }
    
    if (ext == ".csg") {
        GuiCsgDocument* doc = new GuiCsgDocument;
        if (doc->open(spath)) {
            wnd = doc;
        } else {
            delete doc;
        }
    }

    if (!wnd) {
        return 0;
    }
    guiAdd(0, 0, wnd);
    wnd->loadFile(spath);
    guiGetRoot()->getDockSpace()->insert("EditorSpace", wnd);

    guiAddManagedWindow(wnd);
    editorRegisterEditorWindow(spath, wnd);

    guiSetActiveWindow(wnd);
    return wnd;
}
GuiWindow* tryOpenImportWindow(const std::string& ext_, const std::string& spath) {
    GuiWindow* wnd = 0;
    std::string ext = ext_;
    std::transform(ext.begin(), ext.end(), ext.begin(),
        [](unsigned char c) {
            return std::tolower(c);
        }
    );
    /*
    if (ext == ".import") {
        nlohmann::json j;
        std::ifstream f(spath);
        if (!f.is_open()) {
            return 0;
        }
        j << f;
        f.close();

        auto jtype = j["type"];
        if (!jtype.is_string()) {
            LOG_ERR("Import file 'type' field must be a string");
            return 0;
        }
        std::string type = jtype.get<std::string>();
        if (type == "3DModel") {
            wnd = dynamic_cast<GuiImportWindow*>(new GuiImportFbxWnd());
        } else {
            LOG_ERR("Unknown import type " << type);
            return 0;
        }
        if (!wnd) {
            LOG_ERR("Import window was not created");
            return 0;
        }
        guiAdd(0, 0, wnd);

        wnd->loadImport(spath);
        guiAddManagedWindow(wnd);
        guiSetActiveWindow(wnd);
        guiCenterWindowToParent(wnd);
        return wnd;
    }*/

    if (ext == ".m3dp"
        || ext == ".fbx"
        || ext == ".obj"
        || ext == ".dae"
        || ext == ".3ds"
        || ext == ".gltf"
        || ext == ".glb"
    ) {
        //wnd = static_cast<GuiImportWindow*>(new GuiImportFbxWnd());
        wnd = new GuiImportM3dWindow(spath);
    }

    if (!wnd) {
        return 0;
    }
    //guiGetRootHost()->insert(wnd);
    guiAdd(0, 0, wnd);

    //wnd->createImport(spath);

    guiAddManagedWindow(wnd);
    guiSetActiveWindow(wnd);
    guiCenterWindowToParent(wnd);
    return wnd;
}

void fileCb(const GuiEvt_FileConfirmed& e) {
    std::string spath = e.files[0];
    LOG_WARN(spath);
    std::filesystem::path fpath(spath);
    if (!fpath.has_extension()) {
        // TODO: Show a message box with a warning, don't open file
        return;
    }
    std::string ext = fpath.extension().string();
    if (tryOpenEditWindow(ext, fpath.string())) {
        return;
    }
    if (tryOpenImportWindow(ext, fpath.string())) {
        return;
    }
#if defined _WIN32
    ShellExecuteA(0, 0, fpath.string().c_str(), 0, 0, SW_SHOW);
#endif
}
bool messageCb(GUI_MSG msg, GUI_MSG_PARAMS params) {
    //LOG(guiMsgToString(msg));
    return false;
}
bool dropFileCb(const std::filesystem::path& path) {
    std::string spath = path.string();
    LOG_DBG(spath);
    
    if (!path.has_extension()) {
        return false;
    }

    std::string ext = path.extension().string();
    LOG_DBG(ext);

    if (tryOpenEditWindow(ext, spath)) {
        return true;
    }
    if (tryOpenImportWindow(ext, spath)) {
        return true;
    }
    return false;
}


int main(int argc, char* argv) {
    cppiReflectInit();

    platformInit(true, true);
    gpuInit();

    std::shared_ptr<Font> fnt = fontGet("fonts/ProggyClean.ttf", 16, 72);
    guiInit(fnt);
    guiGetRoot()->subscribe<GuiEvt_FileConfirmed>(fileCb);
    guiSetMessageCallback(&messageCb);
    guiSetDropFileCallback(&dropFileCb);

    gui::style guistyle;
    gui::style_sheet sheet;
    sheet.select_styles(&guistyle, { "control", "collapsing-header" });
    guistyle.dbg_print();

    resInit();
    audioInit();

    int screen_width = 0, screen_height = 0;
    platformGetWindowSize(screen_width, screen_height);
    /*
    auto wnd_demo = new GuiDemoWindow;
    guiGetRoot()->pushBack(wnd_demo);
    */

    auto inspector = guiCreate<GuiActorInspector>();

    auto wnd_explorer = guiCreate<GuiFileExplorer>(
        GuiFileExplorerParams{
            .mode = GuiFileExplorerModeBrowse,
            .filters = {}
        }
    );
    guiGetRoot()->pushBack(wnd_explorer);
    auto wnd_viewport = guiCreate<GuiSceneDocument>(inspector);
    
    guiGetRoot()->getMenuBar()
        ->addItem(new GuiMenuItem("File", {
                new GuiMenuListItem("New", {
                    new GuiMenuListItem("CSG Scene", []() {
                        auto wnd = guiCreateWindow<GuiCsgDocument>();
                        guiAdd(0, 0, wnd);
                        guiGetRoot()->getDockSpace()->insert("EditorSpace", wnd);
                    }),
                    new GuiMenuListItem("Scene", [](){
                        auto wnd = guiCreateWindow<GuiSceneDocument>();
                        guiAdd(0, 0, wnd);
                        guiGetRoot()->getDockSpace()->insert("EditorSpace", wnd);
                    })
                }),
                new GuiMenuListItem("Open..."),
                new GuiMenuListItem("SUBMENU TEST", {
                    new GuiMenuListItem("Hello"),
                    new GuiMenuListItem("World")
                }),
                new GuiMenuListItem("Save"),
                new GuiMenuListItem("Save As..."),
                new GuiMenuListItem("Exit")
        }))
        ->addItem(new GuiMenuItem("Edit", {
                new GuiMenuListItem("Undo"),
                new GuiMenuListItem("Redo"),
                new GuiMenuListItem("Copy"),
                new GuiMenuListItem("Cut"),
                new GuiMenuListItem("Paste")
        }))
        ->addItem(new GuiMenuItem("View", {
                new GuiMenuListItem("Preferences"),
                new GuiMenuListItem("ABC"),
                new GuiMenuListItem("CDB"),
                new GuiMenuListItem("QWE"),
                new GuiMenuListItem("QAZ")
        }))
        ->addItem(new GuiMenuItem("Settings"));
    
    auto dock_space = guiGetRoot()->getDockSpace();
    auto dock_root = dock_space->getRoot();
    dock_root->setMode(GUI_DOCK_NODE_MULTIPLE);
    dock_root->setId("EditorSpace");
    dock_root->setLocked(true);
    dock_root = dock_root->splitLeft();
    dock_root->left->setId("Sidebar");
    dock_root->left->setLocked(true);
    dock_root->left->addWindow(inspector);
    dock_root->split_pos = 0.20f;
    dock_root->right->split_pos = 0.3f; 

    {
        auto n = dock_space->findNode("EditorSpace");
        n = n->splitBottom();
        n->split_pos = .65f;
        n->right->setId("Bottom");
    }

    dock_space->insert("EditorSpace", wnd_viewport);
    dock_space->insert("Bottom", wnd_explorer);


    timer timer_;
    while (platformIsRunning()) {
        timer_.start();
        platformPollMessages();
        TransformSystem::nextFrame();
        inputUpdate(g_dt);
        
        gpuFrameBufferUnbind();

        guiPollMessages();
        guiUpdate(g_dt);
        guiLayout();
        guiDraw();

        // Process and render world instances
        gpuTickMaterials(g_dt);

        for (int i = 0; i < gpuGetPipeline()->viewCount(); ++i) {
            EngineRenderView* rv = gpuGetPipeline()->getView(i);
            gpuGetPipeline()->drawSingleView(rv, .0f);
        }
        
        for(auto& inst : game_render_instances) {
            /*
            EngineRenderView* rv = inst->render_view;
            if(!rv) continue;

            gpuRenderer* renderer = rv->getRenderer();
            gpuRenderTarget* target = rv->getRenderTarget();
            gpuRenderBucket* bucket = rv->getRenderBucket();

            inst->world.update(.0f);
            
            //render_bucket.add(renderable_plane.get());
            inst->world.getRenderScene()->draw(bucket);
            if(inst->gizmo_ctx) {
                gizmoPushDrawCommands(inst->gizmo_ctx.get(), bucket);
            }
            DRAW_PARAMS params = {
                .view = rv->getViewTransform(),
                .view_prev = rv->getViewTransform(), // TODO: motion blur
                .projection = rv->getProjection(),
                .vp_rect_ratio = gfxm::rect(0, 0, 1, 1),
                .viewport_x = 0,
                .viewport_y = 0,
                .viewport_width = rv->getRenderTarget()->getWidth(),
                .viewport_height = rv->getRenderTarget()->getHeight()
            };
            renderer->draw(bucket, rv, params);
            */
            if(inst->gizmo_ctx) {
                gizmoClearContext(inst->gizmo_ctx.get());
            }
        }
        
        dbgDrawClearBuffers();

        guiRender();

        platformSwapBuffers();
        g_dt = timer_.stop();

        ResourceManager::get()->getBackend<gpuTexture>()->update();
    }

    audioCleanup();
    resCleanup();

    guiCleanup();
    gpuCleanup();
    platformCleanup();
    return 0;
}