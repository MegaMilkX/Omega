#pragma once

#include "fps_character_controller.auto.hpp"

#include "world/controller/actor_controller.hpp"
#include "player/player.hpp"
#include "input/input.hpp"
#include "platform/platform.hpp"


[[cppi_class]];
class FpsCharacterController : public ActorController {
    int getExecutionPriority() const override { return EXEC_PRIORITY_FIRST; }

    IPlayer* current_player = 0;

    InputContext input_ctx = InputContext("FpsCharacterController");
    InputRange* rangeTranslation = 0;
    InputRange* rangeRotation = 0;
    //InputAction* actionInteract = 0;

    float rotation_x = .0f;
    float rotation_y = .0f;
    float velocity = .0f;
    gfxm::vec3 latestTranslationInput;

public:
    TYPE_ENABLE();

    FpsCharacterController() {
        rangeTranslation = input_ctx.createRange("CharacterLocomotion");
        rangeRotation = input_ctx.createRange("CameraRotation");
    }

    GAME_MESSAGE onMessage(GAME_MESSAGE msg) override {
        switch (msg.msg) {
        case GAME_MSG::PLAYER_ATTACH: {
            current_player = msg.getPayload<GAME_MSG::PLAYER_ATTACH>().player;
            current_player->getInputState()->pushContext(&input_ctx);
            platformPushMouseState(true, true);
            return GAME_MSG::HANDLED;
        }
        case GAME_MSG::PLAYER_DETACH: {
            auto player = msg.getPayload<GAME_MSG::PLAYER_DETACH>().player;
            player->getInputState()->removeContext(&input_ctx);
            current_player = 0;
            platformPopMouseState();
            return GAME_MSG::HANDLED;
        }
        }
        return GAME_MSG::NOT_HANDLED;
    }

    void onReset() override {}
    void onSpawn(Actor* actor) override {}
    void onDespawn(Actor* actor) override {}
    void onActorNodeRegister(type t, ActorNode* component, const std::string& name) override {}
    void onActorNodeUnregister(type t, ActorNode* component, const std::string& name) override {}

    void onUpdate(RuntimeWorld* world, float dt) override {
        auto root = getOwner()->getRoot();
        if (!root) {
            assert(false);
            return;
        }

        rotation_y += gfxm::radian(rangeRotation->getVec3().y) *.1f;
        rotation_x += gfxm::radian(rangeRotation->getVec3().x) *.1f;
        rotation_x = gfxm::clamp(rotation_x, -gfxm::pi * 0.5f, gfxm::pi * 0.5f);

        gfxm::quat qy = gfxm::angle_axis(rotation_y, gfxm::vec3(0, 1, 0));
        gfxm::quat qx = gfxm::angle_axis(rotation_x, gfxm::vec3(1, 0, 0));
        gfxm::quat qcam = qy * qx;

        const float max_velocity = 5.0f;
        if (rangeTranslation->getVec3().length() > FLT_EPSILON) {
            latestTranslationInput = rangeTranslation->getVec3();
            latestTranslationInput = gfxm::to_mat4(qy) * gfxm::vec4(gfxm::normalize(latestTranslationInput), .0f);
            velocity = gfxm::lerp(velocity, max_velocity, .1f);
        } else {
            velocity = gfxm::lerp(velocity, .0f, .1f);
        }

        gfxm::vec3 translation = gfxm::vec3(0, 0, 0);
        if (velocity > .0f) {
            translation.x += latestTranslationInput.x * dt * velocity;
            translation.z += latestTranslationInput.z * dt * velocity;
            //total_distance_walked += translation_delta.length() * dt * 5.0f;
        }
        root->translate(translation);

        if (current_player) {
            auto viewport = current_player->getViewport();

            viewport->setFov(65.f);
            viewport->setCameraPosition(root->getWorldTransform() * gfxm::vec4(0, 1.6, 0, 1));
            viewport->setCameraRotation(qcam);
            viewport->setZFar(1000.f);
            viewport->setZNear(.01f);
        }

        audioSetListenerTransform(root->getWorldTransform() * gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(0, 1.6, 0)));
    }
};