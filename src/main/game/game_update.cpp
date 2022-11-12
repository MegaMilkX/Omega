#include "game/game_common.hpp"

void GameCommon::Update(float dt) {
    static gfxm::mat4 cam_trs(1.0f);
    auto cam_node = world.getCurrentCameraNode();
    if (cam_node) {
        cam_trs = cam_node->getWorldTransform();
    }

    gfxm::vec3 loco_vec = inputCharaTranslation->getVec3();
    gfxm::mat3 loco_rot;
    loco_rot[2] = gfxm::normalize(cam_trs * gfxm::vec4(0, 0, 1, 0));
    loco_rot[1] = gfxm::vec3(0, 1, 0);
    loco_rot[0] = gfxm::cross(loco_rot[1], loco_rot[2]);
    loco_vec = loco_rot * loco_vec;
    loco_vec.y = .0f;
    loco_vec = gfxm::normalize(loco_vec);

    chara->setDesiredLocomotionVector(loco_vec);

    {
        static float time = .0f;
        gfxm::vec3 chara2_forward = chara2->getForward();
        //gfxm::vec3 target_pos = chara->getTranslation();
        gfxm::vec3 target_pos = gfxm::vec3(
            cosf(time) * 10.0f, .0f, sinf(time) * 10.0f
        );
        time += dt;
        gfxm::vec3 chara2_pos = chara2->getTranslation();
        float distance = gfxm::length(target_pos - chara2_pos);
        gfxm::vec3 normal_to_target = gfxm::normalize(target_pos - chara2_pos);
        float dot = gfxm::dot(chara2_forward, normal_to_target);
        bool is_target_visible = dot > FLT_EPSILON;
        if (is_target_visible && distance > 2.f) {
            chara2->setDesiredLocomotionVector(normal_to_target);
        } else {
            chara2->setDesiredLocomotionVector(gfxm::vec3(0, 0, 0));
        }
    }
    if (inputCharaUse->isJustPressed()) {
        chara->actionUse();
    }

    //cam->setTarget(chara->getWorldTransform() * gfxm::vec4(0, 1.6f, 0, 1), gfxm::vec2(0, gfxm::pi));
    //cam->update(&world, dt, &camState);
    //playerFps->update(&world, dt, &camState);

    {
        static float time = .0f;
        constexpr float THRESHOLD = 3.0f;
        if (time >= THRESHOLD) {
            world.postMessage(
                MSGID_MISSILE_SPAWN,
                MSGPLD_MISSILE_SPAWN{
                    gfxm::vec3(15.0f, 1.0f, 15.0f),
                    gfxm::quat(0,0,0,1)
                }
            );
            time -= THRESHOLD;
        }
        time += dt;
    }

    {
        static gfxm::ray r(gfxm::vec3(0, 0, 0), gfxm::vec3(0, 1, 0));
        if (inputFButtons[2]->isPressed()) {
            int mx, my;
            platformGetMousePos(&mx, &my);
            gfxm::rect rc = platformGetViewportRect();
            gfxm::vec2 viewport_size(
                rc.max.x - rc.min.x,
                rc.max.y - rc.min.y
            );
            my = viewport_size.y - my;
            /*
            r = gfxm::ray_viewport_to_world(
                viewport_size, gfxm::vec2(mx, my),
                camState.getProjection(), camState.getView()
            );*/
        }
        world.getCollisionWorld()->rayTest(r.origin, r.origin + r.direction * 10.0f);
    }

    audio().setListenerTransform(cam_trs);

    world.update(dt);
}