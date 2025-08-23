#pragma once

#include "marble_controller.auto.hpp"

#include <random>

#include "world/controller/actor_controller.hpp"
#include "player/player.hpp"
#include "input/input.hpp"
#include "audio/audio.hpp"

#include "world/node/rigid_body_node.hpp"


[[cppi_class]];
class MarbleController : public ActorController {
    int getExecutionPriority() const override { return EXEC_PRIORITY_FIRST; }

    IPlayer* current_player = 0;

    InputContext input_ctx = InputContext("MarbleController");
    InputRange* rangeTranslation = 0;
    InputAction* inputJump = 0;
    
    RHSHARED<AudioClip> clip_jump;

    ActorNodeView<RigidBodyNode> body;
public:
    TYPE_ENABLE();

    MarbleController() {
        body = registerNodeView<RigidBodyNode>("body", true);

        rangeTranslation = input_ctx.createRange("CharacterLocomotion");
        inputJump = input_ctx.createAction("Jump");

        clip_jump = getAudioClip("audio/sfx/swoosh.ogg");
    }

    GAME_MESSAGE onMessage(GAME_MESSAGE msg) override {
        switch (msg.msg) {
        case GAME_MSG::PLAYER_ATTACH: {
            current_player = msg.getPayload<GAME_MSG::PLAYER_ATTACH>().player;
            current_player->getInputState()->pushContext(&input_ctx);
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

    void onReset() override {}
    void onSpawn(Actor* actor) override {}
    void onDespawn(Actor* actor) override {}
    
    void onUpdate(RuntimeWorld* world, float dt) override {
        if(current_player) {
            gfxm::vec3 input_v = gfxm::normalize(rangeTranslation->getVec3());

            gfxm::mat4 input_trs(1.0f);
            if (current_player->getViewport()) {
                input_trs = gfxm::inverse(current_player->getViewport()->getViewTransform());
            }

            gfxm::mat3 orient;
            gfxm::vec3 fwd = input_trs * gfxm::vec4(0, 0, 1, 0);
            fwd.y = .0f;
            fwd = gfxm::normalize(fwd);
            orient[2] = fwd;
            orient[1] = gfxm::vec3(0, 1, 0);
            orient[0] = gfxm::cross(orient[1], orient[2]);
            gfxm::vec3 world_v;
            world_v = orient * input_v;            

            if (body.isValid()) {
                float max_av = 200.f;
                gfxm::vec3 Nangvel = gfxm::normalize(gfxm::cross(gfxm::vec3(0, 1, 0), world_v));
                float factor = 1.0f - gfxm::dot(body->collider.angular_velocity, Nangvel) / max_av;
                gfxm::vec3 angvelfix = gfxm::vec3(0, 1, 0) * gfxm::dot(body->collider.angular_velocity, gfxm::vec3(0, 1, 0));
                
                //body->collider.angular_velocity -= angvelfix * powf(.6f, dt);
                body->collider.angular_velocity *= powf(.5f, dt);
                //body->collider.angular_velocity += Nangvel * 100.f * dt * factor;
                //body->collider.velocity += world_v * 10.f * dt;
                body->collider.impulseAtPoint(world_v * 5.5f * dt, body->getTranslation() + gfxm::vec3(.0f, .25f, .0f));
                /*
                if (inputJump->isJustPressed()) {
                    body->collider.velocity += gfxm::vec3(0, 5, 0);
                }*/
                if (inputJump->isPressed()) {
                    body->collider.gravity_factor = 2.0f;
                } else {
                    body->collider.gravity_factor = 1.0f;
                }
                
                /*
                {
                    gfxm::mat4 view = current_player->getViewport()->getViewTransform();
                    gfxm::vec3 forward = -gfxm::inverse(view)[2];
                    gfxm::vec3 velo = body->collider.velocity;
                    float fov_factor = gfxm::_max(.0f, gfxm::dot(velo, forward)) / 25.f;
                    fov_factor = gfxm::_min(1.f, fov_factor);
                    float fov = 75.f + 25.f * fov_factor;
                    current_player->getViewport()->setFov(fov);
                }*/
            }
        }
    }
};