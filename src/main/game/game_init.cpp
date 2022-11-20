
#include "game_common.hpp"
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

void GameCommon::Init() {
    audioInit();

    render_bucket.reset(new gpuRenderBucket(gpuGetPipeline(), 10000));
    render_target.reset(new gpuRenderTarget);
    gpuGetPipeline()->initRenderTarget(render_target.get());

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

    {
        int screen_width = 0, screen_height = 0;
        platformGetWindowSize(screen_width, screen_height);
        onViewportResize(screen_width, screen_height);
    }

    world.addSystem<wExplosionSystem>();
    world.addSystem<wMissileSystem>();

    camera_actor.setRoot<nodeCamera>("camera");
    camera_actor.addController<ctrlCameraTps>();
    world.spawnActor(&camera_actor);

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
        world.getRenderScene()->addRenderObject(dcl);
        scnNode* nd = new scnNode;
        world.getRenderScene()->addNode(nd);
        dcl->setNode(nd);
        nd->local_transform
            = gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(-5.f, .0f, .0f));
        scnDecal* dcl2 = new scnDecal();
        dcl2->setTexture(resGet<gpuTexture2d>("icon_sprite_test.png"));
        dcl2->setBoxSize(0.45f, 0.45f, 0.45f);
        nd = new scnNode;
        world.getRenderScene()->addNode(nd);
        dcl2->setNode(nd);
        nd->local_transform 
            = gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(-.5f, 1.5f, 5.8f))
            * gfxm::to_mat4(gfxm::angle_axis(0.2f, gfxm::vec3(0,0,1)) * gfxm::angle_axis(-gfxm::pi * .5f, gfxm::vec3(1, 0, 0)));
        world.getRenderScene()->addRenderObject(dcl2);

        {/*
            static RHSHARED<mdlSkeletalModelMaster> model(HANDLE_MGR<mdlSkeletalModelMaster>().acquire());
            assimpLoadSkeletalModel("models/Garuda.fbx", model.get());

            model->getSkeleton()->getRoot()->setScale(gfxm::vec3(10, 10, 10));
            static HSHARED<sklSkeletonInstance> skl_instance = model->getSkeleton()->createInstance();
            static HSHARED<mdlSkeletalModelInstance> inst = model->createInstance(skl_instance);
            //skl_instance->getWorldTransformsPtr()[0] = gfxm::scale(gfxm::mat4(1.0f), gfxm::vec3(10, 10, 10));
            //inst->onSpawn(world.getRenderScene());

            model->getSkeleton().serializeJson("models/garuda/garuda.skeleton");
            model.serializeJson("models/garuda/garuda.skeletal_model");*/
        }
        {/*
            static RHSHARED<mdlSkeletalModelMaster> model = resGet<mdlSkeletalModelMaster>("models/garuda/garuda.skeletal_model");
            garuda_instance = model->createInstance();
            garuda_instance->spawn(world.getRenderScene());
            garuda_instance->getSkeletonInstance()->getWorldTransformsPtr()[0] 
                = gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(0, 0, -3))
                * gfxm::scale(gfxm::mat4(1.0f), gfxm::vec3(10, 10, 10));
                */
            static gameActor garuda_actor;
            auto root = garuda_actor.setRoot<nodeCharacterCapsule>("capsule");
            auto node = root->createChild<nodeSkeletalModel>("model");
            node->setModel(resGet<mdlSkeletalModelMaster>("models/garuda/garuda.skeletal_model"));
            garuda_actor.getRoot()->translate(gfxm::vec3(0, 0, -3));
            world.spawnActor(&garuda_actor);
        }
        {
            static gameActor chara_actor;
            auto root = chara_actor.setRoot<nodeCharacterCapsule>("capsule");
            auto node = root->createChild<nodeSkeletalModel>("model");
            node->setModel(resGet<mdlSkeletalModelMaster>("models/chara_24/chara_24.skeletal_model"));
            auto decal = root->createChild<nodeDecal>("decal");
            auto cam_target = root->createChild<nodeEmpty>("cam_target");
            cam_target->setTranslation(.0f, 1.6f, .0f);
            chara_actor.getRoot()->translate(gfxm::vec3(-6, 0, 0));
            
            //chara_actor.addController<ctrlCharacterPlayerInput>();
            chara_actor.addController<ctrlAnimator>();
            auto fsm = chara_actor.addController<ctrlFsm>();
            fsm->addState("locomotion", new fsmCharacterStateLocomotion);
            fsm->addState("interacting", new fsmCharacterStateInteracting);

            chara_actor.addComponent<AnimatorComponent>();

            world.spawnActor(&chara_actor);

            camera_actor.getController<ctrlCameraTps>()
                ->setTarget(cam_target->getTransformHandle());
        }
        //RHSHARED<mdlSkeletalModelMaster> anor_londo(HANDLE_MGR<mdlSkeletalModelMaster>::acquire());
        //assimpLoadSkeletalModel("models/anor_londo.fbx", anor_londo.get());
        //anor_londo.serializeJson("models/anor_londo.skeletal_model", true);

        static HSHARED<mdlSkeletalModelInstance> mdl_collision =
            resGet<mdlSkeletalModelMaster>("models/collision_test/collision_test.skeletal_model")->createInstance();
        mdl_collision->spawn(world.getRenderScene());

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
            world.getCollisionWorld()->addCollider(collider);
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

    renderable2.reset(new gpuRenderable(material3.get(), mesh.getMeshDesc()));
    renderable2_ubuf = gpuGetPipeline()->createUniformBuffer(UNIFORM_BUFFER_MODEL);
    renderable2->attachUniformBuffer(renderable2_ubuf);

    renderable_plane.reset(new gpuRenderable(material_color.get(), gpu_mesh_plane.getMeshDesc()));
    renderable_plane_ubuf = gpuGetPipeline()->createUniformBuffer(UNIFORM_BUFFER_MODEL);
    renderable_plane->attachUniformBuffer(renderable_plane_ubuf);

    // Typefaces and stuff
    typefaceLoad(&typeface, "OpenSans-Regular.ttf");
    typefaceLoad(&typeface_nimbusmono, "nimbusmono-bold.otf");
    font.reset(new Font(&typeface, 24, 72));

    // Skinned model
    chara.reset_acquire();
    chara->setTranslation(gfxm::vec3(-10, 0, 10));
    chara2.reset_acquire();
    chara2->setTranslation(gfxm::vec3(5, 0, 0));
    world.spawnActor(chara.get());    
    world.spawnActor(chara2.get());
    door.reset(new Door());
    world.spawnActor(door.get());
    world.spawnActor(&anim_test);
    ultima_weapon.reset_acquire();
    world.spawnActor(ultima_weapon.get());
    jukebox.reset_acquire();
    world.spawnActor(jukebox.get());
    vfx_test.reset_acquire();
    world.spawnActor(vfx_test.get());
    
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

    world.getCollisionWorld()->addCollider(&collider_a);
    world.getCollisionWorld()->addCollider(&collider_b);
    world.getCollisionWorld()->addCollider(&collider_c);
    world.getCollisionWorld()->addCollider(&collider_d);
    world.getCollisionWorld()->addCollider(&collider_e);
}