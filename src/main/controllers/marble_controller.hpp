#pragma once

#include "marble_controller.auto.hpp"

#include <random>

#include "world/controller/actor_controller.hpp"
#include "player/player.hpp"
#include "input/input.hpp"
#include "audio/audio.hpp"

#include "world/node/rigid_body_node.hpp"


[[cppi_class]];
class MarbleDriver : public ActorDriver {
    int getExecutionPriority() const override { return EXEC_PRIORITY_FIRST; }
    
    ResourceRef<AudioClip> clip_jump;

    ActorNodeView<RigidBodyNode> body;
    gfxm::vec3 desired_dir;
public:
    TYPE_ENABLE();

    MarbleDriver() {
        body = registerNodeView<RigidBodyNode>("body", true);
        clip_jump = loadResource<AudioClip>("audio/sfx/swoosh");
    }

    GAME_MESSAGE onMessage(GAME_MESSAGE msg) override {
        switch (msg.msg) {
        case GAME_MSG::PAWN_CMD: {
            auto pld = msg.getPayload<GAME_MSG::PAWN_CMD>();
            switch (pld.cmd) {
            case ePawnMoveDirection:
                desired_dir = pld.params;
                return GAME_MSG::HANDLED;
            }
            break;
        }
        }
        return GAME_MSG::NOT_HANDLED;
    }

    void onReset() override {}
    void onSpawnActorDriver(WorldSystemRegistry& reg, Actor* actor) override {}
    void onDespawnActorDriver(WorldSystemRegistry& reg, Actor* actor) override {}
    
    void onUpdate(float dt) override {
        gfxm::vec3 world_v = desired_dir;            

        if (body.isValid()) {
            float max_av = 200.f;
            gfxm::vec3 Nangvel = gfxm::normalize(gfxm::cross(gfxm::vec3(0, 1, 0), world_v));
            float factor = 1.0f - gfxm::dot(body->collider.angular_velocity, Nangvel) / max_av;
            gfxm::vec3 angvelfix = gfxm::vec3(0, 1, 0) * gfxm::dot(body->collider.angular_velocity, gfxm::vec3(0, 1, 0));
            
            body->collider.angular_velocity *= powf(.5f, dt);
            body->collider.impulseAtPoint(world_v * 7.5f * dt, body->getTranslation() + gfxm::vec3(.0f, .25f, .0f));
            
            // TODO:
            /*
            if (inputJump->isPressed()) {
                body->collider.gravity_factor = 2.0f;
            } else {
                body->collider.gravity_factor = 1.0f;
            }*/
        }
    }
};