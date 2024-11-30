#pragma once

#include "game_test.auto.hpp"

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

#include "gui/gui.hpp"

#include "world/world.hpp"

#include "resource/resource.hpp"

#include "config.hpp"

#include "character/character.hpp"
#include "controllers/fps_character_controller.hpp"

#include "game_ui/game_ui.hpp"
#include "csg/csg.hpp"


constexpr int TEST_INSTANCE_COUNT = 500;
[[cppi_class]];
class GameTest : public GameBase {
    HSHARED<mdlSkeletalModelInstance> garuda_instance;

    InputContext input_ctx = InputContext("GameTest");
    InputAction* inputC;
    InputAction* inputV;
    InputAction* inputRecover;
    InputAction* inputSphereCast;
    InputAction* inputFButtons[12];
    InputAction* inputNumButtons[9];

    //gpuUniformBuffer* ubufCam3d;
    gpuUniformBuffer* ubufTime;

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
    RHSHARED<gpuMaterial> material_color;
    RHSHARED<gpuMaterial> material_instancing;

    std::unique_ptr<gpuGeometryRenderable> renderable;
    std::unique_ptr<gpuGeometryRenderable> renderable2;
    std::unique_ptr<gpuGeometryRenderable> renderable_plane;

    // Text
    std::shared_ptr<Font> font;
    std::shared_ptr<Font> font2;

    //
    HSHARED<actorCharacter> chara;
    HSHARED<actorCharacter> chara2;

    HSHARED<AudioClip>          clip_whsh;

    PlayerAgentActor            tps_camera_actor;
    HSHARED<PlayerAgentActor>   chara_actor;
    std::unique_ptr<Actor>      chara_actor_2;
    HSHARED<Actor>              sword_actor;

    PlayerAgentActor            free_camera_actor;

    PlayerAgentActor            fps_player_actor;

    Actor                       ambient_snd_actor;

    std::unique_ptr<DoorActor> door_actor;
    actorAnimTest anim_test;
    HSHARED<actorUltimaWeapon> ultima_weapon;
    HSHARED<actorJukebox> jukebox;
    HSHARED<actorVfxTest> vfx_test;

    // Collision
    CollisionSphereShape shape_sphere;
    CollisionBoxShape    shape_box;
    CollisionBoxShape    shape_box2;
    CollisionCapsuleShape shape_capsule;
    Collider collider_a;
    Collider collider_b;
    Collider collider_c;
    Collider collider_d;
    Collider collider_e;

    // Tooling gui
    GuiLabel* fps_label = 0;
public:
    TYPE_ENABLE();

    void init() override;
    void cleanup() override;

    void update(float dt) override;
    void draw(float dt) override;
};
