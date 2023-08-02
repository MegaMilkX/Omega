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
    // TODO: FIX UNITS
    pos.x -= wnd->size.x.value * .5f;
    pos.y -= wnd->size.y.value * .5f;
    wnd->setPosition(pos.x, pos.y);
}

GuiWindow* tryOpenEditWindow(const std::string& ext, const std::string& spath) {
    GuiEditorWindow* wnd = editorFindEditorWindow(spath);
    if (wnd) {
        guiBringWindowToTop(wnd);
        guiSetActiveWindow(wnd);
        return wnd;
    }
    
    // TODO:
    /*
    if (ext == ".png") {
        wnd = dynamic_cast<GuiEditorWindow*>(guiCreateWindow<GuiCsgDocument>());
    }*/

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
        elem->setSize(0, 0);
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

class GuiAnimStateNode : public GuiElement {
public:
    GuiAnimStateNode() {
        setPosition(0, 0);
        setSize(250, 50);
        addFlags(GUI_FLAG_FLOATING);
        overflow = GUI_OVERFLOW_NONE;
    }
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) {
        switch (msg) {
        case GUI_MSG::PULL_START:
        case GUI_MSG::PULL_STOP: {
            return true;
        }
        case GUI_MSG::PULL: {
            pos.x.value += params.getA<float>();
            pos.y.value += params.getB<float>();
            return true;
        }
        case GUI_MSG::LCLICK: {
            notifyOwner(GUI_NOTIFY::STATE_NODE_CLICKED, this, 0);
            return true;
        }
        case GUI_MSG::DBL_LCLICK: {
            notifyOwner(GUI_NOTIFY::STATE_NODE_DOUBLE_CLICKED, this, 0);
            return true;
        }
        case GUI_MSG::MOVING: {
            gfxm::rect* prc = params.getB<gfxm::rect*>();
            gfxm::vec2 to = gfxm::vec2(prc->max.x, prc->max.y);
            gfxm::vec2 from = gfxm::vec2(prc->min.x, prc->min.y);
            gfxm::vec2 diff = to - from;
            // TODO: FIX UNITS
            pos.x.value += diff.x;
            pos.y.value += diff.y;
            return true;
        }
        }
        return false;
    }
    void onDraw() override {
        gfxm::vec2 shadow_offset(10.0f, 10.0f);
        guiDrawRectRoundBorder(gfxm::rect(
            rc_bounds.min + shadow_offset, rc_bounds.max + shadow_offset
        ), 15.0f, 10.0f, 0x00000000, 0xAA000000);
        guiDrawRectRound(rc_bounds, 15, GUI_COL_BUTTON);
        guiDrawText(client_area, "AnimStateNode", guiGetCurrentFont(), GUI_HCENTER | GUI_VCENTER, GUI_COL_TEXT);
    }
};
class GuiAnimStateGraphWindow : public GuiWindow {
    struct Connection {
        GuiAnimStateNode* from;
        GuiAnimStateNode* to;
    };

    GuiAnimStateNode* connection_preview_src = 0;
    std::vector<Connection> connections;

    bool connectionExists(GuiAnimStateNode* a, GuiAnimStateNode* b) {
        for (int i = 0; i < connections.size(); ++i) {
            auto& c = connections[i];
            if (c.from == a && c.to == b) {
                return true;
            }
        }
        return false;
    }
    bool makeConnection(GuiAnimStateNode* a, GuiAnimStateNode* b) {
        if (connectionExists(a, b)) {
            return false;
        }
        connections.push_back(Connection{ a, b });
        return true;
    }
public:
    GuiAnimStateGraphWindow()
        : GuiWindow("StateGraph") {
        addFlags(GUI_FLAG_DRAG_CONTENT);

        {
            auto ctx_menu = new GuiMenuList();
            ctx_menu->addItem(new GuiMenuListItem("Hello"));
            ctx_menu->addItem(new GuiMenuListItem("World!"));
            ctx_menu->addItem(new GuiMenuListItem("Foo"));
            ctx_menu->addItem(new GuiMenuListItem("Bar"));
            guiAddContextPopup(this, ctx_menu);
        }
        auto node = new GuiAnimStateNode;
        node->setPosition(100, 100);
        guiAdd(this, this, node);

        node = new GuiAnimStateNode;
        node->setPosition(150, 280);
        guiAdd(this, this, node);

        node = new GuiAnimStateNode;
        node->setPosition(250, 220);
        guiAdd(this, this, node);
    }
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::RCLICK:
        case GUI_MSG::DBL_RCLICK: {
            if (connection_preview_src) {
                connection_preview_src = 0;
                return true;
            }
            break;
        }
        case GUI_MSG::NOTIFY: {
            switch (params.getA<GUI_NOTIFY>()) {
            case GUI_NOTIFY::STATE_NODE_CLICKED: {
                if (connection_preview_src) {
                    auto node_b = params.getB<GuiAnimStateNode*>();
                    if (node_b == connection_preview_src) {
                        return true;
                    }
                    makeConnection(connection_preview_src, node_b);
                    connection_preview_src = 0;
                }
                return true;
            }
            case GUI_NOTIFY::STATE_NODE_DOUBLE_CLICKED: {
                connection_preview_src = params.getB<GuiAnimStateNode*>();
                return true;
            }
            }
            break;
        }
        }
        return GuiWindow::onMessage(msg, params);
    }
    void onDraw() override {
        for (int i = 0; i < connections.size(); ++i) {
            auto& c = connections[i];
            gfxm::vec2 pta = (c.from->getBoundingRect().min + c.from->getBoundingRect().max) * .5f;
            gfxm::vec2 ptb = (c.to->getBoundingRect().min + c.to->getBoundingRect().max) * .5f;
            gfxm::vec2 offs = gfxm::normalize(ptb - pta);
            std::swap(offs.x, offs.y);
            offs *= 10.f;
            offs.y = -offs.y;
            guiDrawLineWithArrow(
                pta + offs,
                ptb + offs,
                5.f, GUI_COL_TEXT
            );
        }
        if (connection_preview_src) {
            gfxm::rect rc_node = connection_preview_src->getBoundingRect();
            gfxm::vec2 ptsrc = (rc_node.min + rc_node.max) * .5f;
            gfxm::vec2 mouse = guiGetMousePos();
            gfxm::vec2 offs = gfxm::normalize(mouse - ptsrc);
            std::swap(offs.x, offs.y);
            offs *= 10.f;
            offs.y = -offs.y;
            guiDrawLineWithArrow(
                ptsrc + offs,
                mouse,
                5.f, GUI_COL_TIMELINE_CURSOR
            );
        }

        GuiWindow::onDraw();

        guiDrawText(client_area.min, "Double click on a node - start new connection", guiGetCurrentFont(), 0, GUI_COL_TEXT);
        guiDrawText(client_area.min + gfxm::vec2(.0f, 20.f), MKSTR("Pos content: " << pos_content.x << " " << pos_content.y).c_str(), guiGetCurrentFont(), 0, GUI_COL_TEXT);
    }
};

class GuiElement2 : public GuiElement {
public:
    GUI_BOX box;
    GuiElement2() {
        setSize(0, 0);
        box.setSize(0,0);

        GUI_BOX* header = new GUI_BOX;
        GUI_BOX* content = new GUI_BOX;
        //header->setSize(0, gui::em(2));
        header->setSize(-1, -1);
        header->setInnerText("Hello, World!");
        content->setSize(0, 0);
        content->setZOrder(1);
        box.addChild(header);
        box.addChild(content);
    }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        guiLayoutBox(&box, gfxm::rect_size(rc));
        guiLayoutPlaceBox(&box, rc.min);

        rc_bounds = box.rc;
        client_area = box.rc_content;
    }
    void onDraw() override {
        guiDbgDrawLayoutBox(&box);
    }
};
class GuiBoxWindow : public GuiWindow {
public:
    GuiBoxWindow()
        : GuiWindow("Boxes") {
        guiAdd(this, this, new GuiElement2);
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
    cppiReflectInit();

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
    dock_space->setPosition(0, 0);
    dock_space->setSize(0, 0);
    
    auto wnd = new GuiWindow("1 Test window");
    wnd->setPosition(120, 160);
    wnd->setSize(640, 700);
    guiAdd(0, 0, wnd);
    guiAdd(wnd, wnd, new GuiTextBox());
    guiAdd(wnd, wnd, new GuiImage(resGet<gpuTexture2d>("1648920106773.jpg").get()));
    guiAdd(wnd, wnd, new GuiButton());
    guiAdd(wnd, wnd, new GuiButton());
    auto wnd2 = new GuiWindow("2 Other test window");
    wnd2->setPosition(850, 200);
    wnd2->setSize(320, 800);
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
    auto wnd_state_graph = new GuiAnimStateGraphWindow;
    guiAdd(0, 0, wnd_state_graph);
    
    auto wnd_layout = new GuiLayoutTestWindow();
    guiAdd(0, 0, wnd_layout);
    guiAddManagedWindow(wnd_layout);

    guiAdd(0, 0, new GuiTestWindow2);

    guiAdd(0, 0, new GuiBoxWindow);
    
    guiGetRoot()->createMenuBar()
        ->addItem(new GuiMenuItem("File", {
                new GuiMenuListItem("New", {
                    new GuiMenuListItem("Animator", [&dock_space]() {
                        auto wnd = guiCreateWindow<GuiAnimatorDocument>();
                        guiAdd(0, 0, wnd);
                        dock_space->insert("EditorSpace", wnd);
                    }),
                    new GuiMenuListItem("Animation Sequence", [&dock_space]() {
                        auto wnd = guiCreateWindow<GuiSequenceDocument>();
                        guiAdd(0, 0, wnd);
                        dock_space->insert("EditorSpace", wnd);
                    }),
                    new GuiMenuListItem("CSG Scene", [&dock_space]() {
                        auto wnd = guiCreateWindow<GuiCsgDocument>();
                        guiAdd(0, 0, wnd);
                        dock_space->insert("EditorSpace", wnd);
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

    dock_space->getRoot()->setId("EditorSpace");
    dock_space->getRoot()->setLocked(true);
    dock_space->getRoot()->addWindow(wnd_explorer);
    dock_space->getRoot()->addWindow(wnd);
    dock_space->getRoot()->addWindow(wnd2);
    dock_space->getRoot()->addWindow(wnd_nodes);
    dock_space->getRoot()->addWindow(wnd_cdt);
    dock_space->getRoot()->addWindow(wnd_state_graph);
    dock_space->getRoot()->splitLeft();
    dock_space->getRoot()->left->setId("Sidebar");
    dock_space->getRoot()->left->setLocked(true);
    dock_space->getRoot()->left->addWindow(wnd_demo);
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