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

#include "game_ui/game_ui.hpp"
#include "csg/csg.hpp"


class camCameraController {
public:
    virtual ~camCameraController() {}

    virtual void init(cameraState* state) = 0;
    virtual void update(GameWorld* world, float dt, cameraState* state) = 0;
};

class Camera3d : public camCameraController {
    //InputContext inputCtx = InputContext("Camera");
    InputRange* inputTranslation;
    InputRange* inputRotation;
    InputAction* inputLeftClick;

    float cam_rot_y = 0;
    float cam_rot_x = 0;
    gfxm::vec3 cam_translation;
    gfxm::vec3 cam_wrld_translation;
    gfxm::quat qcam;
public:
    Camera3d() {/*
        inputGetContext("Player")->toFront();
        inputTranslation = inputGetRange("CharacterLocomotion");
        inputRotation = inputGetRange("CameraRotation");
        inputLeftClick = inputGetAction("Shoot");*/
    }

    void init(cameraState* state) override {
        state->projection = gfxm::perspective(gfxm::radian(65), 16.0f / 9.0f, 0.01f, 1000.0f);
        //proj = gfxm::ortho(-500.0f, 500.0f, -500.0f, 500.0f, -10.0f, 10.0f);
        cam_translation = gfxm::vec3(-1.5f, 1.5f, 0.7f);
        state->transform = gfxm::inverse(gfxm::lookAt(cam_translation, gfxm::vec3(0, 1, 0), gfxm::vec3(0, 1, 0)));
        state->view = gfxm::inverse(state->transform);
        cam_wrld_translation = state->transform * gfxm::vec4(0, 0, 0, 1);
        qcam = gfxm::to_quat(gfxm::to_mat3(state->transform));
    }
    void update(GameWorld* world, float dt, cameraState* state) override {
        gfxm::vec3 cam_lcl_delta_translation = inputTranslation->getVec3();
        gfxm::vec3 cam_lcl_delta_rotation;
        cam_lcl_delta_rotation = inputRotation->getVec3();
        gfxm::vec3 cam_wrld_delta_translation = state->transform * gfxm::vec4(cam_lcl_delta_translation, .0f);
        cam_wrld_translation += cam_wrld_delta_translation * (dt) * 6.0f;

        if (inputLeftClick->isPressed()) {
            cam_rot_y += cam_lcl_delta_rotation.y * (1.0f/60.f) * 0.5f; // don't use actual frame time here
            cam_rot_x += cam_lcl_delta_rotation.x * (1.0f/60.f) * 0.5f;
        }
        cam_rot_x = gfxm::clamp(cam_rot_x, -gfxm::pi * 0.5f, gfxm::pi * 0.5f);
        gfxm::quat qy = gfxm::angle_axis(cam_rot_y, gfxm::vec3(0, 1, 0));
        gfxm::quat qx = gfxm::angle_axis(cam_rot_x, gfxm::vec3(1, 0, 0));
        qcam = gfxm::slerp(qcam, qy * qx, 1 - pow(1 - 0.1f * 3.0f, dt * 60.0f)/* 0.1f*/);

        state->transform = gfxm::translate(gfxm::mat4(1.0f), cam_wrld_translation) * gfxm::to_mat4(qcam);

        state->view = gfxm::inverse(state->transform);
    }
};
class Camera3dThirdPerson : public camCameraController {
    //InputContext inputCtx = InputContext("CameraThirdPerson");
    InputRange* inputRotation;
    InputAction* inputLeftClick;
    InputRange* inputScroll;

    gfxm::vec3 target_desired;
    gfxm::vec3 target_interpolated;

    gfxm::vec2 lookat_angle_offset;
    
    float rotation_y = 0;
    float rotation_x = 0;
    float target_distance = 2.0f;
    gfxm::quat qcam;
public:
    Camera3dThirdPerson() {/*
        inputGetContext("Player")->toFront();
        inputRotation = inputGetRange("CameraRotation");
        inputLeftClick = inputGetAction("Shoot");
        inputScroll = inputGetRange("Scroll");*/     
    }

    void setTarget(const gfxm::vec3& tgt, const gfxm::vec2& lookat_angle_offset) {
        target_desired = tgt;
        this->lookat_angle_offset = lookat_angle_offset;
    }

    void init(cameraState* state) override {
        state->projection = gfxm::perspective(gfxm::radian(65), 16.0f / 9.0f, 0.01f, 1000.0f);
    }
    void update(GameWorld* world, float dt, cameraState* state) override {
        gfxm::vec3 cam_lcl_delta_rotation;
        cam_lcl_delta_rotation = inputRotation->getVec3();

        if (inputLeftClick->isPressed()) {
            rotation_y += cam_lcl_delta_rotation.y * (1.0f / 60.f) * 0.5f; // don't use actual frame time here
            rotation_x += cam_lcl_delta_rotation.x * (1.0f / 60.f) * 0.5f;
        }

        float scroll = inputScroll->getVec3().x;
        float mul = scroll > .0f ? (target_distance / 3.0f) : (target_distance / 4.0f);
        target_distance += scroll * mul;
        target_distance = gfxm::_max(0.5f, target_distance);

        rotation_x = gfxm::clamp(rotation_x, -gfxm::pi * 0.48f, gfxm::pi * 0.25f);
        gfxm::quat qy = gfxm::angle_axis(rotation_y, gfxm::vec3(0, 1, 0));
        gfxm::quat qx = gfxm::angle_axis(rotation_x, gfxm::vec3(1, 0, 0));
        qcam = gfxm::slerp(qcam, qy * qx, 1 - pow(1 - 0.1f * 3.0f, dt * 60.0f)/* 0.1f*/);

        state->transform = gfxm::to_mat4(qcam);
        gfxm::vec3 back_normal = gfxm::vec3(.5f, .0f, 1.f);
        gfxm::vec3 side_normal = gfxm::vec3(1.f, .0f, .0f);

        gfxm::vec3 tgt_ = target_desired;
        //tgt_.y += gfxm::_min(5.0f, gfxm::_max(.0f, target_distance - 2.5f) / 2.5f);
        target_interpolated = tgt_;// gfxm::lerp(target_interpolated, tgt_, 1 - pow(1 - 0.1f * 3.0f, dt * 60.0f));
        gfxm::vec3 target_pos = target_interpolated;
        
        float real_distance = target_distance;
        RayCastResult rayResult = world->getCollisionWorld()->rayTest(
            target_interpolated,
            target_interpolated + gfxm::vec3(state->transform * gfxm::vec4(back_normal, .0f)) * target_distance,
            COLLISION_LAYER_DEFAULT
        );
        if (rayResult.hasHit) {
            real_distance = rayResult.distance;
        }
        
        state->transform
            = gfxm::translate(gfxm::mat4(1.0f), target_pos) 
            * state->transform 
            * gfxm::translate(gfxm::mat4(1.0f), back_normal * real_distance);

        state->view = gfxm::inverse(state->transform);
    }
};


constexpr int TEST_INSTANCE_COUNT = 500;
[[cppi_class]];
class GameTest : public GameBase { 

    HSHARED<mdlSkeletalModelInstance> garuda_instance;

    InputContext input_ctx = InputContext("GameTest");
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
    Font* font = 0;
    Font* font2 = 0;

    //
    HSHARED<actorCharacter> chara;
    HSHARED<actorCharacter> chara2;

    HSHARED<AudioClip>          clip_whsh;

    PlayerAgentActor            tps_camera_actor;
    HSHARED<PlayerAgentActor>   chara_actor;

    PlayerAgentActor            free_camera_actor;

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
public:
    void init() override;
    void cleanup() override;

    void update(float dt) override;
    void draw(float dt) override;
};
