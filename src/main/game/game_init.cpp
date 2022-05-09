
#include "game_common.hpp"
#include "common/mesh3d/generate_primitive.hpp"

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

    {
        int screen_width = 0, screen_height = 0;
        platformGetWindowSize(screen_width, screen_height);
        tex_albedo.reset(new gpuTexture2d(GL_RGB, screen_width, screen_height, 3));
        tex_depth.reset(new gpuTexture2d(GL_DEPTH_COMPONENT, screen_width, screen_height, 1));

        fb_color.reset(new gpuFrameBuffer());
        fb_color->addColorTarget("Albedo", tex_albedo.get());
        if (!fb_color->validate()) {
            LOG_ERR("Color only framebuffer is not valid");
        }
        fb_color->prepare();

        frame_buffer.reset(new gpuFrameBuffer());
        frame_buffer->addColorTarget("Albedo", tex_albedo.get());
        frame_buffer->addDepthTarget(tex_depth.get());
        if (!frame_buffer->validate()) {
            LOG_ERR("Framebuffer not valid!");
        }
        frame_buffer->prepare();

        gpu_pipeline.reset(new gpuPipeline());
        gpuPipelineTechnique* tech = gpu_pipeline->createTechnique("Normal", 1);
        tech->getPass(0)->setFrameBuffer(frame_buffer.get());
        tech = gpu_pipeline->createTechnique("GUI", 1);
        tech->getPass(0)->setFrameBuffer(frame_buffer.get());
        tech = gpu_pipeline->createTechnique("Debug", 1);
        tech->getPass(0)->setFrameBuffer(frame_buffer.get());
        gpu_pipeline->compile();


        ubufCam3dDesc = gpu_pipeline->createUniformBufferDesc(UNIFORM_BUFFER_CAMERA_3D);
        ubufCam3dDesc
            ->define(UNIFORM_PROJECTION, UNIFORM_MAT4)
            .define(UNIFORM_VIEW_TRANSFORM, UNIFORM_MAT4)
            .compile();
        ubufTimeDesc = gpu_pipeline->createUniformBufferDesc(UNIFORM_BUFFER_TIME);
        ubufTimeDesc
            ->define(UNIFORM_TIME, UNIFORM_FLOAT)
            .compile();
        ubufModelDesc = gpu_pipeline->createUniformBufferDesc(UNIFORM_BUFFER_MODEL);
        ubufModelDesc
            ->define(UNIFORM_MODEL_TRANSFORM, UNIFORM_MAT4)
            .compile();
        ubufTextDesc = gpu_pipeline->createUniformBufferDesc(UNIFORM_BUFFER_TEXT);
        ubufTextDesc
            ->define(UNIFORM_TEXT_LOOKUP_TEXTURE_WIDTH, UNIFORM_INT)
            .compile();

        ubufCam3d = gpu_pipeline->createUniformBuffer(UNIFORM_BUFFER_CAMERA_3D);
        ubufTime = gpu_pipeline->createUniformBuffer(UNIFORM_BUFFER_TIME);
        ubufText = gpu_pipeline->createUniformBuffer(UNIFORM_BUFFER_TEXT);
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

    assimpLoadScene("2b.fbx", importedScene);

    const char* vs = R"(
        #version 450 
        in vec3 inPosition;
        in vec3 inColorRGB;
        in vec2 inUV;
        in vec3 inNormal;
        out vec3 pos_frag;
        out vec3 col_frag;
        out vec2 uv_frag;
        out vec3 normal_frag;

        layout(std140) uniform bufCamera3d {
            mat4 matProjection;
            mat4 matView;
        };
        layout(std140) uniform bufModel {
            mat4 matModel;
        };
        layout(std140) uniform bufTime {
            float fTime;        
        };
        
        void main(){
            uv_frag = inUV + (fTime * 0.25);
            normal_frag = (matModel * vec4(inNormal, 0)).xyz;
            pos_frag = inPosition;
            col_frag = inColorRGB;
            vec4 pos = matProjection * matView * matModel * vec4(inPosition, 1);
            gl_Position = pos;
        })";
    const char* fs = R"(
        #version 450
        in vec3 pos_frag;
        in vec3 col_frag;
        in vec2 uv_frag;
        in vec3 normal_frag;
        out vec4 outAlbedo;
        uniform sampler2D texAlbedo;
        void main(){
            vec4 pix = texture(texAlbedo, uv_frag);
            float a = pix.a;
            float lightness = dot(normal_frag, normalize(vec3(0, 0, 1))) + dot(normal_frag, normalize(vec3(0, 1, -1)));
            lightness = clamp(lightness, 0.2, 1.0) * 2.0;
            vec3 color = col_frag * (pix.rgb) * lightness;
            outAlbedo = vec4(color, a);
        })";
    shader_default = new gpuShaderProgram(vs, fs);
    const char* vs2 = R"(
        #version 450 
        in vec3 inPosition;
        in vec3 inColorRGB;
        out vec3 col_frag;

        layout(std140) uniform bufCamera3d {
            mat4 matProjection;
            mat4 matView;
        };
        layout(std140) uniform bufModel {
            mat4 matModel;
        };
        
        void main(){
            col_frag = inColorRGB;
            vec4 pos = matProjection * matView * matModel * vec4(inPosition, 1);
            gl_Position = pos;
        })";
    const char* fs2 = R"(
        #version 450
        in vec3 col_frag;
        out vec4 outAlbedo;
        void main(){
            vec3 color = col_frag;
            outAlbedo = vec4(color, 1.0f);
        })";
    shader_vertex_color = new gpuShaderProgram(vs2, fs2);
    const char* vs_text = R"(
        #version 450 
        in vec3 inPosition;
        in vec2 inUV;
        in float inTextUVLookup;
        in vec3 inColorRGB;
        out vec2 uv_frag;
        out vec4 col_frag;

        uniform sampler2D texTextUVLookupTable;

        layout(std140) uniform bufCamera3d {
            mat4 matProjection;
            mat4 matView;
        };
        layout(std140) uniform bufModel {
            mat4 matModel;
        };
        layout(std140) uniform bufText {
            int lookupTextureWidth;
        };
        
        void main(){
            float lookup_x = (inTextUVLookup + 0.5) / float(lookupTextureWidth);
            vec2 uv_ = texture(texTextUVLookupTable, vec2(lookup_x, 0), 0).xy;
            uv_frag = uv_;
            col_frag = vec4(inColorRGB, 1);        

            vec3 pos3 = inPosition;
	        pos3.x = round(pos3.x);
	        pos3.y = round(pos3.y);

            vec3 scale = vec3(length(matModel[0]),
                    length(matModel[1]),
                    length(matModel[2]));

            mat4 billboardView = matView * matModel;
            billboardView[0] = vec4(scale.x,0,0,0);
            billboardView[1] = vec4(0,scale.y,0,0);
            billboardView[2] = vec4(0,0,scale.z,0);   

	        vec4 pos = matProjection * billboardView * vec4(inPosition, 1);
	        gl_Position = pos;
        })";
    const char* fs_text = R"(
        #version 450
        in vec2 uv_frag;
        in vec4 col_frag;
        out vec4 outAlbedo;

        uniform sampler2D texAlbedo;
        uniform vec4 color;
        void main(){
            float c = texture(texAlbedo, uv_frag).x;
            outAlbedo = vec4(1, 1, 1, c);// * col_frag;// * color;
        })";
    shader_text = new gpuShaderProgram(vs_text, fs_text);

    auto image = loadImage("test.jpg");
    texture.setData(image);
    delete image;
    auto image2 = loadImage("test2.png");
    texture2.setData(image2);
    delete image2;
    auto image3 = loadImage("icon_sprite_test.png");
    texture3.setData(image3);
    delete image3;
    auto image4 = loadImage("1648920106773.jpg");
    texture4.setData(image4);
    delete image4;

    material = gpu_pipeline->createMaterial();
    auto tech = material->addTechnique("Normal");
    auto pass = tech->addPass();
    pass->setShader(shader_default);
    pass->cull_faces = 0;
    material->addSampler("texAlbedo", &texture);
    material->compile();

    material2 = gpu_pipeline->createMaterial();
    tech = material2->addTechnique("GUI");
    pass = tech->addPass();
    pass->setShader(shader_default);
    pass->depth_test = 1;
    material2->addSampler("texAlbedo", &texture2);
    material2->compile();

    material3 = gpu_pipeline->createMaterial();
    tech = material3->addTechnique("Normal");
    pass = tech->addPass();
    pass->setShader(shader_default);
    material3->addSampler("texAlbedo", &tex_font_atlas);
    material3->compile();

    material_color = gpu_pipeline->createMaterial();
    tech = material_color->addTechnique("Normal");
    pass = tech->addPass();
    pass->setShader(shader_vertex_color);
    material_color->compile();

    model_3d.reset(new Model3d);
    model_3d->init(gpu_pipeline.get(), importedScene, material);

    scene_mesh.reset(new SceneMesh(gpu_pipeline.get()));
    scene_mesh->renderable.setMaterial(material2);
    scene_mesh->renderable.setMeshDesc(mesh_sphere.getMeshDesc());
    scene_mesh->renderable.compile();

    renderable2_ubuf = gpu_pipeline->createUniformBuffer(UNIFORM_BUFFER_MODEL);
    renderable2.attachUniformBuffer(renderable2_ubuf);
    renderable2.setMaterial(material3);
    renderable2.setMeshDesc(mesh.getMeshDesc());
    renderable2.compile();

    renderable_plane_ubuf = gpu_pipeline->createUniformBuffer(UNIFORM_BUFFER_MODEL);
    renderable_plane.attachUniformBuffer(renderable_plane_ubuf);
    renderable_plane.setMaterial(material_color);
    renderable_plane.setMeshDesc(gpu_mesh_plane.getMeshDesc());
    renderable_plane.compile();

    // Typefaces and stuff
    typefaceLoad(&typeface, "OpenSans-Regular.ttf");
    font.reset(new Font(&typeface, 24, 72));
    gpu_text.reset(new gpuText(font.get()));
    gpu_text->setString("One ring to rule them all,\n one ring to find them, One ring to bring them all,\n and in the darkness bind them;\n In the Land of Mordor where the shadows lie.");
    gpu_text->commit();

    ktImage imgFontAtlas;
    ktImage imgFontLookupTexture;
    font->buildAtlas(&imgFontAtlas, &imgFontLookupTexture);
    tex_font_atlas.setData(&imgFontAtlas);
    tex_font_lookup.setData(&imgFontLookupTexture);
    tex_font_lookup.setFilter(GPU_TEXTURE_FILTER_NEAREST);
    ubufText->setInt(ubufText->getDesc()->getUniform("lookupTextureWidth"), tex_font_lookup.getWidth());

    material_text = gpu_pipeline->createMaterial();
    tech = material_text->addTechnique("Normal");
    pass = tech->addPass();
    pass->setShader(shader_text);
    //pass->addParam("color")->setVec4(gfxm::vec4(1, 1, 1, 1));
    material_text->addSampler("texAlbedo", &tex_font_atlas);
    material_text->addSampler("texTextUVLookupTable", &tex_font_lookup);
    material_text->addUniformBuffer(ubufText);
    material_text->compile();

    renderable_text_ubuf = gpu_pipeline->createUniformBuffer(UNIFORM_BUFFER_MODEL);
    renderable_text.attachUniformBuffer(renderable_text_ubuf);
    renderable_text.setMaterial(material_text);
    renderable_text.setMeshDesc(gpu_text->getMeshDesc());
    renderable_text.compile();

    // Skinned model
    chara.init(gpu_pipeline.get(), &collision_world, material);

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
        int screen_width = 0, screen_height = 0;
        platformGetWindowSize(screen_width, screen_height);

        font2.reset(new Font(&typeface, 12, 72));

        gui_root.reset(new GuiDockSpace(font2.get()));
        gui_root->init();
        gui_root->getRoot()->splitV();
        //gui_root.getRoot()->left->split();
        gui_root->getRoot()->right->splitH();
        gui_root->pos = gfxm::vec2(0.0f, 0.0f);
        gui_root->size = gfxm::vec2(screen_width, screen_height);
        
        auto wnd = new GuiWindow(font2.get(), "Test window");
        wnd->pos = gfxm::vec2(120, 160);
        wnd->size = gfxm::vec2(640, 700);
        wnd->addChild(new GuiImage(&texture3));
        wnd->addChild(new GuiText(font2.get()));
        wnd->addChild(new GuiButton(font2.get()));
        wnd->addChild(new GuiButton(font2.get()));
        wnd->addChild(new GuiTextBox(font2.get()));
        wnd->addChild(new GuiText(font2.get()));
        wnd->addChild(new GuiText(font2.get()));
        auto wnd2 = new GuiWindow(font2.get(), "Other test window");
        wnd2->pos = gfxm::vec2(850, 200);
        wnd2->size = gfxm::vec2(320, 800);
        wnd2->addChild(new GuiImage(&texture4));
        wnd->setDockPosition(GUI_DOCK::FILL);
        wnd2->setDockPosition(GUI_DOCK::RIGHT);
        gui_root->getRoot()->left->addWindow(wnd);
        gui_root->getRoot()->right->left->addWindow(wnd2);
        //auto wnd3 = new GuiWindow(font2.get(), "Free floating window");
        //wnd3->pos = gfxm::vec2(850, 200);
        //wnd3->size = gfxm::vec2(400, 700);
    }
}