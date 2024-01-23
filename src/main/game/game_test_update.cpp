#include "game/game_test.hpp"

#include "world/experimental/actor_anim.hpp"

void GameTest::update(float dt) {
    LocalPlayer* local_player = dynamic_cast<LocalPlayer*>(playerGetPrimary());
    assert(local_player);
    Viewport* viewport = local_player->getViewport();
    assert(viewport);
    gpuRenderTarget* render_target = viewport->getRenderTarget();

    if (inputRecover->isJustPressed()) {
        chara_actor->getRoot()->setTranslation(gfxm::vec3(0, 0, 0));
    }

    if (inputFButtons[0]->isJustPressed()) {
        render_target->setDefaultOutput("Final");
    } else if (inputFButtons[1]->isJustPressed()) {
        render_target->setDefaultOutput("Albedo");
    } else if (inputFButtons[2]->isJustPressed()) {
        render_target->setDefaultOutput("Position");
    } else if (inputFButtons[3]->isJustPressed()) {
        render_target->setDefaultOutput("Normal");
    } else if (inputFButtons[4]->isJustPressed()) {
        render_target->setDefaultOutput("Metalness");
    } else if (inputFButtons[5]->isJustPressed()) {
        render_target->setDefaultOutput("Roughness");
    } else if (inputFButtons[6]->isJustPressed()) {
        render_target->setDefaultOutput("Emission");
    } else if (inputFButtons[7]->isJustPressed()) {
        render_target->setDefaultOutput("Lightness");
    } else if (inputFButtons[8]->isJustPressed()) {
        render_target->setDefaultOutput("Depth");
    }

    if (inputNumButtons[1]->isJustPressed()) {
        playerGetPrimary()->clearAgents();
        playerLinkAgent(playerGetPrimary(), chara_actor.get());
        playerLinkAgent(playerGetPrimary(), &tps_camera_actor);
        audioPlayOnce(clip_whsh->getBuffer(), .5f, .0f);
    } else if(inputNumButtons[2]->isJustPressed()) {
        playerGetPrimary()->clearAgents();
        playerLinkAgent(playerGetPrimary(), chara_actor.get());
        audioPlayOnce(clip_whsh->getBuffer(), .5f, .0f);
    } else if(inputNumButtons[3]->isJustPressed()) {
        playerGetPrimary()->clearAgents();
        playerLinkAgent(playerGetPrimary(), &free_camera_actor);
        audioPlayOnce(clip_whsh->getBuffer(), .5f, .0f);
    }

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
        //chara->setDesiredLocomotionVector(dir);
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

    // Spawn rockets
    {/*
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
        time += dt;*/
    }

    {
        // Sphere cast
        static gfxm::vec3 from = gfxm::vec3(0, 3, 0);
        static gfxm::vec3 to = gfxm::vec3(0,0,0);
        static float radius = .5f;
        if (inputSphereCast->isPressed()) {
            int mx, my;
            platformGetMousePos(&mx, &my);
            gfxm::rect rc = platformGetViewportRect();
            gfxm::vec2 vp_size(rc.max.x - rc.min.x, rc.max.y - rc.min.y);
            my = vp_size.y - my;
            auto ray = gfxm::ray_viewport_to_world(
                vp_size, gfxm::vec2(mx, my),
                viewport->getProjection(), viewport->getViewTransform()
            );
            from = ray.origin;
            to = ray.origin + ray.direction * 5.f;
        }
        getWorld()->getCollisionWorld()->sphereSweep(from, to, radius);
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

    // Actor anim test
    {
        static ActorSampleBuffer buf;
        static ActorAnimation anim;
        static ActorAnimSampler sampler;
        auto init = [this]()->int {
            buf.initialize(chara_actor.get());
            //buf.setValue("decal.color", gfxm::vec4(.4, .2, 1, 1));
            auto node = anim.createVec4Node("decal.color");
            node->curve_[.0f] = gfxm::vec4(0, 0, 1, 1);
            node->curve_[20.f] = gfxm::vec4(0, 1, 0, 1);
            node->curve_[40.f] = gfxm::vec4(1, 0, 0, 1);
            node->curve_[60.f] = gfxm::vec4(1, 0, 1, 1);
            node->curve_[80.f] = gfxm::vec4(1, 1, 0, 1);
            node->curve_[100.f] = gfxm::vec4(0, 0, 1, 1);
            sampler.init(&anim, &buf);
            return 0;
        };
        static int once = init();
        static float cur = .0f;

        buf.clear_flags();
        sampler.sampleAt(cur);
        buf.apply();

        cur += dt * anim.fps;
        if (cur > anim.length) {
            cur = fmodf(cur, anim.length);
        }
    }


    GameBase::update(dt);
}