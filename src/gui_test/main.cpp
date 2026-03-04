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

#include "reflect.hpp"

#include "gpu/gpu.hpp"
#include "gui/gui.hpp"
#include "typeface/font.hpp"
#include "resource/resource.hpp"
#include "input/input.hpp"
#include "mesh3d/generate_primitive.hpp"

#include "skeletal_model/skeletal_model.hpp"
#include "world/world.hpp"
#include "world/node/node_character_capsule.hpp"
#include "world/node/node_skeletal_model.hpp"
#include "world/component/components.hpp"
#include "world/controller/actor_controllers.hpp"

#include "gui/elements/viewport/gui_viewport.hpp"

#include "editor/animator_editor.hpp"
#include "editor/sequence_editor.hpp"
#include "editor/csg_editor.hpp"
#include "editor/scene_editor.hpp"

#include "import/import_skeletal_model.hpp"

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
    GuiImportWindow* wnd = 0;
    std::string ext = ext_;
    std::transform(ext.begin(), ext.end(), ext.begin(),
        [](unsigned char c) {
            return std::tolower(c);
        }
    );

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
    }

    if (ext == ".fbx" || ext == ".obj" || ext == ".dae" || ext == ".3ds" || ext == ".gltf") {
        wnd = static_cast<GuiImportWindow*>(new GuiImportFbxWnd());
    }

    if (!wnd) {
        return 0;
    }
    //guiGetRootHost()->insert(wnd);
    guiAdd(0, 0, wnd);

    wnd->createImport(spath);

    guiAddManagedWindow(wnd);
    guiSetActiveWindow(wnd);
    guiCenterWindowToParent(wnd);
    return wnd;
}

bool messageCb(GUI_MSG msg, GUI_MSG_PARAMS params) {
    //LOG(guiMsgToString(msg));
    switch (msg) {
    case GUI_MSG::FILE_EXPL_OPEN_FILE: {
        std::string spath = params.getA<GuiFileListItem*>()->path_canonical;
        LOG_WARN(spath);
        std::filesystem::path fpath(spath);
        if (!fpath.has_extension()) {
            // TODO: Show a message box with a warning, don't open file
            return true;
        }
        std::string ext = fpath.extension().string();
        if (tryOpenEditWindow(ext, fpath.string())) {
            return true;
        }
        if (tryOpenImportWindow(ext, fpath.string())) {
            return true;
        }
#if defined _WIN32
        ShellExecuteA(0, 0, fpath.string().c_str(), 0, 0, SW_SHOW);
#endif
        return true;
    }
    };
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

void buildPropertyUI(GuiElement* elem, MetaObject* object, type t) {
    for (int i = 0; i < t.prop_count(); ++i) {
        auto prop = t.get_prop(i);
        auto prop_type = prop->t;

        if (prop_type == type_get<float>()) {
            auto gui_input = new GuiInputNumeric(prop->name.c_str());
            elem->pushBack(gui_input);
            gui_input->setValue(prop->getValue<float>(object));
            gui_input->on_change = [object, prop](float value) {
                prop->setValue(object, &value);
            };
        } else if (prop_type == type_get<gfxm::vec2>()) {
            auto gui_input = new GuiInputNumeric2(prop->name.c_str());
            elem->pushBack(gui_input);
            gfxm::vec2 v2 = prop->getValue<gfxm::vec2>(object);
            gui_input->setValue(v2.x, v2.y);
            gui_input->on_change = [object, prop](float x, float y) {
                gfxm::vec2 v2(x, y);
                prop->setValue(object, &v2);
            };
        } else if (prop_type == type_get<gfxm::vec3>()) {
            auto gui_input = new GuiInputNumeric3(prop->name.c_str());
            elem->pushBack(gui_input);
            gfxm::vec3 v3 = prop->getValue<gfxm::vec3>(object);
            gui_input->setValue(v3.x, v3.y, v3.z);
            gui_input->on_change = [object, prop](float x, float y, float z) {
                gfxm::vec3 v3(x, y, z);
                prop->setValue(object, &v3);
            };
        } else if (prop_type == type_get<gfxm::vec4>()) {
            auto gui_input = new GuiInputNumeric4(prop->name.c_str());
            elem->pushBack(gui_input);
            gfxm::vec4 v4 = prop->getValue<gfxm::vec4>(object);
            gui_input->setValue(v4.x, v4.y, v4.z, v4.w);
            gui_input->on_change = [object, prop](float x, float y, float z, float w) {
                gfxm::vec4 v4(x, y, z, w);
                prop->setValue(object, &v4);
            };
        } else if (prop_type == type_get<gfxm::quat>()) {
            auto gui_input = new GuiInputNumeric4(prop->name.c_str());
            elem->pushBack(gui_input);
            gfxm::quat q = prop->getValue<gfxm::quat>(object);
            gui_input->setValue(q.x, q.y, q.z, q.w);
            gui_input->on_change = [object, prop](float x, float y, float z, float w) {
                gfxm::quat q(x, y, z, w);
                prop->setValue(object, &q);
            };
        } else if (prop_type == type_get<std::string>()) {
            auto gui_input = new GuiInputString(prop->name.c_str());
            elem->pushBack(gui_input);
            std::string str = prop->getValue<std::string>(object);
            gui_input->setValue(str);
            gui_input->on_change = [object, prop](const std::string& str) {
                prop->setValue(object, (void*)&str);
            };
        } else {
            elem->pushBack(new GuiLabel(std::format("[NO GUI] {}", prop->name.c_str()).c_str()));
        }
    }
}
void buildActorTreeUI(GuiElement* elem, ActorNode* node) {
    if (!node) {
        return;
    }
    
    auto item = new GuiTreeItem(std::format("{} [{}]", node->getName(), node->get_type().get_name()).c_str());
    item->setCollapsed(false);
    item->user_ptr = (void*)node;
    elem->pushBack(item);
    /*
    item->on_click = [](GuiTreeItem* item) {
        ActorNode* node = (ActorNode*)item->user_ptr;
        auto type = node->get_type();
        node_props->clearChildren();

        fn_buildProps(node_props, node, type);
    };*/

    item->clearChildren();
    for (int i = 0; i < node->childCount(); ++i) {
        buildActorTreeUI(item, node->getChild(i));
    }
}
void initActorInspector(GuiElement* elem, Actor* actor) {
    elem->clearChildren();
    
    elem->pushBack("Nodes");
    auto tree_view = new GuiTreeView();
    tree_view->clearChildren();
    elem->pushBack(tree_view);
    tree_view->clearChildren();
    buildActorTreeUI(tree_view, actor->getRoot());
    
    elem->pushBack("Drivers");
    for (int i = 0; i < actor->driverCount(); ++i) {
        auto drv = actor->getDriver(i);
        auto type = drv->get_type();
        GuiCollapsingHeader* header = new GuiCollapsingHeader(type.get_name());
        elem->pushBack(header);
        header->setOpen(true);
        buildPropertyUI(header, drv, type);
    }
}

int main(int argc, char* argv) {
    cppiReflectInit();

    reflectInit();
    platformInit(true, true);
    gpuInit();

    std::shared_ptr<Font> fnt = fontGet("fonts/ProggyClean.ttf", 16, 72);
    guiInit(fnt);
    guiSetMessageCallback(&messageCb);
    guiSetDropFileCallback(&dropFileCb);

    gui::style guistyle;
    gui::style_sheet sheet;
    sheet.select_styles(&guistyle, { "control", "collapsing-header" });
    guistyle.dbg_print();

    resInit();
    animInit();
    audioInit();

    int screen_width = 0, screen_height = 0;
    platformGetWindowSize(screen_width, screen_height);
    
    //auto wnd_demo = new GuiDemoWindow;
    //guiGetRoot()->pushBack(wnd_demo);
    auto wnd_inspector = new GuiWindow();
    auto wnd_explorer = new GuiFileExplorerWindow();
    guiGetRoot()->pushBack(wnd_explorer);
    auto wnd_viewport = new GuiSceneDocument();
    
    guiGetRoot()->getMenuBar()
        ->addItem(new GuiMenuItem("File", {
                new GuiMenuListItem("New", {
                    new GuiMenuListItem("Animator", []() {
                        auto wnd = guiCreateWindow<GuiAnimatorDocument>();
                        guiAdd(0, 0, wnd);
                        guiGetRoot()->getDockSpace()->insert("EditorSpace", wnd);
                    }),
                    new GuiMenuListItem("Animation Sequence", []() {
                        auto wnd = guiCreateWindow<GuiSequenceDocument>();
                        guiAdd(0, 0, wnd);
                        guiGetRoot()->getDockSpace()->insert("EditorSpace", wnd);
                    }),
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
                new GuiMenuListItem("Save"),
                new GuiMenuListItem("Save As..."),
                new GuiMenuListItem("Exit")
        }))
        ->addItem(new GuiMenuItem("Edit"))
        ->addItem(new GuiMenuItem("View"))
        ->addItem(new GuiMenuItem("Settings"));
    
    auto dock_space = guiGetRoot()->getDockSpace();
    auto dock_root = dock_space->getRoot();
    dock_root->setMode(GUI_DOCK_NODE_SINGLE);
    dock_root->setId("EditorSpace");
    dock_root->setLocked(true);
    dock_root = dock_root->splitLeft();
    dock_root->left->setId("Sidebar");
    dock_root->left->setLocked(true);
    dock_root->left->addWindow(wnd_inspector);
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


    auto prefab = loadResource<ActorPrefab>("actors/character");
    auto actor = prefab->instantiate();
    initActorInspector(wnd_inspector, actor);
    wnd_viewport->viewport.render_instance->world.spawn(actor);


    timer timer_;
    while (platformIsRunning()) {
        timer_.start();
        platformPollMessages();
        inputUpdate(g_dt);
        
        gpuFrameBufferUnbind();

        guiPollMessages();
        guiLayout();
        guiDraw();

        // Process and render world instances
        for(auto& inst : game_render_instances) {
            inst->world.update(.0f/*g_dt*/);
            
            //render_bucket.add(renderable_plane.get());
            inst->world.getRenderScene()->draw(inst->render_bucket);
            if(inst->gizmo_ctx) {
                gizmoPushDrawCommands(inst->gizmo_ctx.get(), inst->render_bucket);
            }
            DRAW_PARAMS params = {
                .view = inst->view_transform,
                .projection = inst->projection,
                .vp_rect_ratio = gfxm::rect(0, 0, 1, 1),
                .viewport_x = 0,
                .viewport_y = 0,
                .viewport_width = inst->render_target->getWidth(),
                .viewport_height = inst->render_target->getHeight()
            };
            gpuDraw(inst->render_bucket, inst->render_target, params);
            /*
            inst->render_target->bindFrameBuffer("Default");
            dbgDrawDraw(
                inst->projection,
                inst->view_transform,
                0, 0, inst->render_target->getWidth(), inst->render_target->getHeight()
            );*/
            inst->render_bucket->clear();
            if(inst->gizmo_ctx) {
                gizmoClearContext(inst->gizmo_ctx.get());
            }
        }
        dbgDrawClearBuffers();

        guiRender();

        platformSwapBuffers();
        g_dt = timer_.stop();
    }

    audioCleanup();
    resCleanup();
    animCleanup();

    guiCleanup();
    gpuCleanup();
    platformCleanup();
    return 0;
}