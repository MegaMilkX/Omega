#include "game/game_test.hpp"
#include "engine.hpp"
#include "world/experimental/actor_anim.hpp"
#include "world/controller/material_controller.hpp"

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
        //chara_actor->getRoot()->setTranslation(fps_player_actor.getRoot()->getTranslation());

        static bool enabled = true;
        enabled = !enabled;
        
        chara_actor->getController<MaterialController>()
            ->enableTechnique("Outline", enabled);

        //renderable2->enableMaterialTechnique("Outline", enabled);
        
        //gpuGetPipeline()->enableTechnique("EnvironmentIBL", enabled);
        gpuGetPipeline()->enableTechnique("Skybox", enabled);
        gpuGetPipeline()->enableTechnique("Fog", !enabled);
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
        static float time = .0f;
        time += dt;
        float x = cosf(time) * 5.f;
        float z = sinf(time) * 5.f;
        test_dcl->getTransformNode()->setTranslation(x, .0f, z);
        test_dcl->getTransformNode()->setRotation(gfxm::angle_axis(time, gfxm::vec3(.0f, 1.f, .0f)));
    }

#if 0
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
#endif
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

#if 1
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
#endif

    // Dynamic bones test
    if(0) {
        struct POINT {
            gfxm::vec3 pos;
            gfxm::vec3 prev_pos;
            gfxm::quat rot;
            gfxm::quat prev_rot;

            // Apply along the Pparent to P vector
            //float linear_velo;
            // Apply around x and y of the parent?
            //gfxm::vec2 angular_velo;
            // Apply around z of the parent?
            // float twist_velo

            gfxm::vec3 velo = gfxm::vec3(0, 0, 0);
            gfxm::vec3 velo_angular = gfxm::vec3(0, 0, 0);
            gfxm::vec3 accel_angular_propagate = gfxm::vec3(0, 0, 0);

            gfxm::vec3 lcl_rest_position;
            gfxm::quat rest_quat; // local
            float rest_length = 0;
            gfxm::vec3 dbg_axis;
            float dbg_angle;

            gfxm::vec3 world_target;
            float compensation_angle = .0f;
        };
        auto fn_init_points = [](POINT* points, int count, const gfxm::vec3& origin, int shape = 0)->int {
            for (int i = 1; i < count; ++i) {
                points[i] = POINT();
            }
            if (shape == 1) {
                for (int i = 1; i < count; ++i) {
                    float a = (i + 1) / float(count);
                    float b = a * 1.f;
                    float c = a * 3.f;
                    points[i].pos = origin + gfxm::vec3(b, -sinf(c) * .3f, 0);
                    points[i].prev_pos = points[i].pos;
                }
            } else if (shape == 2) {
                for (int i = 1; i < count; ++i) {
                    float a = (i + 1) / float(count);
                    float b = a * 1.f;
                    float c = a * 2.f;
                    points[i].pos = origin + gfxm::vec3(-b, -sinf(c) * .3f, 0);
                    points[i].prev_pos = points[i].pos;
                }
            } else {
                for (int i = 1; i < count; ++i) {
                    float a = (i + 1) / float(count);
                    float b = a * 1.f;
                    //float c = a * 2.f;
                    //points[i].pos = origin + gfxm::vec3(0, -sinf(c) * .3f, -b);
                    float c = a * 3.f;
                    points[i].pos = origin + gfxm::vec3(0, sinf(c) * .3f, -b);
                    points[i].prev_pos = points[i].pos;
                }
            }
            for (int i = 1; i < count - 1; ++i) {
                gfxm::vec3 backward = gfxm::normalize(points[i + 1].pos - points[i].pos);
                gfxm::vec3 up = gfxm::vec3(0, 1, 0);
                const gfxm::vec3 right = gfxm::normalize(gfxm::cross(up, backward));
                up = gfxm::normalize(gfxm::cross(backward, right));
                gfxm::mat3 orient(1.f);
                orient[0] = gfxm::vec4(right, 0);
                orient[1] = gfxm::vec4(up, 0);
                orient[2] = gfxm::vec4(backward, 0);
                points[i].rot = gfxm::to_quat(orient);
            }
            int ilast = count - 1;
            points[ilast].rot = points[ilast - 1].rot;

            for (int i = 1; i < count; ++i) {
                gfxm::mat4 mparent = gfxm::translate(gfxm::mat4(1.f), points[i - 1].pos)
                    * gfxm::to_mat4(points[i - 1].rot);
                points[i].lcl_rest_position = gfxm::inverse(mparent) * gfxm::vec4(points[i].pos, 1.f);

                points[i].world_target = points[i].pos;
            }

            for (int i = 1; i < count; ++i) {
                points[i].rest_length = gfxm::length(points[i - 1].pos - points[i].pos);
                points[i].rest_quat 
                    = gfxm::inverse(points[i - 1].rot)
                    * points[i].rot;
            }

            return 0;
        };

#define USE_FALLOFF 1
#define USE_PROPAGATION 1
#define USE_ENERGY_SHARING 1
#define USE_GRAVITY 1

        static const float TERMINAL_ANGULAR_VELO = 20.f;
        auto fn_update_points2 = [this](POINT* points, int count, float dt) {
            dt *= rope_time_scale;
            const gfxm::vec3 root_pos_delta = points[0].pos - points[0].prev_pos;
            const gfxm::quat root_rot_delta = gfxm::inverse(points[0].prev_rot) * points[0].rot;

            // Store previous transform
            for (int i = 0; i < count; ++i) {
                points[i].prev_rot = points[i].rot;
                points[i].prev_pos = points[i].pos;
                points[i].accel_angular_propagate = gfxm::vec3(0,0,0);
            }

            // Adjust to rest distance
            for (int i = 1; i < count; ++i) {
                POINT& P0 = points[i - 1];
                POINT& P1 = points[i];
                float rest_length = P1.rest_length;
                const gfxm::vec3 V = P1.pos - P0.pos;
                const gfxm::vec3 N = gfxm::normalize(V);
                P1.pos = P0.pos + gfxm::normalize(V) * rest_length;
            }

            // Rotational motion
            for (int i = 1; i < count; ++i) {
                POINT& P0 = points[i - 1];
                POINT& P1 = points[i];

                const float min_falloff = .1f;
                float falloff 
                    = min_falloff 
                    + (1.f - gfxm::_min(1.f, (i + 1) / float(count))) 
                    * (1.0f - min_falloff);
                falloff *= falloff;

                gfxm::mat4 mparent = gfxm::translate(gfxm::mat4(1.f), P0.pos)
                    * gfxm::to_mat4(P0.rot);
                gfxm::vec3 world_target = mparent * gfxm::vec4(P1.lcl_rest_position, 1.f);

                gfxm::vec3 A = P1.pos - P0.pos;
                gfxm::vec3 B = world_target - P0.pos;
                gfxm::vec3 axis = gfxm::normalize(gfxm::cross(gfxm::normalize(A), gfxm::normalize(B)));
                if (!axis.is_valid()) {
                    continue;
                }
                float angle = acosf(gfxm::clamp(fabs(gfxm::dot(gfxm::normalize(A), gfxm::normalize(B))), .0f, 1.f));

                {
                    P1.rot = P0.rot * P1.rest_quat;

                    gfxm::quat Qnew = gfxm::inverse(gfxm::angle_axis(angle, axis)) * P1.rot;
                    P1.rot = Qnew;
                }

                // Gravity?
                // NOTE: Only want gravity if there's already a downward motion
                gfxm::vec3 gravity_accel;
                {
                    gfxm::vec3 R = P1.pos - P0.pos;
                    gfxm::vec3 tangent_velo = gfxm::cross(axis, R);
                    float gravity_factor = gfxm::dot(gfxm::vec3(0, -1, 0), gfxm::normalize(tangent_velo));
                    gravity_factor = 9.8f * gfxm::_min(1.f, gfxm::_max(.0f, gravity_factor));
                    gravity_accel = axis * (gfxm::sign(angle) * gravity_factor) * dt;
                }
                float propagation_factor = .5f;
#if !USE_PROPAGATION
                propagation_factor = .0f;
#endif
                float energy_sharing_factor = .85f;
#if !USE_ENERGY_SHARING
                energy_sharing_factor = .0f;
#endif
                float strength = 1250.f;
#if USE_FALLOFF
                strength *= falloff;
#endif
#if !USE_GRAVITY
                gravity_accel *= .0f;
#endif

                float angle_factor = angle;
                gfxm::vec3 angular_accel = axis * angle * dt * strength + gravity_accel;
                angular_accel += P0.accel_angular_propagate;
                gfxm::vec3 angular_accel_self = angular_accel * (1.f - propagation_factor);
                gfxm::vec3 angular_accel_propagate = angular_accel * propagation_factor;
                P1.velo_angular += angular_accel_self;
                P0.velo_angular -= angular_accel_self * energy_sharing_factor;
                P1.accel_angular_propagate = angular_accel_propagate;

                P1.velo_angular *= powf(.1f, dt);

                // Limit angular velocity
                {
                    float l = P1.velo_angular.length();
                    l = gfxm::_min(TERMINAL_ANGULAR_VELO, l);
                    P1.velo_angular = gfxm::normalize(P1.velo_angular) * l;
                }

                while(1) {
                    float va_len = P1.velo_angular.length();
                    if (va_len < FLT_EPSILON) {
                        break;
                    }
                    gfxm::vec3 axis = P1.velo_angular / va_len;
                    float angle = va_len * dt;

                    gfxm::quat q = gfxm::angle_axis(angle, axis);
                    gfxm::vec3 Plcl = P1.pos - P0.pos;
                    gfxm::vec3 Pnew = gfxm::to_mat4(q) * gfxm::vec4(Plcl, 1.f);
                    Pnew = P0.pos + Pnew;
                    P1.world_target = Pnew;

                    P1.pos = Pnew;

                    {
                        gfxm::quat Qnew = q * P1.rot;
                        P1.rot = Qnew;
                    }

                    P1.world_target = Pnew;
                    break;
                }

                // Limit rotation
                {
                    const float BEND_X_LIMIT = gfxm::pi * .1f;
                    const float BEND_Y_LIMIT = gfxm::pi * .2f;
                    const float TWIST_Z_LIMIT = gfxm::pi * .05f;

                    gfxm::mat4 mparent = gfxm::translate(gfxm::mat4(1.f), P0.pos)
                        * gfxm::to_mat4(P0.rot);
                    gfxm::vec3 world_target = mparent * gfxm::vec4(P1.lcl_rest_position, 1.f);

                    gfxm::vec3 A = gfxm::inverse(mparent) * gfxm::vec4(P1.pos, 1.f);
                    gfxm::vec3 B = P1.lcl_rest_position;
                    gfxm::vec3 axis = gfxm::normalize(gfxm::cross(gfxm::normalize(B), gfxm::normalize(A)));
                    if (!axis.is_valid()) {
                        continue;
                    }
                    float angle = acosf(gfxm::clamp(fabs(gfxm::dot(gfxm::normalize(B), gfxm::normalize(A))), .0f, 1.f));

                    gfxm::quat q_original = gfxm::angle_axis(angle, axis);
                    gfxm::vec3 angular_offset = axis * angle;
                    gfxm::mat3 P0_orient = gfxm::to_mat3(P0.rot);

                    float angle_x = gfxm::dot(angular_offset, gfxm::vec3(1, 0, 0));
                    float angle_y = gfxm::dot(angular_offset, gfxm::vec3(0, 1, 0));
                    float angle_z = gfxm::dot(angular_offset, gfxm::vec3(0, 0, 1));
                    P1.dbg_angle = angle_x;

                    gfxm::vec2 bend_lim2(angle_x, angle_y);
                    bend_lim2 = gfxm::normalize(bend_lim2);
                    bend_lim2 = gfxm::vec2(fabs(bend_lim2.x * BEND_X_LIMIT), fabs(bend_lim2.y * BEND_Y_LIMIT));

                    angle_x = gfxm::clamp(angle_x, -bend_lim2.x, bend_lim2.x);
                    angle_y = gfxm::clamp(angle_y, -bend_lim2.y, bend_lim2.y);
                    angle_z = gfxm::clamp(angle_z, -TWIST_Z_LIMIT, TWIST_Z_LIMIT);

                    gfxm::vec3 angular_offset_limited
                        = gfxm::vec3(angle_x, 0, 0)
                        + gfxm::vec3(0, angle_y, 0)
                        + gfxm::vec3(0, 0, angle_z);

                    gfxm::quat q = gfxm::angle_axis(angular_offset_limited.length(), gfxm::normalize(angular_offset_limited));

                    {
                        gfxm::vec3 Plcl = P1.lcl_rest_position;
                        gfxm::vec3 Pnew = gfxm::to_mat4(q) * gfxm::vec4(Plcl, 1.f);
                        Pnew = mparent * gfxm::vec4(Pnew, 1.f);
                        P1.pos = Pnew;

                        P1.rot = P0.rot * q * P1.rest_quat;

                        P1.dbg_axis = angular_offset_limited;
                    }
                }
            }

            // Apply angular velocities
            for (int i = 1; i < count; ++i) {
                POINT& P0 = points[i - 1];
                POINT& P1 = points[i];
                /*
                while(1) {
                    float va_len = P1.velo_angular.length();
                    if (va_len < FLT_EPSILON) {
                        break;
                    }
                    gfxm::vec3 axis = P1.velo_angular / va_len;
                    float angle = va_len * dt;

                    gfxm::quat q = gfxm::angle_axis(angle, axis);
                    gfxm::vec3 Plcl = P1.pos - P0.pos;
                    gfxm::vec3 Pnew = gfxm::to_mat4(q) * gfxm::vec4(Plcl, 1.f);
                    Pnew = P0.pos + Pnew;
                    P1.world_target = Pnew;

                    P1.pos = Pnew;
                    P1.rot = P0.rot * P1.rest_quat;

                    P1.world_target = Pnew;
                    break;
                }*/
            }

            // Angular velocity damping
            for (int i = 1; i < count; ++i) {
                POINT& P1 = points[i];
                //P1.velo_angular *= powf(.1f, dt);
            }
        };
        
        auto fn_debug_draw = [](POINT* points, int count) {
            for (int i = 0; i < count - 1; ++i) {
                //float a = i / float(N_POINTS);
                //a = sqrtf(a);
                float b = gfxm::_min(1.f, points[i].velo.length() * .2f);
                float a = 1.0f - b;
                uint64_t col = gfxm::hsv2rgb32(a, gfxm::_min(1.f, gfxm::sqrt(b)), 1);
                
                dbgDrawLine(points[i].pos, points[i + 1].pos, col);
                //dbgDrawLine(points[i].pos, gfxm::vec3(points[i].pos) + points[i].force * .001f, DBG_COLOR_GREEN);
                if(i > 0) {
                    dbgDrawLine(points[i].pos, gfxm::vec3(points[i].pos) + points[i].velo_angular * .1f, DBG_COLOR_GREEN);
                }
            }
            for (int i = 0; i < count; ++i) {
                float b = gfxm::_min(1.f, points[i].velo_angular.length() / TERMINAL_ANGULAR_VELO);
                float a = 1.0f - b;
                uint64_t col = gfxm::hsv2rgb32(a, gfxm::_min(1.f, gfxm::sqrt(b)), 1);

                gfxm::mat4 m = gfxm::translate(gfxm::mat4(1.f), points[i].pos)
                    * gfxm::to_mat4(points[i].rot);
                dbgDrawBox(m, gfxm::vec3(.025f, .01f, .05f), col);
                dbgDrawBox(m, gfxm::vec3(.01f, .025f, .05f), col);
                /*dbgDrawSphere(
                    gfxm::translate(gfxm::mat4(1.f), points[i].pos)
                    * gfxm::to_mat4(points[i].rot),
                    .05f, col
                );*/
            }
            for (int i = 1; i < count; ++i) {
                float b = gfxm::_min(1.f, fabs(points[i].dbg_angle / gfxm::pi));
                float a = 1.0f - b;
                uint64_t col = gfxm::hsv2rgb32(a, gfxm::_min(1.f, gfxm::sqrt(b)), 1);
                //dbgDrawText(points[i].pos, std::format("{:.0f}", gfxm::degrees(points[i].dbg_angle)), DBG_COLOR_RED);
                dbgDrawLine(points[i - 1].pos, points[i].world_target, col);
                dbgDrawSphere(
                    points[i].world_target,
                    .025f, col
                );
            }

            for (int i = 1; i < count; ++i) {
                dbgDrawLine(points[i].pos, points[i].pos + points[i].dbg_axis, DBG_COLOR_WHITE);
            }
        };

        const int N_SIM_STEPS = 1;
        {
            const int N_POINTS = 12;
            static POINT points[N_POINTS];
            auto model = chara_actor->findNode<SkeletalModelNode>("model");
            auto skel_node = model->getBoneProxy("Pelvis");
            gfxm::vec3 offs = gfxm::to_mat4(skel_node->getWorldRotation()) * gfxm::vec4(0, .1, -.08, .0f);
            gfxm::vec3 Porigin = skel_node->getWorldTranslation() + offs;
            gfxm::quat Qorigin = skel_node->getWorldRotation();
            points[0].pos = Porigin;
            points[0].rot = Qorigin;
            if (rope_reset) {
                fn_init_points(points, N_POINTS, Porigin);
            }
            if(rope_is_sim_running || rope_step_once) {
                for(int i = 0; i < N_SIM_STEPS; ++i) {
                    fn_update_points2(points, N_POINTS, dt / float(N_SIM_STEPS));
                }
            }
            fn_debug_draw(points, N_POINTS);
        }
        
        if(1) {
            const int N_POINTS = 12;
            static POINT points[N_POINTS];
            auto model = chara_actor->findNode<SkeletalModelNode>("model");
            auto skel_node = model->getBoneProxy("Spine0");
            gfxm::vec3 offs = gfxm::to_mat4(skel_node->getWorldRotation()) * gfxm::vec4(.15, 0, 0, .0f);
            gfxm::vec3 Porigin = skel_node->getWorldTranslation() + offs;
            gfxm::quat Qorigin = skel_node->getWorldRotation();
            points[0].pos = Porigin;
            points[0].rot = Qorigin;
            if (rope_reset) {
                fn_init_points(points, N_POINTS, Porigin, 1);
            }
            if(rope_is_sim_running || rope_step_once) {
                for(int i = 0; i < N_SIM_STEPS; ++i) {
                    fn_update_points2(points, N_POINTS, dt / float(N_SIM_STEPS));
                }
            }
            fn_debug_draw(points, N_POINTS);
        }

        if(1) {
            const int N_POINTS = 12;
            static POINT points[N_POINTS];
            auto model = chara_actor->findNode<SkeletalModelNode>("model");
            auto skel_node = model->getBoneProxy("Spine0");
            gfxm::vec3 offs = gfxm::to_mat4(skel_node->getWorldRotation()) * gfxm::vec4(.15, 0, 0, .0f);
            gfxm::vec3 Porigin = skel_node->getWorldTranslation() - offs;
            gfxm::quat Qorigin = skel_node->getWorldRotation();
            points[0].pos = Porigin;
            points[0].rot = Qorigin;
            if (rope_reset) {
                fn_init_points(points, N_POINTS, Porigin, 2);
            }
            if(rope_is_sim_running || rope_step_once) {
                for(int i = 0; i < N_SIM_STEPS; ++i) {
                    fn_update_points2(points, N_POINTS, dt / float(N_SIM_STEPS));
                }
            }
            fn_debug_draw(points, N_POINTS);
        }
        if (rope_reset) {
            rope_reset = false;
        }
        if (rope_step_once) {
            rope_step_once = false;
        }
    }

    // Update Actor Inspector properties
    // TODO: Move this
    {
        extern std::vector<std::function<void(void)>> prop_updaters;
        for (auto updater : prop_updaters) {
            updater();
        }
    }

    GameBase::update(dt);
}