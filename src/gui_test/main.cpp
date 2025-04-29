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

// TODO: REMOVE THIS !!!
#include "resource_cache/resource_cache.hpp"
#include "static_model/static_model.hpp"


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

std::unique_ptr<GuiDockSpace> dock_space;
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
    dock_space->insert("EditorSpace", wnd);

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
        elem->setSize(gui::content(), gui::content());

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
    void onLayout(const gfxm::vec2& extents, uint64_t flags) override {
        guiLayoutBox(&box, extents);
        guiLayoutPlaceBox(&box, gfxm::vec2(0, 0));

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
class GuiBigTextTestWindow : public GuiWindow {
public:
    GuiBigTextTestWindow()
    : GuiWindow("BigText") {
        auto container = new GuiElement();
        container->setStyleClasses({ "container", "code" });
        pushBack(container);
        container->setSize(gui::perc(100), gui::perc(100));
        container->pushBack(R"(#vertex
#version 450 

in vec3 inPosition;
in vec3 inColorRGB;
in vec2 inUV;
in vec3 inNormal;
out vec3 pos_frag;
out vec3 col_frag;
out vec2 uv_frag;
out vec3 normal_frag;

layout(std140) uniform bufCamera3d {
	mat4 matProjection;
	mat4 matView;
	vec2 screenSize;
};
layout(std140) uniform bufModel {
	mat4 matModel;
};

void main(){
	uv_frag = inUV;
	normal_frag = normalize((matModel * vec4(inNormal, 0)).xyz);
	pos_frag = (matModel * vec4(inPosition, 1)).xyz;
	col_frag = inColorRGB;
	vec4 pos = matProjection * matView * matModel * vec4(inPosition, 1);
	gl_Position = pos;
}

#fragment
#version 450
in vec3 pos_frag;
in vec3 col_frag;
in vec2 uv_frag;
in vec3 normal_frag;

out vec4 outAlbedo;
out vec4 outPosition;
out vec4 outNormal;
out vec4 outMetalness;
out vec4 outRoughness;

uniform sampler2D texAlbedo;
layout(std140) uniform bufCamera3d {
	mat4 matProjection;
	mat4 matView;
	vec2 screenSize;
};

void main(){
	vec3 N = normalize(normal_frag);
	if(!gl_FrontFacing) {
		N *= -1;
	}
	vec4 pix = texture(texAlbedo, uv_frag);
	outAlbedo = vec4(pix);
	outPosition = vec4(pos_frag, 1);
	outNormal = vec4(N, 1);
	outMetalness = vec4(0.3, 0, 0, 1);
	outRoughness = vec4(0.4, 0, 0, 1);
}

)");
    }
};


#include "audio/audio.hpp"
#include "audio/res_cache_audio_clip.hpp"

#include "gui_cdt_test_window.hpp"
int main(int argc, char* argv) {
    cppiReflectInit();

    reflectInit();
    platformInit(true, true);
    typefaceInit();
    gpuInit();

    std::shared_ptr<Font> fnt = fontGet("fonts/ProggyClean.ttf", 16, 72);
    guiInit(fnt);
    guiSetMessageCallback(&messageCb);
    guiSetDropFileCallback(&dropFileCb);

    /*
    auto files = fsFindAllFiles(".", "*.import");
    for (auto& f : files) {
        std::fstream file(f);
        nlohmann::json j;
        try {
            j << file;
        } catch(const std::exception& ex) {
            continue;
        }
        ImportSettingsFbx import_;
        import_.from_json(j);

        nlohmann::json out_json;
        import_.to_json(out_json);

        file.close();
        {
            std::fstream file(f, std::ios::out | std::ios::trunc);
            file << out_json.dump(4);
        }
    }*/

    gui::style guistyle;
    gui::style_sheet sheet;
    sheet.select_styles(&guistyle, { "control", "collapsing-header" });
    guistyle.dbg_print();

    resInit();
    animInit();
    resAddCache<AudioClip>(new resCacheAudioClip);
    audioInit();

    // TODO: REMOVE THIS !!!
    resAddCache<StaticModel>(new resCacheDefault<StaticModel>());

    int screen_width = 0, screen_height = 0;
    platformGetWindowSize(screen_width, screen_height);
    
    dock_space.reset(new GuiDockSpace());
    
    auto wnd_demo = guiGetRoot()->pushBack(new GuiDemoWindow);
    auto wnd_explorer = guiGetRoot()->pushBack(new GuiFileExplorerWindow());
    auto wnd_nodes = guiGetRoot()->pushBack(new GuiNodeEditorWindow());
    auto wnd_cdt = guiGetRoot()->pushBack(new GuiCdtTestWindow());
    auto wnd_big_text = guiGetRoot()->pushBack(new GuiBigTextTestWindow);
    /*
    auto wnd_layout = new GuiLayoutTestWindow();
    guiAdd(0, 0, wnd_layout);
    guiAddManagedWindow(wnd_layout);
    */
    //guiAdd(0, 0, new GuiTestWindow2);
    
    //guiAdd(0, 0, new GuiBoxWindow);
    
    guiGetRoot()->createMenuBar()
        ->addItem(new GuiMenuItem("File", {
                new GuiMenuListItem("New", {
                    new GuiMenuListItem("Animator", []() {
                        auto wnd = guiCreateWindow<GuiAnimatorDocument>();
                        guiAdd(0, 0, wnd);
                        dock_space->insert("EditorSpace", wnd);
                    }),
                    new GuiMenuListItem("Animation Sequence", []() {
                        auto wnd = guiCreateWindow<GuiSequenceDocument>();
                        guiAdd(0, 0, wnd);
                        dock_space->insert("EditorSpace", wnd);
                    }),
                    new GuiMenuListItem("CSG Scene", []() {
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
    dock_space->getRoot()->addWindow(wnd_nodes);
    dock_space->getRoot()->addWindow(wnd_cdt);
    dock_space->getRoot()->addWindow(wnd_big_text);
    dock_space->getRoot()->addWindow(wnd_explorer);
    dock_space->getRoot()->splitLeft();
    dock_space->getRoot()->left->setId("Sidebar");
    dock_space->getRoot()->left->setLocked(true);
    dock_space->getRoot()->left->addWindow(wnd_demo);
    dock_space->getRoot()->split_pos = 0.20f;
    dock_space->getRoot()->right->split_pos = 0.3f; 
    

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
            
            inst->render_target->bindFrameBuffer("Normal");
            dbgDrawDraw(
                inst->projection,
                inst->view_transform,
                0, 0, inst->render_target->getWidth(), inst->render_target->getHeight()
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