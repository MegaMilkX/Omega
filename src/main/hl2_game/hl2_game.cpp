#include "hl2_game.hpp"

#include "engine_runtime/components/render_view_list.hpp"
#include "world/node/node_character_capsule.hpp"
#include "agents/fps_player_agent.hpp"
#include "world/common_systems/player_start_system.hpp"

#include "collision/shape/box.hpp"

void HL2GameInstance::onInit(IEngineRuntime* rt) {
    world.reset(new RuntimeWorld);
    scene_mgr.reset(new SceneManager());
    world->spawn(scene_mgr.get());

    primary_view.reset(new EngineRenderView(gfxm::rect(0, 0, 1, 1), 0, 0, false));
    primary_player.reset(new LocalPlayer(primary_view.get(), 0));

    primary_view->setRenderTarget(gpuGetDefaultRenderTarget());    
    if(auto list = rt->getComponent<RenderViewList>()) {
        list->push_back(primary_view.get());
    }

    playerAdd(primary_player.get());
    playerSetPrimary(primary_player.get());

    scene_mgr->loadScene<HL2Scene>(
        //"experimental/hl2/maps/d2_coast_08.bsp"
        //"experimental/hl2/maps/d2_coast_03.bsp"
        //"experimental/hl2/maps/d1_town_01.bsp"
        //"experimental/hl2/maps/d2_prison_02.bsp"
        //"experimental/hl2/maps/d1_trainstation_01.bsp"
        //"experimental/hl2/maps/d1_trainstation_02.bsp"
        //"experimental/hl2/maps/d1_trainstation_03.bsp"
        //"experimental/hl2/maps/d1_trainstation_04.bsp"
        //"experimental/hl2/maps/d1_trainstation_05.bsp"
        //"experimental/hl2/maps/d1_trainstation_06.bsp"
        //"experimental/hl2/maps/d1_canals_07.bsp"
        //"experimental/hl2/maps/d1_canals_13.bsp"
        //"experimental/hl2/maps/d1_eli_01.bsp"
        //"experimental/hl2/maps/test.bsp"
        "experimental/hl2/maps/collision_test.bsp"
        //"experimental/hl2/maps/l4d_vs_hospital01_apartment.bsp"
        //"bsp/q1/e1m1.bsp"
    );

    {
        phyWorld* phy_world = getWorld()->getSystem<phyWorld>();
        phyRigidBody* body0 = new phyRigidBody;
        phyRigidBody* body1 = new phyRigidBody;
        phyRigidBody* body2 = new phyRigidBody;
        phyBoxShape* shape0 = new phyBoxShape;
        phyBoxShape* shape1 = new phyBoxShape;
        phyBoxShape* shape2 = new phyBoxShape;
        shape0->half_extents = gfxm::vec3(.2f, .2f, .2f);
        shape1->half_extents = gfxm::vec3(.2f, .2f, .2f);
        shape2->half_extents = gfxm::vec3(.2f, .2f, .2f);
        body0->setShape(shape0);
        body0->setPosition(gfxm::vec3(0, 3, 3));
        body0->mass = 30.0f;
        body0->collision_group |= COLLISION_LAYER_DEFAULT;
        body1->setShape(shape1);
        body1->setPosition(gfxm::vec3(0, 2, 3));
        body1->mass = 30.0f;
        body1->collision_group |= COLLISION_LAYER_DEFAULT;
        body2->setShape(shape2);
        body2->setPosition(gfxm::vec3(0, 1, 3));
        body2->mass = 30.0f;
        body2->collision_group |= COLLISION_LAYER_DEFAULT;
        phyJoint* joint0 = new phyJoint(
            nullptr, body0, gfxm::vec3(0, 3.5f, 3), gfxm::mat3(1.f)
        );
        phyJoint* joint1 = new phyJoint(
            body0, body1, gfxm::vec3(0, 2.5f, 3), gfxm::mat3(1.f)
        );
        //joint0->linear_mask = gfxm::vec3(1, 1, 0);
        phyJoint* joint2 = new phyJoint(
            body1, body2, gfxm::vec3(0, 1.5f, 3), gfxm::mat3(1.f)
        );

        phy_world->addCollider(body0);
        phy_world->addCollider(body1);
        phy_world->addCollider(body2);
        phy_world->addJoint(joint0);
        phy_world->addJoint(joint1);
        phy_world->addJoint(joint2);
    }

    //gpuGetPipeline()->enableTechnique("EnvironmentIBL", false);
    //gpuGetPipeline()->enableTechnique("Posteffects/GammaTonemap", false);
    
    // TODO: Should be loaded from config file
    // Input: bind actions and ranges
    inputCreateActionDesc("C")
        .linkKey(Key.Keyboard.C, 1.f);
    inputCreateActionDesc("V")
        .linkKey(Key.Keyboard.V, 1.f);
    inputCreateActionDesc("Z")
        .linkKey(Key.Keyboard.Z, 1.f);
    inputCreateActionDesc("X")
        .linkKey(Key.Keyboard.X, 1.f);
    inputCreateActionDesc("ToggleWireframe")
        .linkKey(Key.Keyboard.I, 1.f);
    inputCreateActionDesc("Recover")
        .linkKey(Key.Keyboard.Q, 1.f);
    inputCreateActionDesc("SphereCast")
        .linkKey(Key.Keyboard.Z, 1.f);
    inputCreateActionDesc("CharacterInteract")
        .linkKey(Key.Keyboard.E, 1.f);
    inputCreateActionDesc("Shoot")
        .linkKey(Key.Mouse.BtnLeft, 1.f);
    inputCreateActionDesc("ShootAlt")
        .linkKey(Key.Mouse.BtnRight, 1.f);
    inputCreateActionDesc("Sprint")
        .linkKey(Key.Keyboard.LeftShift, 1.f);
    inputCreateActionDesc("Jump")
        .linkKey(Key.Keyboard.Space, 1.f);
    inputCreateActionDesc("Crouch")
        .linkKey(Key.Keyboard.LeftControl, 1.f);

    inputCreateRangeDesc("CharacterLocomotion")
        .linkKeyX(Key.Keyboard.A, -1.0f)
        .linkKeyX(Key.Keyboard.D, 1.0f)
        .linkKeyZ(Key.Keyboard.W, -1.0f)
        .linkKeyZ(Key.Keyboard.S, 1.0f);
    inputCreateRangeDesc("CameraRotation")
        .linkKeyY(Key.Mouse.AxisX, 1.0f)
        .linkKeyX(Key.Mouse.AxisY, 1.0f);
    inputCreateRangeDesc("Scroll")
        .linkKeyX(Key.Mouse.Scroll, -1.0f);

    for (int i = 0; i < 12; ++i) {
        inputCreateActionDesc(MKSTR("F" << (i + 1)).c_str())
            .linkKey(Key.Keyboard.F1 + i, 1.f);
    }
    for (int i = 0; i < 9; ++i) {
        inputCreateActionDesc(MKSTR("_" << i).c_str())
            .linkKey(Key.Keyboard._0 + i, 1.f);
    }

    {
        fps_player_actor.setFlags(ACTOR_FLAG_UPDATE);
        auto capsule = fps_player_actor.setRoot<CharacterCapsuleNode>("capsule");
        capsule->shape.height = 1.2f;
        capsule->shape.radius = .2f;
        capsule->collider.setCenterOffset(gfxm::vec3(.0f, .8f + .2f, .0f));
        capsule->collider.collision_group
            = COLLISION_LAYER_CHARACTER;
        capsule->createChild<EmptyNode>("head");
        fps_player_actor.addDriver<FpsCharacterDriver>();
        //fps_player_actor.addController<FpsCameraController>();

        //fps_player_actor.setTranslation(hl2scene.player_origin);
        //fps_player_actor.setRotation(hl2bspmodel.player_orientation);
        //fps_player_actor.getDriver<FpsCharacterDriver>()->setOrientation(hl2scene.player_orientation);

        getWorld()->spawn(&fps_player_actor);
        if (auto sys = getWorld()->getSystem<PlayerStartSystem>()) {
            if (!sys->points.empty()) {
                fps_player_actor.setTranslation(sys->points[0].origin);
                //fps_player_actor.setRotation(sys->points[0].rotation);
                fps_player_actor.getDriver<FpsCharacterDriver>()->setOrientation(sys->points[0].rotation);
            } else {
                LOG_ERR("No player start points available");
            }
        } else {
            LOG_ERR("Hl2GameInstance: PlayerStartSystem not found");
        }
    }

    playerGetPrimary()->clearRoles();
    playerGetPrimary()->addRole<FpsPlayerController>(*getWorld(), &fps_player_actor);
    playerGetPrimary()->addRole<FpsSpectator>(*getWorld(), &fps_player_actor);

    auto input_state = playerGetPrimary()->getInputState();
    input_state->pushContext(&input_ctx);

    inputRecover = input_ctx.createAction("Recover");
    inputToggleWireframe = input_ctx.createAction("ToggleWireframe");
    inputStepPhysics = input_ctx.createAction("Z");
    inputRunPhysics = input_ctx.createAction("X");
    for (int i = 0; i < 12; ++i) {
        inputFButtons[i] = input_ctx.createAction(MKSTR("F" << (i + 1)).c_str());
    }
    for (int i = 0; i < 9; ++i) {
        inputNumButtons[i] = input_ctx.createAction(MKSTR("_" << i).c_str());
    }
}
void HL2GameInstance::onCleanup() {
    world.reset();
}

extern bool dbg_stepPhysics;
void HL2GameInstance::onUpdate(float dt) {
    LocalPlayer* local_player = dynamic_cast<LocalPlayer*>(playerGetPrimary());
    assert(local_player);
    EngineRenderView* viewport = local_player->getViewport();
    assert(viewport);
    gpuRenderTarget* render_target = viewport->getRenderTarget();

    if (inputRecover->isJustPressed()) {
        if (auto sys = getWorld()->getSystem<PlayerStartSystem>()) {
            if (!sys->points.empty()) {
                static int i = 0;
                const auto& pt = sys->points[i++];
                i = i % sys->points.size();
                fps_player_actor.setTranslation(pt.origin);
                //fps_player_actor.setRotation(pt.rotation);
                fps_player_actor.getDriver<FpsCharacterDriver>()->setOrientation(pt.rotation);
            } else {
                LOG_ERR("No player start points available");
            }
        } else {
            LOG_ERR("Hl2GameInstance: PlayerStartSystem not found");
        }
    }

    if (inputStepPhysics->isJustPressed()) {
        dbg_stepPhysics = true;
    }
    if (inputRunPhysics->isPressed()) {
        dbg_stepPhysics = true;
    }
    if (inputToggleWireframe->isJustPressed()) {
        render_target->dbg_drawWireframe = !render_target->dbg_drawWireframe;
    }
    if (inputFButtons[0]->isJustPressed()) {
        render_target->setDefaultOutput("Final", RT_OUTPUT_RGB);
    } else if (inputFButtons[1]->isJustPressed()) {
        render_target->setDefaultOutput("Albedo", RT_OUTPUT_RGB);
    } else if (inputFButtons[2]->isJustPressed()) {
        render_target->setDefaultOutput("Position", RT_OUTPUT_RGB);
    } else if (inputFButtons[3]->isJustPressed()) {
        render_target->setDefaultOutput("Normal", RT_OUTPUT_RGB);
    } else if (inputFButtons[4]->isJustPressed()) {
        render_target->setDefaultOutput("Metalness", RT_OUTPUT_RRR);
    } else if (inputFButtons[5]->isJustPressed()) {
        render_target->setDefaultOutput("Roughness", RT_OUTPUT_RRR);
    } else if (inputFButtons[6]->isJustPressed()) {
        render_target->setDefaultOutput("VelocityMap", RT_OUTPUT_RGB);
    } else if (inputFButtons[7]->isJustPressed()) {
        render_target->setDefaultOutput("Lightness", RT_OUTPUT_RGB);
    } else if (inputFButtons[8]->isJustPressed()) {
        render_target->setDefaultOutput("Depth", RT_OUTPUT_DEPTH);
    } else if (inputFButtons[10]->isJustPressed()) {
        render_target->setDefaultOutput("AmbientOcclusion", RT_OUTPUT_RRR);
    }

    if(inputNumButtons[0]->isJustPressed()) {
        static bool dbg_enableCollisionDbgDraw = false;
        dbg_enableCollisionDbgDraw = !dbg_enableCollisionDbgDraw;
        getWorld()->getSystem<phyWorld>()->enableDbgDraw(dbg_enableCollisionDbgDraw);
    }
    // TODO:

    world->update(dt);
}
void HL2GameInstance::onDraw(float dt) {
    /*
    LocalPlayer* local_player = dynamic_cast<LocalPlayer*>(playerGetPrimary());
    assert(local_player);
    EngineRenderView* viewport = local_player->getViewport();
    assert(viewport);
    gpuRenderBucket* render_bucket = viewport->getRenderBucket();
    gpuRenderTarget* render_target = viewport->getRenderTarget();
    
    hl2scene.draw(render_bucket);*/
}


void HL2GameInstance::onPlayerJoined(IPlayer* player) {
    LOG("IGameInstance: onPlayerJoined");
    LocalPlayer* local = dynamic_cast<LocalPlayer*>(player);
    if (!local) {
        return;
    }
    assert(local->getViewport());

    Camera* cam = new Camera;
    world->spawn(cam);
    local->getViewport()->setCamera(cam);
    cam->setZNear(.01f);
    cam->setZFar(1000.f);
}
void HL2GameInstance::onPlayerLeft(IPlayer* player) {
    LOG("IGameInstance: onPlayerLeft");
    LocalPlayer* local = dynamic_cast<LocalPlayer*>(player);
    if (!local) {
        return;
    }
    assert(local->getViewport());

    auto cam = local->getViewport()->getCamera();
    world->despawn(cam);
    delete cam;
}