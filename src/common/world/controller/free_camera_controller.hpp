#pragma once

#include "free_camera_controller.auto.hpp"
#include "actor_controller.hpp"

#include "input/input.hpp"
#include "world/actor.hpp"
#include "world/world.hpp"
#include "player/player.hpp"
#include "audio/audio.hpp"


[[cppi_class]];
class FreeCameraController : public ActorController {
    int getExecutionPriority() const override { return EXEC_PRIORITY_CAMERA; }

    InputContext input_ctx;
    InputRange* rangeLook = 0;
    InputRange* rangeMove = 0;
    InputAction* actionLeftClick = 0;

    IPlayer* current_player = 0;

    gfxm::vec3 world_pos;
    float rotation_y = 0;
    float rotation_x = 0;
    gfxm::quat qcam;
public:
    TYPE_ENABLE();

    FreeCameraController() {
        rangeLook = input_ctx.createRange("CameraRotation");
        rangeMove = input_ctx.createRange("CharacterLocomotion");
        actionLeftClick = input_ctx.createAction("Shoot");
    }

    void onReset() override {}
    void onSpawn(Actor* actor) override {
        assert(actor->getRoot());
    }
    void onDespawn(Actor* actor) override {}
    void onActorNodeRegister(type t, ActorNode* component, const std::string& name) override {}
    void onActorNodeUnregister(type t, ActorNode* component, const std::string& name) override {}
    GAME_MESSAGE onMessage(GAME_MESSAGE msg) override {
        switch (msg.msg) {
        case GAME_MSG::PLAYER_ATTACH: {
            current_player = msg.getPayload<GAME_MSG::PLAYER_ATTACH>().player;
            current_player->getInputState()->pushContext(&input_ctx);
            if (current_player->getViewport()) {
                gfxm::mat4 trs = gfxm::inverse(current_player->getViewport()->getViewTransform());
                world_pos = trs * gfxm::vec4(0, 0, 0, 1);
                gfxm::vec2 euler = gfxm::to_euler_xy(trs);
                rotation_x = euler.x;
                rotation_y = euler.y;
            }
            return GAME_MSG::HANDLED;
        }
        case GAME_MSG::PLAYER_DETACH: {
            auto player = msg.getPayload<GAME_MSG::PLAYER_DETACH>().player;
            player->getInputState()->removeContext(&input_ctx);
            current_player = 0;
            return GAME_MSG::HANDLED;
        }
        }
        return GAME_MSG::NOT_HANDLED;
    }
    void onUpdate(RuntimeWorld* world, float dt) override {
        if (!current_player) {
            return;
        }

        auto actor = getOwner();
        auto root = actor->getRoot();
        assert(root);

        gfxm::vec3 cam_lcl_delta_rotation;
        cam_lcl_delta_rotation = rangeLook->getVec3();
        if (actionLeftClick->isPressed()) {
            rotation_y += cam_lcl_delta_rotation.y * (1.0f / 60.f) * 0.5f; // don't use actual frame time here
            rotation_x += cam_lcl_delta_rotation.x * (1.0f / 60.f) * 0.5f;
        }

        gfxm::vec3 cam_lcl_delta_pos = rangeMove->getVec3();

        rotation_x = gfxm::clamp(rotation_x, -gfxm::pi * 0.48f, gfxm::pi * 0.25f);
        gfxm::quat qy = gfxm::angle_axis(rotation_y, gfxm::vec3(0, 1, 0));
        gfxm::quat qx = gfxm::angle_axis(rotation_x, gfxm::vec3(1, 0, 0));
        qcam = qy * qx;// gfxm::slerp(qcam, qy * qx, 1 - pow(1 - 0.1f * 3.0f, dt * 60.0f));

        gfxm::mat4 orient_trs = gfxm::to_mat4(qcam);

        gfxm::vec3 world_delta_pos = orient_trs * gfxm::vec4(gfxm::normalize(cam_lcl_delta_pos), 1.f);
        world_pos += world_delta_pos * 10.f * dt;

        root->setTranslation(world_pos);
        root->setRotation(qcam);
        
        auto viewport = current_player->getViewport();
        if (!viewport) {
            return;
        }
        viewport->setFov(65.f);
        viewport->setCameraPosition(world_pos);
        viewport->setCameraRotation(qcam);
        viewport->setZFar(1000.f);
        viewport->setZNear(.01f);

        // TODO: ?
        if (current_player) {
            audioSetListenerTransform(root->getWorldTransform());
        }
    }
};
