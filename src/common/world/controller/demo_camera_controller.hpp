#pragma once

#include "demo_camera_controller.auto.hpp"
#include "actor_controller.hpp"

#include "input/input.hpp"
#include "world/actor.hpp"
#include "world/world.hpp"
#include "player/player.hpp"
#include "audio/audio.hpp"
#include "math/bezier.hpp"


[[cppi_class]];
class DemoCameraDriver : public ActorDriver {
    int getExecutionPriority() const override { return EXEC_PRIORITY_CAMERA; }

    IPlayer* current_player = 0;

    gfxm::vec3 world_pos;
    float time = .0f;
public:
    TYPE_ENABLE();

    DemoCameraDriver() {}

    void onReset() override {}
    void onSpawnActorDriver(WorldSystemRegistry& reg, Actor* actor) override {
        assert(actor->getRoot());
    }
    void onDespawnActorDriver(WorldSystemRegistry& reg, Actor* actor) override {}
    void onActorNodeRegister(type t, ActorNode* component, const std::string& name) override {}
    void onActorNodeUnregister(type t, ActorNode* component, const std::string& name) override {}
    GAME_MESSAGE onMessage(GAME_MESSAGE msg) override {
        switch (msg.msg) {
        case GAME_MSG::PLAYER_ATTACH: {
            current_player = msg.getPayload<GAME_MSG::PLAYER_ATTACH>().player;
            if (current_player->getViewport()) {
                gfxm::mat4 trs = gfxm::inverse(current_player->getViewport()->getViewTransform());
                world_pos = trs * gfxm::vec4(0, 0, 0, 1);
            }
            return GAME_MSG::HANDLED;
        }
        case GAME_MSG::PLAYER_DETACH: {
            auto player = msg.getPayload<GAME_MSG::PLAYER_DETACH>().player;
            current_player = 0;
            return GAME_MSG::HANDLED;
        }
        }
        return GAME_MSG::NOT_HANDLED;
    }
    void onUpdate(float dt) override {
        if (!current_player) {
            return;
        }

        auto actor = getOwner();
        auto root = actor->getRoot();
        assert(root);

        world_pos = gfxm::vec3(
            15.f * cosf(time * .25f), 5.f + 4.f * sinf(time * .25f), 15.f * sinf(time * .25f)
        );
        time += dt;

        gfxm::mat3 m3cam = gfxm::to_mat3(gfxm::lookAtView(world_pos, gfxm::vec3(0, 7.5, 0), gfxm::vec3(0, 1, 0)));
        //m3cam = gfxm::to_mat3(gfxm::inverse(gfxm::to_mat4(m3cam)));
        gfxm::quat qcam = gfxm::to_quat(m3cam);

        root->setTranslation(world_pos);
        root->setRotation(qcam);
        
        auto viewport = current_player->getViewport();
        if (!viewport) {
            return;
        }
        auto cam = viewport->getCamera();
        if (!cam) {
            return;
        }
        cam->setFov(gfxm::radian(65.f));
        cam->setCameraPosition(world_pos);
        cam->setCameraRotation(qcam);
        cam->setZFar(1000.f);
        cam->setZNear(.01f);

        // TODO: ?
        if (current_player) {
            audioSetListenerTransform(root->getWorldTransform());
        }
    }
};
