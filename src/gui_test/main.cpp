#include <Windows.h>

#include <assert.h>

#include <string>

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

#include "editor/csg_editor.hpp"
#include "editor/sequence_editor.hpp"

#include "import/import_skeletal_model.hpp"

#include "test/layout_test.hpp"


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
    pos -= wnd->size * .5f;
    wnd->setPosition(pos);
}

GuiWindow* tryOpenEditWindow(const std::string& ext, const std::string& spath) {
    GuiEditorWindow* wnd = editorFindEditorWindow(spath);
    if (wnd) {
        guiBringWindowToTop(wnd);
        guiSetActiveWindow(wnd);
        return wnd;
    }
    
    // TODO:
    if (ext == ".png") {
        wnd = dynamic_cast<GuiEditorWindow*>(guiCreateWindow<GuiCsgDocument>());
    }

    if (!wnd) {
        return 0;
    }
    guiAdd(0, 0, wnd);
    wnd->loadFile(spath);

    editorRegisterEditorWindow(spath, wnd);

    guiSetActiveWindow(wnd);
    return wnd;
}
GuiWindow* tryOpenImportWindow(const std::string& ext, const std::string& spath) {
    GuiImportWindow* wnd = 0;
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
        if (type == "SkeletalModel") {
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

    if (ext == ".fbx" || ext == ".obj" || ext == ".dae" || ext == ".3ds") {
        wnd = dynamic_cast<GuiImportWindow*>(new GuiImportFbxWnd());
    }

    if (!wnd) {
        return 0;
    }
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
        std::experimental::filesystem::path fpath(spath);
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
bool dropFileCb(const std::experimental::filesystem::path& path) {
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

class GuiTestWindow2 : public GuiWindow {
public:
    GuiTestWindow2()
        : GuiWindow("Hello") {

        {
            auto header = new GuiCollapsingHeader("Component", false, true, 0);
            guiAdd(this, this, header);
            guiAdd(header, this, new GuiComboBox());
        }

        auto elem = new GuiElement();
        guiAdd(this, this, elem);
        elem->size = gfxm::vec2(0, 0);
        elem->overflow = GUI_OVERFLOW_FIT;

        guiAdd(elem, this, new GuiLabel("Hello, World!"), GUI_FLAG_FRAME | GUI_FLAG_PERSISTENT);

        {
            auto header = new GuiCollapsingHeader("Component", false, true, 0);
            guiAdd(elem, this, header);
            guiAdd(header, this, new GuiInputText());
        }
        {
            auto header = new GuiCollapsingHeader("Component", false, true, 0);
            guiAdd(elem, this, header);
            guiAdd(header, this, new GuiInputFloat3("Translation"));
            guiAdd(header, this, new GuiInputFloat3("Rotation"));
            guiAdd(header, this, new GuiInputFloat3("Scale"));
        }
        
        guiAdd(elem, this, new GuiButton("Press me"));
    }
};


#include "audio/audio_mixer.hpp"
#include "audio/res_cache_audio_clip.hpp"
inline void audioInit() {
    resAddCache<AudioClip>(new resCacheAudioClip);
    audio().init(44100, 16);
}
inline void audioCleanup() {
    audio().cleanup();
}


#include "gui_cdt_test_window.hpp"
int main(int argc, char* argv) {
    reflectInit();
    platformInit(true, true);
    gpuInit(new build_config::gpuPipelineCommon());
    typefaceInit();

    Font* fnt = fontGet("fonts/ProggyClean.ttf", 16, 72);
    guiInit(fnt);
    guiSetMessageCallback(&messageCb);
    guiSetDropFileCallback(&dropFileCb);

    resInit();
    animInit();
    audioInit();

    int screen_width = 0, screen_height = 0;
    platformGetWindowSize(screen_width, screen_height);

    std::unique_ptr<GuiDockSpace> dock_space;
    dock_space.reset(new GuiDockSpace());
    dock_space->pos = gfxm::vec2(0.0f, 0.0f);
    dock_space->size = gfxm::vec2(screen_width, screen_height);

    auto wnd = new GuiWindow("1 Test window");
    wnd->pos = gfxm::vec2(120, 160);
    wnd->size = gfxm::vec2(640, 700);
    guiAdd(0, 0, wnd);
    guiAdd(wnd, wnd, new GuiTextBox());
    guiAdd(wnd, wnd, new GuiImage(resGet<gpuTexture2d>("1648920106773.jpg").get()));
    guiAdd(wnd, wnd, new GuiButton());
    guiAdd(wnd, wnd, new GuiButton());
    auto wnd2 = new GuiWindow("2 Other test window");
    wnd2->pos = gfxm::vec2(850, 200);
    wnd2->size = gfxm::vec2(320, 800);
    guiAdd(0, 0, wnd2);
    guiAdd(wnd2, wnd2, new GuiImage(resGet<gpuTexture2d>("effect_004.png").get()));
    auto wnd_demo = new GuiDemoWindow();
    guiAdd(0, 0, wnd_demo);
    auto wnd_explorer = new GuiFileExplorerWindow();
    guiAdd(0, 0, wnd_explorer);
    auto wnd_nodes = new GuiNodeEditorWindow();
    guiAdd(0, 0, wnd_nodes);
    auto wnd_cdt = new GuiCdtTestWindow();
    guiAdd(0, 0, wnd_cdt);
    auto wnd_seq = guiCreateWindow<GuiSequenceDocument>();
    guiAdd(0, 0, wnd_seq);
    auto wnd_csg2 = new GuiCsgDocument;
    guiAdd(0, 0, wnd_csg2);

    auto wnd_layout = new GuiLayoutTestWindow();
    guiAdd(0, 0, wnd_layout);
    guiAddManagedWindow(wnd_layout);

    guiAdd(0, 0, new GuiTestWindow2);

    guiGetRoot()->createMenuBar()
        ->addItem(new GuiMenuItem("File", {
                new GuiMenuListItem("New", {
                    new GuiMenuListItem("Scene"),
                    new GuiMenuListItem("Actor"),
                    new GuiMenuListItem("Animation Sequence", [&dock_space]() {
                        auto wnd = guiCreateWindow<GuiSequenceDocument>();
                        guiAdd(0, 0, wnd);
                        auto n = dock_space->findNode("EditorSpace");
                        if (n) {
                            n->addWindow(wnd);
                        }
                    }),
                    new GuiMenuListItem("CSG Scene")
                }),
                new GuiMenuListItem("Open..."),
                new GuiMenuListItem("Save"),
                new GuiMenuListItem("Save As..."),
                new GuiMenuListItem("Exit")
        }))
        ->addItem(new GuiMenuItem("Edit"))
        ->addItem(new GuiMenuItem("View"))
        ->addItem(new GuiMenuItem("Settings"));

    dock_space->getRoot()->setId("EditorSpace");
    dock_space->getRoot()->setLocked(true);
    dock_space->getRoot()->addWindow(wnd);
    dock_space->getRoot()->addWindow(wnd2);
    dock_space->getRoot()->addWindow(wnd_nodes);
    dock_space->getRoot()->addWindow(wnd_cdt);
    dock_space->getRoot()->addWindow(wnd_seq);
    dock_space->getRoot()->addWindow(wnd_csg2);
    dock_space->getRoot()->splitLeft();
    dock_space->getRoot()->left->setId("Sidebar");
    dock_space->getRoot()->left->setLocked(true);
    dock_space->getRoot()->left->addWindow(wnd_demo);
    dock_space->getRoot()->left->addWindow(wnd_explorer);
    dock_space->getRoot()->split_pos = 0.20f;
    dock_space->getRoot()->right->split_pos = 0.3f;

    gpuUniformBuffer* ubufCam3d = gpuGetPipeline()->createUniformBuffer(UNIFORM_BUFFER_CAMERA_3D);
    gpuGetPipeline()->attachUniformBuffer(ubufCam3d);    
    

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
            inst->world.update(g_dt);
            
            //render_bucket.add(renderable_plane.get());
            inst->world.getRenderScene()->draw(inst->render_bucket);
            ubufCam3d->setMat4(
                gpuGetPipeline()->getUniformBufferDesc(UNIFORM_BUFFER_CAMERA_3D)->getUniform("matProjection"),
                inst->projection
            );
            ubufCam3d->setMat4(
                gpuGetPipeline()->getUniformBufferDesc(UNIFORM_BUFFER_CAMERA_3D)->getUniform("matView"),
                inst->view_transform
            );
            gpuDraw(
                inst->render_bucket, inst->render_target,
                inst->view_transform,
                inst->projection
            );
            
            inst->render_target->bindFrameBuffer("Normal", 0);
            dbgDrawDraw(
                inst->projection,
                inst->view_transform
            );
            inst->render_bucket->clear();
        }
        dbgDrawClearBuffers();

        guiRender();

        platformSwapBuffers();
        g_dt = timer_.stop();
    }

    audioCleanup();
    resCleanup();
    animCleanup();

    dock_space.reset();
    guiCleanup();
    typefaceCleanup();
    gpuCleanup();
    platformCleanup();
    return 0;
}