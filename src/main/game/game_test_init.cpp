
#include "game_test.hpp"
#include "mesh3d/generate_primitive.hpp"

#include "handle/hshared.hpp"

#include "reflection/reflection.hpp"

#include "skeleton/skeleton_editable.hpp"
#include "skeleton/skeleton_prototype.hpp"
#include "skeleton/skeleton_instance.hpp"

#include "skeletal_model/skeletal_model.hpp"
#include "import/assimp_load_skeletal_model.hpp"

#include "world/node/node_camera.hpp"
#include "world/node/node_skeletal_model.hpp"
#include "world/node/node_character_capsule.hpp"
#include "world/node/node_decal.hpp"
#include "world/component/components.hpp"

#include "game_ui/game_ui.hpp"

void GameTest::init() {
    GameBase::init();

    gameuiInit();

    InputContext* inputCtxDebug = inputCreateContext("Debug");
    InputContext* inputCtxPlayer = inputCreateContext("Player");

    for (int i = 0; i < 12; ++i) {
        inputFButtons[i] = inputCreateAction(MKSTR("F" << (i + 1)).c_str());
        inputFButtons[i]->linkKey(Key.Keyboard.F1 + i);
        inputCtxDebug->linkAction(inputFButtons[i]);
    }

    inputCharaTranslation = inputCreateRange("CharacterLocomotion");
    inputCharaUse = inputCreateAction("CharacterInteract");
    inputRotation = inputCreateRange("CameraRotation");
    InputAction* inputLeftClick = inputCreateAction("Shoot");
    InputRange* inputScroll = inputCreateRange("Scroll");
    InputAction* inputSprint = inputCreateAction("Sprint");

    inputCharaTranslation
        ->linkKeyX(Key.Keyboard.A, -1.0f)
        .linkKeyX(Key.Keyboard.D, 1.0f)
        .linkKeyZ(Key.Keyboard.W, -1.0f)
        .linkKeyZ(Key.Keyboard.S, 1.0f);
    inputCharaUse
        ->linkKey(Key.Keyboard.E, 1.0f);
    inputRotation
        ->linkKeyY(Key.Mouse.AxisX, 1.0f)
        .linkKeyX(Key.Mouse.AxisY, 1.0f);
    inputLeftClick
        ->linkKey(Key.Mouse.BtnLeft);
    inputScroll
        ->linkKeyX(Key.Mouse.Scroll, -1.0f);
    inputSprint
        ->linkKey(Key.Keyboard.LeftShift, 1.0f);

    inputCtxPlayer
        ->linkAction(inputCharaUse)
        .linkRange(inputCharaTranslation)
        .linkRange(inputRotation)
        .linkAction(inputLeftClick)
        .linkRange(inputScroll)
        .linkAction(inputSprint);

    {
        ubufCam3d = gpuGetPipeline()->createUniformBuffer(UNIFORM_BUFFER_CAMERA_3D);
        ubufTime = gpuGetPipeline()->createUniformBuffer(UNIFORM_BUFFER_TIME);
        gpuGetPipeline()->attachUniformBuffer(ubufCam3d);
        gpuGetPipeline()->attachUniformBuffer(ubufTime);
    }

    getWorld()->addSystem<wExplosionSystem>();
    getWorld()->addSystem<wMissileSystem>();

    camera_actor.setRoot<nodeCamera>("camera");
    camera_actor.addController<ctrlCameraTps>();
    getWorld()->spawnActor(&camera_actor);

    //cam.reset(new Camera3d);
    //cam.reset(new Camera3dThirdPerson);
    //cam->init(&camState);
    //playerFps.reset(new playerControllerFps);
    //playerFps->init(&camState, &world);

    Mesh3d mesh_ram;
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
        dcl->setTexture(resGet<gpuTexture2d>("pentagram.png"));
        dcl->setBoxSize(7, 2, 7);
        getWorld()->getRenderScene()->addRenderObject(dcl);
        scnNode* nd = new scnNode;
        getWorld()->getRenderScene()->addNode(nd);
        dcl->setNode(nd);
        nd->local_transform
            = gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(-5.f, .0f, .0f));
        scnDecal* dcl2 = new scnDecal();
        dcl2->setTexture(resGet<gpuTexture2d>("icon_sprite_test.png"));
        dcl2->setBoxSize(0.45f, 0.45f, 0.45f);
        nd = new scnNode;
        getWorld()->getRenderScene()->addNode(nd);
        dcl2->setNode(nd);
        nd->local_transform 
            = gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(-.5f, 1.5f, 5.8f))
            * gfxm::to_mat4(gfxm::angle_axis(0.2f, gfxm::vec3(0,0,1)) * gfxm::angle_axis(-gfxm::pi * .5f, gfxm::vec3(1, 0, 0)));
        getWorld()->getRenderScene()->addRenderObject(dcl2);

        {/*
            static RHSHARED<mdlSkeletalModelMaster> model(HANDLE_MGR<mdlSkeletalModelMaster>().acquire());
            assimpLoadSkeletalModel("models/Garuda.fbx", model.get());

            model->getSkeleton()->getRoot()->setScale(gfxm::vec3(10, 10, 10));
            static HSHARED<sklSkeletonInstance> skl_instance = model->getSkeleton()->createInstance();
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
            static gameActor garuda_actor;
            auto root = garuda_actor.setRoot<nodeCharacterCapsule>("capsule");
            auto node = root->createChild<nodeSkeletalModel>("model");
            node->setModel(resGet<mdlSkeletalModelMaster>("models/garuda/garuda.skeletal_model"));
            garuda_actor.getRoot()->translate(gfxm::vec3(0, 0, -3));
            getWorld()->spawnActor(&garuda_actor);
        }
        {
            chara_actor.reset_acquire();
            auto root = chara_actor->setRoot<nodeCharacterCapsule>("capsule");
            auto node = root->createChild<nodeSkeletalModel>("model");
            node->setModel(resGet<mdlSkeletalModelMaster>("models/chara_24/chara_24.skeletal_model"));
            auto decal = root->createChild<nodeDecal>("decal");
            auto cam_target = root->createChild<nodeEmpty>("cam_target");
            cam_target->setTranslation(.0f, 1.5f, .0f);
            chara_actor->getRoot()->translate(gfxm::vec3(-6, 0, 0));
            
            //chara_actor.addController<ctrlCharacterPlayerInput>();
            chara_actor->addController<ctrlAnimator>();
            auto fsm = chara_actor->addController<ctrlFsm>();
            fsm->addState("locomotion", new fsmCharacterStateLocomotion);
            fsm->addState("interacting", new fsmCharacterStateInteracting);

            AnimatorComponent* anim_comp = chara_actor->addComponent<AnimatorComponent>();
            {
                auto anim_idle = resGet<Animation>("models/chara_24/Idle.animation");
                auto anim_run2 = resGet<Animation>("models/chara_24/Run.animation");
                auto anim_falling = resGet<Animation>("models/chara_24/Falling.animation");
                auto anim_action_opendoor = resGet<Animation>("models/chara_24_anim_door/Action_OpenDoor.animation");
                auto anim_action_dooropenback = resGet<Animation>("models/chara_24/Action_DoorOpenBack.animation");
                auto skeleton = resGet<sklSkeletonMaster>("models/chara_24/chara_24.skeleton");
                static RHSHARED<animSequence> seq_idle;
                seq_idle.reset_acquire();
                seq_idle->setSkeletalAnimation(anim_idle);
                static RHSHARED<animSequence> seq_run2;
                seq_run2.reset_acquire();
                seq_run2->setSkeletalAnimation(anim_run2);
                static RHSHARED<animSequence> seq_falling;
                seq_falling.reset_acquire();
                seq_falling->setSkeletalAnimation(anim_falling);
                static RHSHARED<audioSequence> audio_seq;
                audio_seq.reset_acquire();
                audio_seq->length = 40.0f;
                audio_seq->fps = 60.0f;
                audio_seq->insert(0, resGet<AudioClip>("audio/sfx/footsteps/asphalt00.ogg"));
                audio_seq->insert(20, resGet<AudioClip>("audio/sfx/footsteps/asphalt04.ogg"));
                seq_run2->setAudioSequence(audio_seq);
                static RHSHARED<animSequence> seq_open_door_front;
                seq_open_door_front.reset_acquire();
                seq_open_door_front->setSkeletalAnimation(anim_action_opendoor);
                static RHSHARED<animSequence> seq_open_door_back;
                seq_open_door_back.reset_acquire();
                seq_open_door_back->setSkeletalAnimation(anim_action_dooropenback);
                static RHSHARED<AnimatorMaster> animator_master;
                animator_master.reset_acquire();
                animator_master->setSkeleton(skeleton);
                animator_master->addParam("velocity");
                animator_master->addSignal("sig_door_open");
                animator_master->addSignal("sig_door_open_back");
                animator_master->addFeedbackEvent("fevt_door_open_end");
                animator_master
                    ->addSampler("idle", "Default", seq_idle)
                    .addSampler("run", "Locomotion", seq_run2)
                    .addSampler("falling", "Falling", seq_falling)
                    .addSampler("open_door_front", "Interact", seq_open_door_front)
                    .addSampler("open_door_back", "Interact", seq_open_door_back);
                animUnitFsm* fsm = animator_master->setRoot<animUnitFsm>();
                animFsmState* state_idle = fsm->addState("Idle");
                animFsmState* state_loco = fsm->addState("Locomotion");
                animFsmState* state_fall = fsm->addState("Falling");
                animFsmState* state_door_front = fsm->addState("DoorOpenFront");
                animFsmState* state_door_back = fsm->addState("DoorOpenBack");
                state_idle->setUnit<animUnitSingle>()->setSampler("idle");
                state_loco->setUnit<animUnitSingle>()->setSampler("run");
                state_fall->setUnit<animUnitSingle>()->setSampler("falling");
                state_door_front->setUnit<animUnitSingle>()->setSampler("open_door_front");
                state_door_front->onExit(call_feedback_event_(animator_master.get(), "fevt_door_open_end"));
                state_door_back->setUnit<animUnitSingle>()->setSampler("open_door_back");
                state_door_back->onExit(call_feedback_event_(animator_master.get(), "fevt_door_open_end"));
                fsm->addTransition("Idle", "Locomotion", param_(animator_master.get(), "velocity") > FLT_EPSILON, 0.15f);
                fsm->addTransition("Idle", "Falling", param_(animator_master.get(), "is_falling") > FLT_EPSILON, 0.15f);
                fsm->addTransition("Locomotion", "Idle", param_(animator_master.get(), "velocity") <= FLT_EPSILON, 0.15f);
                fsm->addTransition("Locomotion", "Falling", param_(animator_master.get(), "is_falling") > FLT_EPSILON, 0.15f);
                fsm->addTransition("Falling", "Idle", param_(animator_master.get(), "is_falling") <= FLT_EPSILON, 0.15f);
                fsm->addTransitionAnySource("DoorOpenFront", signal_(animator_master.get(), "sig_door_open"), 0.15f);
                fsm->addTransitionAnySource("DoorOpenBack", signal_(animator_master.get(), "sig_door_open_back"), 0.15f);
                fsm->addTransition("DoorOpenFront", "Idle", state_complete_(), 0.15f);
                fsm->addTransition("DoorOpenBack", "Idle", state_complete_(), 0.15f);
                animator_master->compile();

                anim_comp->setAnimatorMaster(animator_master);
            }

            getWorld()->spawnActor(chara_actor.get());

            camera_actor.getController<ctrlCameraTps>()
                ->setTarget(cam_target->getTransformHandle());
        }
        //RHSHARED<mdlSkeletalModelMaster> anor_londo(HANDLE_MGR<mdlSkeletalModelMaster>::acquire());
        //assimpLoadSkeletalModel("models/anor_londo.fbx", anor_londo.get());
        //anor_londo.serializeJson("models/anor_londo.skeletal_model", true);

        static HSHARED<mdlSkeletalModelInstance> mdl_collision =
            resGet<mdlSkeletalModelMaster>("models/collision_test/collision_test.skeletal_model")->createInstance();
        mdl_collision->spawn(getWorld()->getRenderScene());

        {
            static CollisionTriangleMesh col_trimesh;
            assimpImporter importer;
            importer.loadFile("models/collision_test.fbx");
            importer.loadCollisionTriangleMesh(&col_trimesh);
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
        new gpuRenderable(material_instancing.get(), mesh_sphere.getMeshDesc(), &instancing_desc)
    );

    renderable2.reset(new gpuRenderable(material3.get(), mesh.getMeshDesc(), 0, "MyCube"));
    renderable2_ubuf = gpuGetPipeline()->createUniformBuffer(UNIFORM_BUFFER_MODEL);
    renderable2->attachUniformBuffer(renderable2_ubuf);

    renderable_plane.reset(new gpuRenderable(material_color.get(), gpu_mesh_plane.getMeshDesc()));
    renderable_plane_ubuf = gpuGetPipeline()->createUniformBuffer(UNIFORM_BUFFER_MODEL);
    renderable_plane->attachUniformBuffer(renderable_plane_ubuf);

    // Typefaces and stuff
    font = fontGet("OpenSans-Regular.ttf", 24);

    // Skinned model
    chara.reset_acquire();
    chara->setTranslation(gfxm::vec3(-10, 0, 10));
    chara2.reset_acquire();
    chara2->setTranslation(gfxm::vec3(5, 0, 0));
    getWorld()->spawnActor(chara.get());
    getWorld()->spawnActor(chara2.get());
    door.reset(new Door());
    getWorld()->spawnActor(door.get());
    getWorld()->spawnActor(&anim_test);
    ultima_weapon.reset_acquire();
    getWorld()->spawnActor(ultima_weapon.get());
    jukebox.reset_acquire();
    getWorld()->spawnActor(jukebox.get());
    vfx_test.reset_acquire();
    getWorld()->spawnActor(vfx_test.get());
    //vfx_test->setTranslation(gfxm::vec3(3, 0, -5));
    
    // Collision
    shape_sphere.radius = .5f;
    shape_box.half_extents = gfxm::vec3(1.0f, 0.5f, 0.5f);
    shape_capsule.height = 1.5f;
    shape_capsule.radius = .3f;

    collider_a.setPosition(gfxm::vec3(1, 1, 1));
    collider_a.setShape(&shape_sphere);
    //collider_b.position = gfxm::vec3(0, 2, 0);
    collider_b.setShape(&shape_box);
    //collider_b.rotation = gfxm::angle_axis(-.4f, gfxm::vec3(0, 1, 0));
    collider_c.setPosition(gfxm::vec3(-1, 0, 1));
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