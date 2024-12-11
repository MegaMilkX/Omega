
#include "game_test.hpp"
#include "mesh3d/generate_primitive.hpp"

#include "handle/hshared.hpp"

#include "reflection/reflection.hpp"

#include "skeleton/skeleton_editable.hpp"
#include "skeleton/skeleton_instance.hpp"

#include "skeletal_model/skeletal_model.hpp"
#include "import/assimp_load_skeletal_model.hpp"

#include "world/node/node_camera.hpp"
#include "world/node/node_skeletal_model.hpp"
#include "world/node/node_character_capsule.hpp"
#include "world/node/node_decal.hpp"
#include "world/node/node_text_billboard.hpp"
#include "world/component/components.hpp"
#include "world/controller/character_controller.hpp"
#include "world/controller/free_camera_controller.hpp"

#include "game_ui/game_ui.hpp"

#include "gui/gui.hpp"

HSHARED<PlayerAgentActor> createPlayerActor(Actor* tps_camera) {
    HSHARED<PlayerAgentActor> chara_actor;

    chara_actor.reset_acquire();
    chara_actor->setFlags(ACTOR_FLAG_UPDATE);

    auto root = chara_actor->setRoot<CharacterCapsuleNode>("capsule");
    auto node = root->createChild<SkeletalModelNode>("model");
    node->setModel(getSkeletalModel("models/chara_24/chara_24.skeletal_model"));
    auto probe = root->createChild<ProbeNode>("probe");
    probe->setTranslation(0, .5f, .5f);
    probe->shape.radius = 1.f;
    auto decal = root->createChild<DecalNode>("decal");
    decal->setTexture(resGet<gpuTexture2d>("images/character_selection_decal.png"));
    decal->setSize(2, 1, 2);
    type_get<DecalNode>().set_property("color", decal, gfxm::vec4(1, 0, 1, 1));
    auto text = root->createChild<TextBillboardNode>("player_name");
    text->setText("Unknown");
    text->setTranslation(.0f, 1.9f, .0f);
    text->setScale(.5f, .5f, .5f);
    text->setFont(fontGet("fonts/OpenSans-Regular.ttf", 32, 72));
    auto cam_target = root->createChild<EmptyNode>("cam_target");
    cam_target->setTranslation(.0f, 1.4f, .0f);
    /*
    auto particles = root->createChild<ParticleEmitterNode>("particles");
    particles->setEmitter(resGet<ParticleEmitterMaster>("particle_emitters/test_emitter.pte"));
    particles->setTranslation(.0f, 1.f, .0f);
    */
    chara_actor->addController<AnimatorController>();
    chara_actor->addController<CharacterController>();

    AnimatorComponent* anim_comp = chara_actor->addComponent<AnimatorComponent>();
    {
        auto anim_idle = getAnimation("models/chara_24/Idle.anim");
        auto anim_run2 = getAnimation("models/chara_24/Run.anim");
        auto anim_falling = getAnimation("models/chara_24/Falling.anim");
        auto anim_action_opendoor = getAnimation("models/chara_24/Falling.anim");
        auto anim_action_dooropenback = getAnimation("models/chara_24/Falling.anim");
        auto skeleton = getSkeleton("models/chara_24/chara_24.skeleton");
        static RHSHARED<audioSequence> audio_seq;
        audio_seq.reset_acquire();
        audio_seq->length = 40.0f;
        audio_seq->fps = 60.0f;
        audio_seq->insert(0, getAudioClip("audio/sfx/footsteps/asphalt00.ogg"));
        audio_seq->insert(20, getAudioClip("audio/sfx/footsteps/asphalt04.ogg"));
        anim_run2->setAudioSequence(audio_seq);

        static RHSHARED<AnimatorMaster> animator_master;
        animator_master.reset_acquire();
        animator_master->setSkeleton(skeleton);
        animator_master->addParam("velocity");
        animator_master->addParam("is_falling");
        animator_master->addSignal("sig_door_open");
        animator_master->addSignal("sig_door_open_back");
        animator_master->addFeedbackEvent("fevt_door_open_end");
        animator_master
            ->addSampler("idle", "Locomotion", anim_idle)
            .addSampler("run", "Locomotion", anim_run2)
            .addSampler("falling", "Falling", anim_falling)
            .addSampler("open_door_front", "Interact", anim_action_opendoor)
            .addSampler("open_door_back", "Interact", anim_action_dooropenback);
        animUnitFsm* fsm = new animUnitFsm;
        animator_master->setRoot(fsm);
        animFsmState* state_idle = fsm->addState("Idle");
        animFsmState* state_loco = fsm->addState("Locomotion");
        animFsmState* state_fall = fsm->addState("Falling");
        animFsmState* state_door_front = fsm->addState("DoorOpenFront");
        animFsmState* state_door_back = fsm->addState("DoorOpenBack");
        animUnitSingle* unitSingleIdle = new animUnitSingle;
        unitSingleIdle->setSampler("idle");
        state_idle->setUnit(unitSingleIdle);
        {
            //state_loco->setUnit<animUnitSingle>()->setSampler("run");
            animUnitBlendTree* bt = new animUnitBlendTree;
            state_loco->setUnit(bt);
            auto node_blend2 = bt->addNode<animBtNodeBlend2>();
            auto node_clip0 = bt->addNode<animBtNodeClip>();
            auto node_clip1 = bt->addNode<animBtNodeClip>();
            bt->setOutputNode(node_blend2);
            node_clip0->setSampler("idle");
            node_clip1->setSampler("run");
            node_blend2->setInputs(node_clip0, node_clip1);
            node_blend2->setWeightExpression("velocity");
        }
        animUnitSingle* unitSingleFalling = new animUnitSingle;
        unitSingleFalling->setSampler("falling");
        state_fall->setUnit(unitSingleFalling);
        state_fall->onExit("@fevt_door_open_end"); // remove, just testing
        animUnitSingle* unitSingleOpenDoorFront = new animUnitSingle;
        unitSingleOpenDoorFront->setSampler("open_door_front");
        animUnitSingle* unitSingleOpenDoorBack = new animUnitSingle;
        unitSingleOpenDoorBack->setSampler("open_door_back");

        state_door_front->setUnit(unitSingleOpenDoorFront);
        state_door_front->onExit("@fevt_door_open_end");
        state_door_back->setUnit(unitSingleOpenDoorBack);
        state_door_back->onExit("@fevt_door_open_end");
        fsm->addTransition("Idle", "Locomotion", "velocity > .00001", 0.01f);
        fsm->addTransition("Idle", "Falling", "is_falling", 0.15f);
        fsm->addTransition("Locomotion", "Idle", "velocity <= .00001", 0.01f);
        fsm->addTransition("Locomotion", "Falling", "is_falling", 0.15f);
        fsm->addTransition("Falling", "Idle", "is_falling == 0 && velocity <= .00001", 0.15f);
        fsm->addTransition("Falling", "Locomotion", "is_falling == 0 && velocity > .00001", 0.15f);
        fsm->addTransitionAnySource("DoorOpenFront", "sig_door_open", 0.15f);
        fsm->addTransitionAnySource("DoorOpenBack", "sig_door_open_back", 0.15f);
        fsm->addTransition("DoorOpenFront", "Idle", "state_complete", 0.15f);
        fsm->addTransition("DoorOpenBack", "Idle", "state_complete", 0.15f);
        animator_master->compile();

        //animvm::expr_parse("return (2 + 3 * 10) / 8;");
        /*
        animvm::expr_parse(
            "float foo;"
            "float bar;"
            "bar = 13;"
            "foo = bar + 500 = 99;"
            "return bar;"
        );
        animvm::expr_parse("@fevt_door_open_end; return 0;");
        animvm::expr_parse("@anim_end; return 0;");
        animvm::expr_parse("@footstep; return 0;");
        animvm::expr_parse("@shoot; return 0;");
        */
        //anim::vm_test();

        anim_comp->setAnimatorMaster(animator_master);
    }

    chara_actor->getRoot()->setTranslation(gfxm::vec3(-8, 0, 0));
    actorWriteJson(chara_actor.get(), "actors/chara_24.actor");
    chara_actor->getRoot()->setTranslation(gfxm::vec3(-6, 0, 0));

    // Attaching the third person camera to the character
    tps_camera->getController<CameraTpsController>()
        //->setTarget(node->getBoneProxy("Head"));
        ->setTarget(cam_target->getTransformHandle());

    return chara_actor;
}

void GameTest::init() {
    GameBase::init();

    gameuiInit();

    //guiGetRoot()->pushBack(new GuiDemoWindow);
    //guiGetRoot()->pushBack("Hello, World! \nTest");
    fps_label = new GuiLabel("FPS: -");
    fps_label->setStyleClasses({"perf-stats"});
    guiGetRoot()->pushBack(fps_label);

    guiGetStyleSheet().add("perf-stats", {
        gui::background_color(0x99000000),
        gui::border_radius(0, 0, gui::em(1), 0)
    });

    // Input: bind actions and ranges
    inputCreateActionDesc("C")
        .linkKey(Key.Keyboard.C, 1.f);
    inputCreateActionDesc("V")
        .linkKey(Key.Keyboard.V, 1.f);
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
    inputCreateActionDesc("Sprint")
        .linkKey(Key.Keyboard.LeftShift, 1.f);
    inputCreateActionDesc("Jump")
        .linkKey(Key.Keyboard.Space, 1.f);

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
    
    // Input: get action references to actually read them
    auto input_state = playerGetPrimary()->getInputState();
    input_state->pushContext(&input_ctx);

    inputC = input_ctx.createAction("C");
    inputV = input_ctx.createAction("V");
    inputToggleWireframe = input_ctx.createAction("ToggleWireframe");
    inputRecover = input_ctx.createAction("Recover");
    inputSphereCast = input_ctx.createAction("SphereCast");
    for (int i = 0; i < 12; ++i) {
        inputFButtons[i] = input_ctx.createAction(MKSTR("F" << (i + 1)).c_str());
    }
    for (int i = 0; i < 9; ++i) {
        inputNumButtons[i] = input_ctx.createAction(MKSTR("_" << i).c_str());
    }

    //
    {
        ubufTime = gpuGetPipeline()->createUniformBuffer(UNIFORM_BUFFER_TIME);
        gpuGetPipeline()->attachUniformBuffer(ubufTime);
    }

    getWorld()->addSystem<wExplosionSystem>();
    getWorld()->addSystem<wMissileSystem>();

    clip_whsh = getAudioClip("audio/sfx/whsh.ogg");

    tps_camera_actor.setRoot<EmptyNode>("camera");
    tps_camera_actor.addController<CameraTpsController>();
    getWorld()->spawnActor(&tps_camera_actor);

    free_camera_actor.setRoot<EmptyNode>("camera");
    free_camera_actor.addController<FreeCameraController>();
    getWorld()->spawnActor(&free_camera_actor);

    auto snd = ambient_snd_actor.setRoot<SoundEmitterNode>("snd");
    snd->setClip(getAudioClip("audio/amb/amb01.ogg"));
    snd->setLooping(true);
    snd->setAttenuationRadius(20.f);
    getWorld()->spawnActor(&ambient_snd_actor);

    // Monolith sound
    {
        /*
        Actor* actor_snd = new Actor;
        actor_snd->setFlags(ACTOR_FLAG_UPDATE);
        auto snd = actor_snd->setRoot<SoundEmitterNode>("snd");
        snd->setClip(getAudioClip("audio/amb/monolith.ogg"));
        snd->setLooping(true);
        snd->setAttenuationRadius(4.5f);
        actor_snd->setTranslation(gfxm::vec3(0, 2, -35));
        //snd->setTranslation(gfxm::vec3(0, 2, -30));
        //audioSetPosition(snd->getChannelHandle(), gfxm::vec3(0, 2, -30));
        getWorld()->spawnActor(actor_snd);
        */
    }

    {
        Actor* graffiti = new Actor;
        auto decal = graffiti->setRoot<DecalNode>("decal");
        auto tex = resGet<gpuTexture2d>("textures/decals/E_GRF.0039.png");
        decal->setTexture(tex);
        decal->setBlendMode(GPU_BLEND_MODE::NORMAL);
        decal->setSize(3 * tex->getAspectRatio(), 1, 3);
        decal->setTranslation(gfxm::vec3(-10, 0, 10));
        decal->setRotation(gfxm::angle_axis(gfxm::radian(-45.f), gfxm::vec3(0, 1, 0)));
        getWorld()->spawnActor(graffiti);
    }

    {
        Actor* cerberus_pbr = new Actor;
        auto model = cerberus_pbr->setRoot<SkeletalModelNode>("model");
        model->setModel(resGet<mdlSkeletalModelMaster>("models/Cerberus_LP/Cerberus_LP.skeletal_model"));
        cerberus_pbr->setTranslation(gfxm::vec3(-10, 2, 0));
        cerberus_pbr->setScale(gfxm::vec3(5, 5, 5));
        getWorld()->spawnActor(cerberus_pbr);
    }
    {
        Actor* damaged_helmet = new Actor;
        auto model = damaged_helmet->setRoot<SkeletalModelNode>("model");
        model->setModel(resGet<mdlSkeletalModelMaster>("models/DamagedHelmet/glTF-Embedded/damagedhelmet/DamagedHelmet.skeletal_model"));
        damaged_helmet->setTranslation(gfxm::vec3(-15, 2, 0));
        damaged_helmet->setScale(gfxm::vec3(2, 2, 2));
        damaged_helmet->rotate(gfxm::angle_axis(gfxm::degrees(-90.f), gfxm::vec3(1, 0, 0)));
        getWorld()->spawnActor(damaged_helmet);
    }
    {
        Actor* hebe2 = new Actor;
        auto model = hebe2->setRoot<SkeletalModelNode>("model");
        model->setModel(resGet<mdlSkeletalModelMaster>("models/hebe2/hebe2/hebe2.skeletal_model"));
        hebe2->setTranslation(gfxm::vec3(0, 1.75, -13));
        hebe2->setScale(gfxm::vec3(2, 2, 2));
        //hebe2->rotate(gfxm::angle_axis(gfxm::degrees(180.f), gfxm::vec3(0, 1, 0)));
        getWorld()->spawnActor(hebe2);
    }

    //cam.reset(new Camera3d);
    //cam.reset(new Camera3dThirdPerson);
    //cam->init(&camState);
    //playerFps.reset(new playerControllerFps);
    //playerFps->init(&camState, &world);

    Mesh3d mesh_ram;
    //meshGenerateVoxelField(&mesh_ram, 0, 0, 0);
    meshGenerateCube(&mesh_ram);
    Mesh3d mesh_plane;
    meshGenerateCheckerPlane(&mesh_plane, 50, 50, 50);
    Mesh3d mesh_sph;
    //meshGenerateVoxelField(&mesh_sph, 0,0,0);
    meshGenerateSphereCubic(&mesh_sph, 0.5f, 10);
    mesh.setData(&mesh_ram);
    mesh_sphere.setData(&mesh_sph);
    gpu_mesh_plane.setData(&mesh_plane);
    
    material_instancing     = resGet<gpuMaterial>("materials/instancing.mat");
    material                = resGet<gpuMaterial>("materials/default.mat");
    material2               = resGet<gpuMaterial>("materials/default2.mat");
    material3               = resGet<gpuMaterial>("materials/default3.mat");
    material_color          = resGet<gpuMaterial>("materials/color.mat");

    {   
        scnDecal* dcl = new scnDecal();
        dcl->setTexture(resGet<gpuTexture2d>("textures/decals/magic_elements.png"));
        dcl->setBoxSize(7, 2, 7);
        getWorld()->getRenderScene()->addRenderObject(dcl);
        Handle<TransformNode> nd(HANDLE_MGR<TransformNode>::acquire());
        dcl->setTransformNode(nd);
        nd->translate(-5.f, .0f, .0f);
        scnDecal* dcl2 = new scnDecal();
        dcl2->setTexture(resGet<gpuTexture2d>("icon_sprite_test.png"));
        dcl2->setBoxSize(0.45f, 0.45f, 0.45f);
        dcl2->setBlending(GPU_BLEND_MODE::NORMAL);
        nd = HANDLE_MGR<TransformNode>::acquire();
        dcl2->setTransformNode(nd);
        nd->translate(-.5f, 1.5f, 5.8f);
        nd->rotate(gfxm::angle_axis(0.2f, gfxm::vec3(0, 0, 1)) * gfxm::angle_axis(-gfxm::pi * .5f, gfxm::vec3(1, 0, 0)));
        getWorld()->getRenderScene()->addRenderObject(dcl2);

        {/*
            static RHSHARED<mdlSkeletalModelMaster> model(HANDLE_MGR<mdlSkeletalModelMaster>().acquire());
            assimpLoadSkeletalModel("models/Garuda.fbx", model.get());

            model->getSkeleton()->getRoot()->setScale(gfxm::vec3(10, 10, 10));
            static HSHARED<SkeletonPose> skl_instance = model->getSkeleton()->createInstance();
            static HSHARED<mdlSkeletalModelInstance> inst = model->createInstance(skl_instance);
            //skl_instance->getWorldTransformsPtr()[0] = gfxm::scale(gfxm::mat4(1.0f), gfxm::vec3(10, 10, 10));
            //inst->onSpawn(world->getRenderScene());

            model->getSkeleton().serializeJson("models/garuda/garuda.skeleton");
            model.serializeJson("models/garuda/garuda.skeletal_model");*/
        }
        {/*
            static RHSHARED<mdlSkeletalModelMaster> model = resGet<mdlSkeletalModelMaster>("models/garuda/garuda.skeletal_model");
            garuda_instance = model->createInstance();
            garuda_instance->spawn(world->getRenderScene());
            garuda_instance->getSkeletonInstance()->getWorldTransformsPtr()[0] 
                = gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(0, 0, -3))
                * gfxm::scale(gfxm::mat4(1.0f), gfxm::vec3(10, 10, 10));
                */
            static Actor garuda_actor;
            auto root = garuda_actor.setRoot<CharacterCapsuleNode>("capsule");
            auto node = root->createChild<SkeletalModelNode>("model");
            node->setModel(getSkeletalModel("models/garuda/garuda.skeletal_model"));
            garuda_actor.translate(gfxm::vec3(0, 0, -3));
            garuda_actor.setScale(gfxm::vec3(10, 10, 10));
            getWorld()->spawnActor(&garuda_actor);
        }
        {
            auto actor = new Actor;
            auto root = actor->setRoot<CharacterCapsuleNode>("capsule");
            auto model = root->createChild<SkeletalModelNode>("model");
            model->setModel(getSkeletalModel("import_test/2b/2b.skeletal_model"));
            actor->getRoot()->translate(gfxm::vec3(0, 0, -6));
            getWorld()->spawnActor(actor);
        }
        // Snow
        {
            auto actor = new Actor;
            actor->setFlags(ACTOR_FLAG_UPDATE);
            auto root = actor->setRoot<ParticleEmitterNode>("particles");
            RHSHARED<ParticleEmitterMaster> emitter_ref = resGet<ParticleEmitterMaster>("particle_emitters/env_dust.pte");
            root->setEmitter(emitter_ref);
            root->setTranslation(.0f, 10.5f, .0f);
            getWorld()->spawnActor(actor);
        }

        chara_actor = createPlayerActor(&tps_camera_actor);
        getWorld()->spawnActor(chara_actor.get());
        
        // Sword
        /*{
            SkeletalModelNode* node = chara_actor->findNode<SkeletalModelNode>("model");
            assert(node);
            sword_actor.reset_acquire();
            auto nmodel = sword_actor->setRoot<SkeletalModelNode>("sword");
            nmodel->setModel(getSkeletalModel("models/sword/sword.skeletal_model"));
            getWorld()->spawnActor(sword_actor.get());
            sword_actor->attachToTransform(node->getBoneProxy("AttachHand.R"));
        }*/

        chara_actor_2.reset(actorReadJson("actors/chara_24.actor"));
        if (chara_actor_2) {
            getWorld()->spawnActor(chara_actor_2.get());
        }

        {
            fps_player_actor.setFlags(ACTOR_FLAG_UPDATE);
            auto capsule = fps_player_actor.setRoot<CharacterCapsuleNode>("capsule");
            capsule->shape.height = 1.2f;
            capsule->shape.radius = .2f;
            capsule->collider.setCenterOffset(gfxm::vec3(.0f, .8f + .2f, .0f));
            fps_player_actor.addController<FpsCharacterController>();
            //fps_player_actor.addController<FpsCameraController>();

            getWorld()->spawnActor(&fps_player_actor);
        }
        
        playerLinkAgent(playerGetPrimary(), chara_actor.get());
        playerLinkAgent(playerGetPrimary(), &tps_camera_actor);


        LOG_DBG("Loading the csg scene model");
        static HSHARED<mdlSkeletalModelInstance> mdl_collision =
            resGet<mdlSkeletalModelMaster>("csg/scene5.csg.skeletal_model"/*"models/collision_test/collision_test.skeletal_model"*/)->createInstance();
        LOG_DBG("Spawning the csg scene");
        mdl_collision->spawn(getWorld()->getRenderScene());
        LOG_DBG("Done");

        {
            static CollisionTriangleMesh col_trimesh;/*
            assimpImporter importer;
            importer.loadFile("models/collision_test.fbx");
            importer.loadCollisionTriangleMesh(&col_trimesh);*/
            std::vector<uint8_t> bytes;
            fsSlurpFile("csg/scene5.csg.collision_mesh", bytes);
            col_trimesh.deserialize(bytes);

            CollisionTriangleMeshShape* shape = new CollisionTriangleMeshShape;
            shape->setMesh(&col_trimesh);
            Collider* collider = new Collider;
            collider->setFlags(COLLIDER_STATIC);
            collider->setShape(shape);
            getWorld()->getCollisionWorld()->addCollider(collider);
        }
    }

    for (int i = 0; i < TEST_INSTANCE_COUNT; ++i) {
        positions[i] = gfxm::vec4(15.0f - (rand() % 100) * .30f, (rand() % 100) * 0.30f, 15.0f - (rand() % 100) * 0.30f, .0f);
    }
    inst_pos_buffer.setArrayData(positions, sizeof(positions));
    instancing_desc.setInstanceAttribArray(VFMT::ParticlePosition_GUID, &inst_pos_buffer);
    instancing_desc.setInstanceCount(TEST_INSTANCE_COUNT);
    renderable.reset(
        new gpuGeometryRenderable(material_instancing.get(), mesh_sphere.getMeshDesc(), &instancing_desc)
    );

    renderable2.reset(new gpuGeometryRenderable(material3.get(), mesh.getMeshDesc(), 0, "MyCube"));
    renderable_plane.reset(new gpuGeometryRenderable(material_color.get(), gpu_mesh_plane.getMeshDesc()));

    // Typefaces and stuff
    font = fontGet("fonts/OpenSans-Regular.ttf", 24);

    // Skinned model
    chara.reset_acquire();
    chara->setTranslation(gfxm::vec3(-10, 0, 10));
    chara2.reset_acquire();
    chara2->setTranslation(gfxm::vec3(5, 0, 0));
    getWorld()->spawnActor(chara.get());
    getWorld()->spawnActor(chara2.get());
    door_actor.reset(new DoorActor());
    getWorld()->spawnActor(door_actor.get());
    getWorld()->spawnActor(&anim_test);
    ultima_weapon.reset_acquire();
    getWorld()->spawnActor(ultima_weapon.get());
    jukebox.reset_acquire();
    getWorld()->spawnActor(jukebox.get());
    vfx_test.reset_acquire();
    getWorld()->spawnActor(vfx_test.get());
    //vfx_test->setTranslation(gfxm::vec3(3, 0, -5));
    
    // Collision
    shape_sphere.radius = 1.f;
    shape_box.half_extents = gfxm::vec3(1.0f, 0.5f, 0.5f);
    shape_capsule.height = 1.5f;
    shape_capsule.radius = .3f;

    collider_a.setPosition(gfxm::vec3(1, 1, 1));
    collider_a.setShape(&shape_sphere);
    //collider_b.position = gfxm::vec3(0, 2, 0);
    collider_b.setShape(&shape_box);
    //collider_b.rotation = gfxm::angle_axis(-.4f, gfxm::vec3(0, 1, 0));
    collider_c.setPosition(gfxm::vec3(-10, 2, 15));
    collider_c.setShape(&shape_sphere);
    collider_d.setPosition(gfxm::vec3(0, 1.6f, -0.3f));
    collider_d.setShape(&shape_box2);

    collider_e.setShape(&shape_capsule);
    collider_e.setPosition(gfxm::vec3(-10.0f, 1.0f, 6.0f));
    collider_e.setRotation(gfxm::angle_axis(1.0f, gfxm::vec3(0, 0, 1)));

    getWorld()->getCollisionWorld()->addCollider(&collider_a);
    getWorld()->getCollisionWorld()->addCollider(&collider_b);
    getWorld()->getCollisionWorld()->addCollider(&collider_c);
    getWorld()->getCollisionWorld()->addCollider(&collider_d);
    getWorld()->getCollisionWorld()->addCollider(&collider_e);
}