
#include "game_common.hpp"
#include "mesh3d/generate_primitive.hpp"

#include "handle/hshared.hpp"

#include "reflection/reflection.hpp"

#include "skeleton/skeleton_editable.hpp"
#include "skeleton/skeleton_prototype.hpp"
#include "skeleton/skeleton_instance.hpp"

#include "skeletal_model/skeletal_model.hpp"
#include "import/assimp_load_skeletal_model.hpp"

void GameCommon::Init() {
    for (int i = 0; i < 12; ++i) {
        inputFButtons[i] = inputCtx.createAction(MKSTR("F" << (i+1)).c_str());
        inputFButtons[i]->linkKey(InputDeviceType::Keyboard, Key.Keyboard.F1 + i);
    }

    inputCharaTranslation = inputCtxChara.createRange("CharaLocomotion");
    inputCharaTranslation
        ->linkKeyX(InputDeviceType::Keyboard, Key.Keyboard.A, -1.0f)
        .linkKeyX(InputDeviceType::Keyboard, Key.Keyboard.D, 1.0f)
        .linkKeyZ(InputDeviceType::Keyboard, Key.Keyboard.W, -1.0f)
        .linkKeyZ(InputDeviceType::Keyboard, Key.Keyboard.S, 1.0f);
    inputCharaUse = inputCtxChara.createAction("CharaUse");
    inputCharaUse->linkKey(InputDeviceType::Keyboard, Key.Keyboard.E, 1.0f);

    gpu_pipeline.reset(new build_config::gpuPipelineCommon);
    gpuInit(gpu_pipeline.get()); // !!

    {
        ubufCam3d = gpu_pipeline->createUniformBuffer(UNIFORM_BUFFER_CAMERA_3D);
        ubufTime = gpu_pipeline->createUniformBuffer(UNIFORM_BUFFER_TIME);
        gpu_pipeline->attachUniformBuffer(ubufCam3d);
        gpu_pipeline->attachUniformBuffer(ubufTime);
    }    

    {
        int screen_width = 0, screen_height = 0;
        platformGetWindowSize(screen_width, screen_height);
        onViewportResize(screen_width, screen_height);
    }

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

    shader_default          = resGet<gpuShaderProgram>("shaders/default.glsl");
    shader_vertex_color     = resGet<gpuShaderProgram>("shaders/vertex_color.glsl");
    shader_text             = resGet<gpuShaderProgram>("shaders/text.glsl");
    shader_instancing       = resGet<gpuShaderProgram>("shaders/instancing.glsl");
    
    texture                 = resGet<gpuTexture2d>("test.jpg");
    texture2                = resGet<gpuTexture2d>("test2.png");
    texture3                = resGet<gpuTexture2d>("icon_sprite_test.png");
    texture4                = resGet<gpuTexture2d>("1648920106773.jpg");
    
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

        {/*
            static RHSHARED<sklmSkeletalModelEditable> model(HANDLE_MGR<sklmSkeletalModelEditable>().acquire());
            assimpLoadSkeletalModel("models/Garuda.fbx", model.get());

            model->getSkeleton()->getRoot()->setScale(gfxm::vec3(10, 10, 10));
            static HSHARED<sklSkeletonInstance> skl_instance = model->getSkeleton()->createInstance();
            static HSHARED<sklmSkeletalModelInstance> inst = model->createInstance(skl_instance);
            //skl_instance->getWorldTransformsPtr()[0] = gfxm::scale(gfxm::mat4(1.0f), gfxm::vec3(10, 10, 10));
            //inst->onSpawn(world.getRenderScene());

            model->getSkeleton().serializeJson("models/garuda/garuda.skeleton");
            model.serializeJson("models/garuda/garuda.skeletal_model");*/
        }
        {
            static HSHARED<sklmSkeletalModelInstance> inst
                = resGet<sklmSkeletalModelEditable>("models/garuda/garuda.skeletal_model")->createInstance();
            inst->onSpawn(world.getRenderScene());

            static HSHARED<sklmSkeletalModelInstance> ultima_inst
                = resGet<sklmSkeletalModelEditable>("models/ultima_weapon/ultima_weapon.skeletal_model")->createInstance();
            ultima_inst->onSpawn(world.getRenderScene());
            ultima_inst->getSkeletonInstance()->getWorldTransformsPtr()[0] 
                = gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(6, 0, -6))
                * gfxm::scale(gfxm::mat4(1.0f), gfxm::vec3(2, 2, 2));
                
            //static HSHARED<sklmSkeletalModelInstance> anor_londo
            //    = resGet<sklmSkeletalModelEditable>("models/anor_londo/anor_londo.skeletal_model")->createInstance();
            //anor_londo->onSpawn(world.getRenderScene());
            //anor_londo->getSkeletonInstance()->getWorldTransformsPtr()[0] = gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(350, -125, -250));
            //resGet<sklmSkeletalModelEditable>("models/anor_londo/anor_londo.skeletal_model")->dbgLog();
        }
        //RHSHARED<sklmSkeletalModelEditable> anor_londo(HANDLE_MGR<sklmSkeletalModelEditable>::acquire());
        //assimpLoadSkeletalModel("models/anor_londo.fbx", anor_londo.get());
        //anor_londo.serializeJson("models/anor_londo.skeletal_model", true);
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
    renderable2_ubuf = gpu_pipeline->createUniformBuffer(UNIFORM_BUFFER_MODEL);
    renderable2->attachUniformBuffer(renderable2_ubuf);

    renderable_plane.reset(new gpuRenderable(material_color.get(), gpu_mesh_plane.getMeshDesc()));
    renderable_plane_ubuf = gpu_pipeline->createUniformBuffer(UNIFORM_BUFFER_MODEL);
    renderable_plane->attachUniformBuffer(renderable_plane_ubuf);

    // Typefaces and stuff
    typefaceLoad(&typeface, "OpenSans-Regular.ttf");
    typefaceLoad(&typeface_nimbusmono, "nimbusmono-bold.otf");
    font.reset(new Font(&typeface, 24, 72));

    // Skinned model
    chara.init(&collision_world, material_color.get(), &world);
    chara2.init(&collision_world, material_color.get(), &world);
    chara2.setTranslation(gfxm::vec3(5, 0, 0));
    chara.onSpawn(&world);
    chara2.onSpawn(&world);

    door.init(&collision_world);

    // Collision
    shape_sphere.radius = .5f;
    shape_box.half_extents = gfxm::vec3(1.0f, 0.5f, 0.5f);

    collision_debug_draw.reset(new CollisionDebugDraw(gpu_pipeline.get()));
    collision_world.setDebugDrawInterface(collision_debug_draw.get());
    collider_a.position = gfxm::vec3(1, 1, 1);
    collider_a.setShape(&shape_sphere);
    //collider_b.position = gfxm::vec3(0, 2, 0);
    collider_b.setShape(&shape_box);
    //collider_b.rotation = gfxm::angle_axis(-.4f, gfxm::vec3(0, 1, 0));
    collider_c.position = gfxm::vec3(-1, 0, 1);
    collider_c.setShape(&shape_sphere);
    collider_d.position = gfxm::vec3(0, 1.6f, -0.3f);
    collider_d.setShape(&shape_box2);
    collision_world.addCollider(&collider_a);
    collision_world.addCollider(&collider_b);
    collision_world.addCollider(&collider_c);
    collision_world.addCollider(&collider_d);

    // Box input
    inputBoxTranslation = inputCtxBox.createRange("Translation");
    inputBoxRotation = inputCtxBox.createRange("Rotation");
    inputBoxTranslation
        ->linkKeyX(InputDeviceType::Keyboard, Key.Keyboard.Left, -1.0f)
        .linkKeyX(InputDeviceType::Keyboard, Key.Keyboard.Right, 1.0f)
        .linkKeyY(InputDeviceType::Keyboard, Key.Keyboard.Q, -1.0f)
        .linkKeyY(InputDeviceType::Keyboard, Key.Keyboard.E, 1.0f)
        .linkKeyZ(InputDeviceType::Keyboard, Key.Keyboard.Up, -1.0f)
        .linkKeyZ(InputDeviceType::Keyboard, Key.Keyboard.Down, 1.0f);
    inputBoxRotation
        ->linkKeyX(InputDeviceType::Keyboard, Key.Keyboard.Z, 1)
        .linkKeyY(InputDeviceType::Keyboard, Key.Keyboard.X, 1)
        .linkKeyZ(InputDeviceType::Keyboard, Key.Keyboard.C, 1);

    // gui?
    {
        font2.reset(new Font(&typeface_nimbusmono, 13, 72));

        guiInit(font2.get());

        int screen_width = 0, screen_height = 0;
        platformGetWindowSize(screen_width, screen_height);

        gui_root.reset(new GuiDockSpace());
        gui_root->getRoot()->splitLeft();
        //gui_root.getRoot()->left->split();
        gui_root->getRoot()->right->splitTop();
        gui_root->pos = gfxm::vec2(0.0f, 0.0f);
        gui_root->size = gfxm::vec2(screen_width, screen_height);
        
        auto wnd = new GuiWindow("1 Test window");
        wnd->pos = gfxm::vec2(120, 160);
        wnd->size = gfxm::vec2(640, 700);
        wnd->addChild(new GuiTextBox());
        wnd->addChild(new GuiImage(texture3.get()));
        wnd->addChild(new GuiButton());
        wnd->addChild(new GuiButton());
        auto wnd2 = new GuiWindow("2 Other test window");
        wnd2->pos = gfxm::vec2(850, 200);
        wnd2->size = gfxm::vec2(320, 800);
        wnd2->addChild(new GuiImage(texture4.get()));
        gui_root->getRoot()->left->addWindow(wnd);
        gui_root->getRoot()->right->left->addWindow(wnd2);
        auto wnd3 = new GuiWindow("3 Third test window");
        wnd3->pos = gfxm::vec2(850, 200);
        wnd3->size = gfxm::vec2(400, 700);
        wnd3->addChild(new GuiImage(gpu_pipeline->tex_albedo.get()));
        gui_root->getRoot()->right->right->addWindow(wnd3);
        auto wnd4 = new GuiDemoWindow();
    }
}