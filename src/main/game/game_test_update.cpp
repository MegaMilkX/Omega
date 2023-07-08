#include "game/game_test.hpp"

void GameTest::update(float dt) {
    if (inputFButtons[0]->isJustPressed()) {
        gpuGetDefaultRenderTarget()->setDefaultOutput("Final");
    } else if (inputFButtons[1]->isJustPressed()) {
        gpuGetDefaultRenderTarget()->setDefaultOutput("Albedo");
    } else if (inputFButtons[2]->isJustPressed()) {
        gpuGetDefaultRenderTarget()->setDefaultOutput("Position");
    } else if (inputFButtons[3]->isJustPressed()) {
        gpuGetDefaultRenderTarget()->setDefaultOutput("Normal");
    } else if (inputFButtons[4]->isJustPressed()) {
        gpuGetDefaultRenderTarget()->setDefaultOutput("Metalness");
    } else if (inputFButtons[5]->isJustPressed()) {
        gpuGetDefaultRenderTarget()->setDefaultOutput("Roughness");
    } else if (inputFButtons[6]->isJustPressed()) {
        gpuGetDefaultRenderTarget()->setDefaultOutput("Emission");
    } else if (inputFButtons[7]->isJustPressed()) {
        gpuGetDefaultRenderTarget()->setDefaultOutput("Lightness");
    } else if (inputFButtons[8]->isJustPressed()) {
        gpuGetDefaultRenderTarget()->setDefaultOutput("Depth");
    }

    static gfxm::mat4 cam_trs(1.0f);
    auto cam_node = getWorld()->getCurrentCameraNode();
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

    //chara->setDesiredLocomotionVector(loco_vec);
    {
        gfxm::vec3 target = chara_actor->getRoot()->getWorldTransform()[3];
        gfxm::vec3 me_pos = chara->getWorldTransform()[3];
        gfxm::vec3 dir = target - me_pos;
        if (dir.length() > 2.f) {
            dir.y = .0f;
            dir = gfxm::normalize(dir);
        } else {
            dir = gfxm::vec3(.0f, .0f, .0f);
        }
        chara->setDesiredLocomotionVector(dir);
    }

    {
        static float delay_buf = .0f;
        static gfxm::vec3 target_pos;
        if (delay_buf <= .0f) {
            delay_buf = rand() % 5;
            target_pos = gfxm::vec3(rand() % 40 - 20, .0f, rand() % 40 - 20);
        }
        static float time = .0f;
        gfxm::vec3 chara2_forward = chara2->getForward();
        //gfxm::vec3 target_pos = chara->getTranslation();
        /*gfxm::vec3 target_pos = gfxm::vec3(
            cosf(time) * 10.0f, .0f, sinf(time) * 10.0f
        );*/
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
        delay_buf -= dt;
    }
    /*
    if (inputCharaUse->isJustPressed()) {
        chara->actionUse();
    }*/

    //cam->setTarget(chara->getWorldTransform() * gfxm::vec4(0, 1.6f, 0, 1), gfxm::vec2(0, gfxm::pi));
    //cam->update(&world, dt, &camState);
    //playerFps->update(&world, dt, &camState);

    {
        static float time = .0f;
        constexpr float THRESHOLD = 3.0f;
        if (time >= THRESHOLD) {
            getWorld()->postMessage(
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
        getWorld()->getCollisionWorld()->rayTest(r.origin, r.origin + r.direction * 10.0f);
    }

    audio().setListenerTransform(cam_trs);

    GameBase::update(dt);
}