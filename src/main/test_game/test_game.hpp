#pragma once

#include "test_game.auto.hpp"

#include "game/game_base.hpp"

#include "gpu/gpu_pipeline.hpp"
#include "gpu/gpu_mesh.hpp"
#include "gpu/gpu_material.hpp"
#include "gpu/gpu_renderable.hpp"

#include "assimp_load_scene.hpp"

#include "input/input.hpp"

#include "typeface/typeface.hpp"
#include "typeface/font.hpp"
#include "gpu/gpu_uniform_buffer.hpp"
#include "gpu/gpu_text.hpp"


#include "gpu/render/uniform.hpp"

#include "gpu/gpu.hpp"
#include "gpu/renderable/geometry.hpp"
#include "gpu/renderable/decal.hpp"

#include "gui/gui.hpp"

#include "world/world.hpp"

#include "resource/resource.hpp"

#include "config.hpp"

#include "character/character.hpp"
#include "controllers/fps_character_controller.hpp"
#include "controllers/marble_controller.hpp"

#include "game_ui/game_ui.hpp"
#include "csg/csg.hpp"

#include "experimental/hl2/hl2_bsp.hpp"
#include "experimental/hl2/hl2_vtf.hpp"
#include "experimental/hl2/hl2_material.hpp"
#include "experimental/hl2/hl2_mdl.hpp"

#include "scene/scene_manager.hpp"
#include "agents/fps_player_agent.hpp"
#include "agents/tps_player_agent.hpp"
#include "agents/free_cam_agent.hpp"

#include "resource_manager/resource_manager.hpp"

#include <io.h>
#include <fcntl.h>
struct PipeParams {
    HANDLE hRead;
    GuiTextElement* elem;
};

inline DWORD WINAPI PipeReader(LPVOID lpParam) {
    PipeParams* params = (PipeParams*)lpParam;
    char buffer[256];
    DWORD bytesRead = 0;

    while (ReadFile(params->hRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL)) {
        buffer[bytesRead] = '\0';
        params->elem->setContent(buffer);
    }
}

inline void RedirectCRT(HANDLE hPipeWrite)
{
    int fd = _open_osfhandle((intptr_t)hPipeWrite, _O_TEXT);

    // Duplicate to stdout & stderr
    _dup2(fd, _fileno(stdout));
    _dup2(fd, _fileno(stderr));

    // Turn off buffering
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
}

class GuiConsole : public GuiWindow {
public:
    GuiConsole()
    : GuiWindow("Console") {
        GuiTextElement* elem = new GuiTextElement;
        pushBack(elem);

        HANDLE hRead, hWrite;
        SECURITY_ATTRIBUTES sa = {
            sizeof(SECURITY_ATTRIBUTES),
            NULL,
            TRUE
        };
        CreatePipe(&hRead, &hWrite, &sa, 0);

        SetStdHandle(STD_OUTPUT_HANDLE, hWrite);
        SetStdHandle(STD_ERROR_HANDLE, hWrite);

        RedirectCRT(hWrite);
        /*
        FILE* fp = 0;
        freopen_s(&fp, "CONOUT$", "w", stdout);
        freopen_s(&fp, "CONOUT$", "w", stderr);
        */
        PipeParams* params = new PipeParams{ hRead, elem };
        CreateThread(NULL, 0, PipeReader, (LPVOID)params, 0, NULL);
    }


};

// TODO: REMOVE
struct TextData : public ILoadable {
public:
    std::string text;

    TextData() {
        LOG_DBG("TextData()");
    }
    ~TextData() {
        LOG_DBG("~TextData()");
    }

    DEFINE_EXTENSIONS(e_txt, e_csv, e_ini, e_cfg, e_json, e_xml);

    bool load(byte_reader& reader) override {
        auto view = reader.try_slurp();
        if(!view) return false;
        text = std::string(view.data, view.data + view.size);
        return true;
    }
    void print() {
        LOG("\n" << text << "\n");
    }
};

constexpr int TEST_INSTANCE_COUNT = 500;
[[cppi_class]];
class TestGameInstance : public IGameInstance {
    std::unique_ptr<IWorld> world;
    std::unique_ptr<IPlayer> primary_player;
    std::unique_ptr<EngineRenderView> primary_view;
    std::unique_ptr<SceneManager> scene_mgr;

    /*
    FpsPlayerAgent* fps_agent = nullptr;
    FpsSpectator* fps_spectator = nullptr;
    TpsPlayerAgent* tps_agent = nullptr;
    TpsSpectator* tps_spectator = nullptr;*/

    HSHARED<SkeletalModelInstance> garuda_instance;

    InputContext input_ctx = InputContext("TestGame");
    InputAction* inputC;
    InputAction* inputV;
    InputAction* inputZ;
    InputAction* inputToggleWireframe;
    InputAction* inputRecover;
    InputAction* inputSphereCast;
    InputRange* inputScroll;
    InputAction* inputFButtons[12];
    InputAction* inputNumButtons[9];

    //cameraState camState;
    //std::unique_ptr<Camera3d> cam;
    //std::unique_ptr<Camera3dThirdPerson> cam;
    //std::unique_ptr<playerControllerFps> playerFps;
    
    gpuMesh mesh;
    gpuMesh mesh_sphere;
    gpuMesh gpu_mesh_plane;

    gfxm::vec4          positions[TEST_INSTANCE_COUNT];
    gpuBuffer           inst_pos_buffer;
    gpuInstancingDesc   instancing_desc;

    RHSHARED<gpuMaterial> material;
    RHSHARED<gpuMaterial> material2;
    RHSHARED<gpuMaterial> material3;
    RHSHARED<gpuMaterial> material_parallax;
    RHSHARED<gpuMaterial> material_modular;
    RHSHARED<gpuMaterial> material_new_decal;
    RHSHARED<gpuMaterial> material_color;
    RHSHARED<gpuMaterial> material_instancing;

    std::unique_ptr<gpuGeometryRenderable> renderable;
    std::unique_ptr<gpuGeometryRenderable> renderable2;
    std::unique_ptr<gpuGeometryRenderable> renderable_parallax;
    std::unique_ptr<gpuGeoRenderable> renderable_new;
    std::unique_ptr<gpuDecalRenderable> renderable_new_decal;
    std::unique_ptr<gpuGeometryRenderable> renderable_plane;
    std::unique_ptr<gpuGeometryRenderable> renderable_sphere;

    //HL2Scene hl2bspmodel;

    // Decals
    scnDecal* test_dcl = 0;

    // Text
    std::shared_ptr<Font> font;
    std::shared_ptr<Font> font2;

    //
    HSHARED<actorCharacter> chara;
    HSHARED<actorCharacter> chara2;

    ResourceRef<AudioClip>      clip_whsh;

    Actor                       tps_camera_actor;
    std::unique_ptr<Actor>      chara_actor;
    std::unique_ptr<Actor>      chara_actor_2;
    HSHARED<Actor>              sword_actor;
    HSHARED<Actor>              redbull_actor;

    Actor                       demo_camera_actor;

    Actor                       fps_player_actor;

    Actor                       ball_actor;

    Actor                       ambient_snd_actor;

    std::unique_ptr<DoorActor> door_actor;
    actorAnimTest anim_test;
    HSHARED<actorUltimaWeapon> ultima_weapon;
    HSHARED<actorJukebox> jukebox;
    HSHARED<actorVfxTest> vfx_test;

    // Collision
    phyBoxShape    shape_box;
    phyBoxShape    shape_box2;
    phyCapsuleShape shape_capsule;
    phySphereShape shape_sphere;
    phyRigidBody collider_b;
    phyRigidBody collider_d;
    phyRigidBody collider_e;
    phyRigidBody collider_f;
    Actor capsule_actor;

    // Tooling gui
    GuiLabel* fps_label = 0;

    // Dynamic bones
    float rope_terminal_velocity = 20.f;
    float rope_damping = 0.1f;
    float rope_rigidity = 1.f;
    float rope_bend_rigidity = 1.f;
    float rope_mass = .1;
    float rope_time_scale = 1.f;
    bool rope_reset = true;
    bool rope_is_sim_running = false;
    bool rope_step_once = false;

    IWorld* getWorld() { return world.get(); }

public:
    TYPE_ENABLE();

    void onInit(IEngineRuntime* rt) override;
    void onCleanup() override;

    void onUpdate(float dt) override;
    void onDraw(float dt) override;

    void onPlayerJoined(IPlayer* player) override;
    void onPlayerLeft(IPlayer* player) override;
};
