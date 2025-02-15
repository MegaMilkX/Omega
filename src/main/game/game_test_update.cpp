#include "game/game_test.hpp"
#include "engine.hpp"
#include "world/experimental/actor_anim.hpp"

void GameTest::update(float dt) {
    LocalPlayer* local_player = dynamic_cast<LocalPlayer*>(playerGetPrimary());
    assert(local_player);
    Viewport* viewport = local_player->getViewport();
    assert(viewport);
    gpuRenderTarget* render_target = viewport->getRenderTarget();

    fps_label->setCaption(
        std::format(
            "Frame time (no vsync): {:.3f}ms\
\nLeftover: {:.3f}ms\
\nFrame time: {:.3f}ms\
\nRender: {:.3f}ms\
\nCollision: {:.3f}ms\
\nAudio: {:.3f}ms\
\nFPS: {:.1f}",
            engineGetStats().frame_time_no_vsync * 1000.f,
            (engineGetStats().frame_time - engineGetStats().frame_time_no_vsync) * 1000.f,
            engineGetStats().frame_time * 1000.f,
            engineGetStats().render_time * 1000.f,
            engineGetStats().collision_time * 1000.f,
            audioGetStats().buffer_update_time.load() * 1000.f,
            engineGetStats().fps
        ).c_str()
    );

    if (inputRecover->isJustPressed()) {
        //chara_actor->getRoot()->setTranslation(gfxm::vec3(0, 0, 0));
        chara_actor->getRoot()->setTranslation(fps_player_actor.getRoot()->getTranslation());
    }

    if (inputToggleWireframe->isJustPressed()) {
        render_target->dbg_drawWireframe = !render_target->dbg_drawWireframe;
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
    } else if (inputFButtons[10]->isJustPressed()) {
        render_target->setDefaultOutput("AmbientOcclusion");
    }

    static int render_range_min = 0;
    static int render_range_max = INT_MAX;
    if (inputC->isJustPressed()) {
        render_range_min--;
    } else if(inputV->isJustPressed()) {
        render_range_max++;
    }
    render_target->setDebugRenderGeometryRange(render_range_min, render_range_max);

    if (inputNumButtons[1]->isJustPressed()) {
        playerGetPrimary()->clearAgents();
        playerLinkAgent(playerGetPrimary(), chara_actor.get());
        playerLinkAgent(playerGetPrimary(), &tps_camera_actor);

        ActorNode* n = chara_actor->findNode<EmptyNode>("cam_target");
        if (!n) {
            n = chara_actor->getRoot();
        }
        tps_camera_actor.getController<CameraTpsController>()
            ->setTarget(n->getTransformHandle());

        audioPlayOnce(clip_whsh->getBuffer(), .5f, .0f);
    } else if(inputNumButtons[2]->isJustPressed()) {
        playerGetPrimary()->clearAgents();
        playerLinkAgent(playerGetPrimary(), chara_actor.get());
        audioPlayOnce(clip_whsh->getBuffer(), .5f, .0f);
    } else if(inputNumButtons[3]->isJustPressed()) {
        playerGetPrimary()->clearAgents();
        playerLinkAgent(playerGetPrimary(), &free_camera_actor);
        audioPlayOnce(clip_whsh->getBuffer(), .5f, .0f);
    } else if(inputNumButtons[4]->isJustPressed()) {
        playerGetPrimary()->clearAgents();
        // PLAYER_LINK_TYPE::ALL
        // PLAYER_LINK_TYPE::INPUT
        // PLAYER_LINK_TYPE::VIEWPORT
        // PLAYER_LINK_TYPE::AUDIO
        // or this way:
        // SPECTATE, CONTROL
        playerLinkAgent(playerGetPrimary(), &fps_player_actor);
        audioPlayOnce(clip_whsh->getBuffer(), .5f, .0f);
    } else if(inputNumButtons[5]->isJustPressed()) {
        if (chara_actor_2->get_type().is_derived_from(type_get<PlayerAgentActor>())) {
            playerGetPrimary()->clearAgents();
            playerLinkAgent(playerGetPrimary(), (PlayerAgentActor*)chara_actor_2.get());
            playerLinkAgent(playerGetPrimary(), &tps_camera_actor);

            ActorNode* n = chara_actor_2->findNode<EmptyNode>("cam_target");
            if (!n) {
                n = chara_actor_2->getRoot();
            }
            tps_camera_actor.getController<CameraTpsController>()
                ->setTarget(n->getTransformHandle());

            audioPlayOnce(clip_whsh->getBuffer(), .5f, .0f);
        } else {
            LOG_ERR("Actor is not a PlayerAgentActor");
        }
    } else if(inputNumButtons[6]->isJustPressed()) {
        playerGetPrimary()->clearAgents();
        playerLinkAgent(playerGetPrimary(), &demo_camera_actor);
        audioPlayOnce(clip_whsh->getBuffer(), .5f, .0f);
    } else if(inputNumButtons[0]->isJustPressed()) {
        static bool dbg_enableCollisionDbgDraw = false;
        dbg_enableCollisionDbgDraw = !dbg_enableCollisionDbgDraw;
        getWorld()->getCollisionWorld()->enableDbgDraw(dbg_enableCollisionDbgDraw);
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
        static gfxm::vec3 target_pos;
        static float delay_buf = .0f;
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

        chara2->setDesiredLocomotionVector(normal_to_target);
        /*
        bool is_target_visible = dot > FLT_EPSILON;
        if (is_target_visible && distance > 2.f) {
            chara2->setDesiredLocomotionVector(normal_to_target);
        } else {
            chara2->setDesiredLocomotionVector(gfxm::vec3(0, 0, 0));
        }*/
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

    dbgDrawText(gfxm::vec3(0, 2, 0), "Hello, World!");

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
            //from = ray.origin;
            //to = from + gfxm::vec3(.0f, -3.f, .0f);
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
            anim.fps = 120.f;
            buf.initialize(chara_actor.get());
            //buf.setValue("decal.color", gfxm::vec4(.4, .2, 1, 1));
            auto node = anim.createVec4Node("decal.color");/*
            node->curve_[.0f] = gfxm::vec4(0, 0, 1, 1);
            node->curve_[20.f] = gfxm::vec4(0, 1, 0, 1);
            node->curve_[40.f] = gfxm::vec4(1, 0, 0, 1);
            node->curve_[60.f] = gfxm::vec4(1, 0, 1, 1);
            node->curve_[80.f] = gfxm::vec4(1, 1, 0, 1);
            node->curve_[100.f] = gfxm::vec4(0, 0, 1, 1);*/
            node->curve_[.0f] = gfxm::vec4(1, 1, 1, 1);
            node->curve_[50.f] = gfxm::vec4(1, 1, 1, 0);
            node->curve_[100.f] = gfxm::vec4(1, 1, 1, 1);
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

    // Flowy bones test
    {
        const int BONE_COUNT = 10;
        static std::vector<Handle<TransformNode>> bones;
        static std::vector<gfxm::vec3> points;
        static std::vector<gfxm::vec3> velocities;
        const gfxm::vec3 origin(-8.5, 2, 3);

        auto init = []()->int {
            bones.resize(BONE_COUNT);
            points.resize(BONE_COUNT);
            velocities.resize(BONE_COUNT);
            for (int i = 0; i < BONE_COUNT; ++i) {
                bones[i] = HANDLE_MGR<TransformNode>::acquire();
            }
            bones[0]->setTranslation(-7, 2, 3);
            for (int i = 1; i < BONE_COUNT; ++i) {
                transformNodeAttach(bones[i - 1], bones[i]);
                bones[i]->setTranslation(0, -.2, 0);
            }
            return 0;
        };
        static int a = init();
        
        static float t = .0f;
        t += dt;
        for (int i = 0; i < BONE_COUNT; ++i) {
            points[i] = bones[i]->getWorldTranslation();
        }
        bones[0]->setTranslation(origin + gfxm::vec3(sinf(t * 5.f) * .5f, 0, cosf(t * 5.f) * .5f));
        points[0] = bones[0]->getWorldTranslation();
        for (int i = 1; i < BONE_COUNT; ++i) {
            //velocities[i] += gfxm::vec3(0, -9.8f, 0) * dt;
            //velocities[i].y = gfxm::_min(10.f, velocities[i].y);
            points[i] += gfxm::vec3(0, -1.8f, 0) * dt;
        }
        for (int i = 1; i < BONE_COUNT; ++i) {
            const gfxm::vec3 a = points[i - 1];
            const gfxm::vec3 b = points[i];
            const gfxm::vec3 V = b - a;
            const gfxm::vec3 N = normalize(V);
            const float len = V.length();
            if (len > .2f) {
                //velocities[i] += -N * len * dt;
                points[i] = a + N * .2f;
                //velocities[i] = gfxm::vec3(0,0,0);
            }
        }
        for (int i = 1; i < BONE_COUNT; ++i) {
            //points[i] += velocities[i] * dt;
        }

        for (int i = 1; i < BONE_COUNT; ++i) {
            const gfxm::mat4& inv_parent = gfxm::inverse(bones[i - 1]->getWorldTransform());
            gfxm::vec4 p4 = inv_parent * gfxm::vec4(points[i], 1);
            bones[i]->setTranslation(gfxm::vec3(p4));
        }
        for (int i = 1; i < BONE_COUNT; ++i) {
            dbgDrawLine(bones[i - 1]->getWorldTranslation(), bones[i]->getWorldTranslation(), DBG_COLOR_WHITE);
            
            dbgDrawLine(bones[i - 1]->getWorldTranslation(), bones[i - 1]->getWorldTranslation() + bones[i - 1]->getWorldRight() * .1f, DBG_COLOR_RED);
            dbgDrawLine(bones[i - 1]->getWorldTranslation(), bones[i - 1]->getWorldTranslation() + bones[i - 1]->getWorldUp() * .1f, DBG_COLOR_GREEN);
            dbgDrawLine(bones[i - 1]->getWorldTranslation(), bones[i - 1]->getWorldTranslation() + bones[i - 1]->getWorldBack() * .1f, DBG_COLOR_BLUE);
        }
    }


    GameBase::update(dt);
}