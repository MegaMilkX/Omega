#pragma once

#include "gpu/gpu_pipeline.hpp"
#include "gpu/gpu_mesh.hpp"
#include "gpu/gpu_material.hpp"
#include "gpu/gpu_renderable.hpp"

#include "assimp_load_scene.hpp"

#include "input/input.hpp"

#include "typeface/typeface.hpp"
#include "typeface/font.hpp"
#include "gpu/gpu_uniform_buffer.hpp"
#include "gpu/gpu_text.hpp"

#include "common/collision/collision_world.hpp"

#include "gpu/render/uniform.hpp"

#include "gpu/gpu.hpp"

#include "gui/gui.hpp"

#include "game/world/world.hpp"

#include "resource/resource.hpp"

#include "config.hpp"


class Camera3d {
    InputContext inputCtx = InputContext("Camera");
    InputRange* inputTranslation;
    InputRange* inputRotation;
    InputAction* inputLeftClick;

    float cam_rot_y = 0;
    float cam_rot_x = 0;
    gfxm::mat4 proj;
    gfxm::vec3 cam_translation;
    gfxm::mat4 view;
    gfxm::mat4 inverse_view;
    gfxm::vec3 cam_wrld_translation;
    gfxm::quat qcam;
public:
    Camera3d() {
        inputTranslation = inputCtx.createRange("Translation");
        inputRotation = inputCtx.createRange("Rotation");
        inputLeftClick = inputCtx.createAction("LeftClick");
        inputTranslation
            ->linkKeyX(InputDeviceType::Keyboard, Key.Keyboard.A, -1.0f)
            .linkKeyX(InputDeviceType::Keyboard, Key.Keyboard.D, 1.0f)
            .linkKeyZ(InputDeviceType::Keyboard, Key.Keyboard.W, -1.0f)
            .linkKeyZ(InputDeviceType::Keyboard, Key.Keyboard.S, 1.0f);
        inputRotation
            ->linkKeyY(InputDeviceType::Mouse, Key.Mouse.AxisX, 1.0f)
            .linkKeyX(InputDeviceType::Mouse, Key.Mouse.AxisY, 1.0f);
        inputLeftClick
            ->linkKey(InputDeviceType::Mouse, Key.Mouse.BtnLeft);
    
        proj = gfxm::perspective(gfxm::radian(65), 16.0f / 9.0f, 0.01f, 1000.0f);
        //proj = gfxm::ortho(-500.0f, 500.0f, -500.0f, 500.0f, -10.0f, 10.0f);
        cam_translation = gfxm::vec3(-1.5f, 1.5f, 0.7f);
        view = gfxm::inverse(gfxm::lookAt(cam_translation, gfxm::vec3(0, 1, 0), gfxm::vec3(0, 1, 0)));
        inverse_view = gfxm::inverse(view);
        cam_wrld_translation = view * gfxm::vec4(0, 0, 0, 1);
        qcam = gfxm::to_quat(gfxm::to_mat3(view));
    }

    void update(float dt) {
        gfxm::vec3 cam_lcl_delta_translation = inputTranslation->getVec3();
        gfxm::vec3 cam_lcl_delta_rotation;
        cam_lcl_delta_rotation = inputRotation->getVec3();
        gfxm::vec3 cam_wrld_delta_translation = view * gfxm::vec4(cam_lcl_delta_translation, .0f);
        cam_wrld_translation += cam_wrld_delta_translation * (dt) * 6.0f;

        if (inputLeftClick->isPressed()) {
            cam_rot_y += cam_lcl_delta_rotation.y * (1.0f/60.f) * 0.5f; // don't use actual frame time here
            cam_rot_x += cam_lcl_delta_rotation.x * (1.0f/60.f) * 0.5f;
        }
        cam_rot_x = gfxm::clamp(cam_rot_x, -gfxm::pi * 0.5f, gfxm::pi * 0.5f);
        gfxm::quat qy = gfxm::angle_axis(cam_rot_y, gfxm::vec3(0, 1, 0));
        gfxm::quat qx = gfxm::angle_axis(cam_rot_x, gfxm::vec3(1, 0, 0));
        qcam = gfxm::slerp(qcam, qy * qx, 1 - pow(1 - 0.1f * 3.0f, dt * 60.0f)/* 0.1f*/);

        view = gfxm::translate(gfxm::mat4(1.0f), cam_wrld_translation) * gfxm::to_mat4(qcam);

        inverse_view = gfxm::inverse(view);
    }

    const gfxm::mat4& getProjection() const {
        return proj;
    }
    const gfxm::mat4& getView() const {
        return view;
    }
    const gfxm::mat4& getInverseView() const {
        return inverse_view;
    }
};
class Camera3dThirdPerson {
    InputContext inputCtx = InputContext("CameraThirdPerson");
    InputRange* inputRotation;
    InputAction* inputLeftClick;
    InputRange* inputScroll;

    gfxm::mat4 proj;
    gfxm::mat4 view;
    gfxm::mat4 inverse_view;

    gfxm::vec3 target_desired;
    gfxm::vec3 target_interpolated;
    
    float rotation_y = 0;
    float rotation_x = 0;
    float target_distance = 2.0f;
    gfxm::quat qcam;
public:
    Camera3dThirdPerson() {
        inputRotation = inputCtx.createRange("Rotation");
        inputLeftClick = inputCtx.createAction("LeftClick");
        inputScroll = inputCtx.createRange("Scroll");
        inputRotation
            ->linkKeyY(InputDeviceType::Mouse, Key.Mouse.AxisX, 1.0f)
            .linkKeyX(InputDeviceType::Mouse, Key.Mouse.AxisY, 1.0f);
        inputLeftClick
            ->linkKey(InputDeviceType::Mouse, Key.Mouse.BtnLeft);
        inputScroll
            ->linkKeyX(InputDeviceType::Mouse, Key.Mouse.Scroll, -1.0f);

        proj = gfxm::perspective(gfxm::radian(65), 16.0f / 9.0f, 0.01f, 1000.0f);
    }

    void setTarget(const gfxm::vec3& tgt) {
        target_desired = tgt;
    }

    void update(float dt) {
        gfxm::vec3 cam_lcl_delta_rotation;
        cam_lcl_delta_rotation = inputRotation->getVec3();

        if (inputLeftClick->isPressed()) {
            rotation_y += cam_lcl_delta_rotation.y * (1.0f / 60.f) * 0.5f; // don't use actual frame time here
            rotation_x += cam_lcl_delta_rotation.x * (1.0f / 60.f) * 0.5f;
        }

        float scroll = inputScroll->getVec3().x;
        float mul = scroll > .0f ? (target_distance / 3.0f) : (target_distance / 4.0f);
        target_distance += scroll * mul;
        target_distance = gfxm::_max(0.5f, target_distance);

        rotation_x = gfxm::clamp(rotation_x, -gfxm::pi * 0.48f, gfxm::pi * 0.25f);
        gfxm::quat qy = gfxm::angle_axis(rotation_y, gfxm::vec3(0, 1, 0));
        gfxm::quat qx = gfxm::angle_axis(rotation_x, gfxm::vec3(1, 0, 0));
        qcam = gfxm::slerp(qcam, qy * qx, 1 - pow(1 - 0.1f * 3.0f, dt * 60.0f)/* 0.1f*/);

        view = gfxm::to_mat4(qcam);
        gfxm::vec3 back_normal = gfxm::vec3(0, 0, 1);
        target_interpolated = gfxm::lerp(target_interpolated, target_desired, 1 - pow(1 - 0.1f * 3.0f, dt * 60.0f));
        gfxm::vec3 target_pos = target_interpolated;
        view = gfxm::translate(gfxm::mat4(1.0f), target_pos) * view * gfxm::translate(gfxm::mat4(1.0f), back_normal * target_distance);

        inverse_view = gfxm::inverse(view);
    }
    const gfxm::mat4& getProjection() const {
        return proj;
    }
    const gfxm::mat4& getView() const {
        return view;
    }
    const gfxm::mat4& getInverseView() const {
        return inverse_view;
    }
};

#include "character/character.hpp"


class CollisionDebugDraw : public CollisionDebugDrawCallbackInterface {
    std::vector<gfxm::vec3> vertices;
    std::vector<unsigned char> colors;
public:
    gpuBuffer gpu_vertices;
    gpuBuffer gpu_colors;
    gpuMeshDesc mesh_desc;
    HSHARED<gpuShaderProgram> shader_program;
    gpuMaterial* material;
    gpuRenderable renderable;

    CollisionDebugDraw(gpuPipeline* gpu_pipeline) {
        mesh_desc.setAttribArray(VFMT::Position_GUID, &gpu_vertices, 0);
        mesh_desc.setAttribArray(VFMT::ColorRGBA_GUID, &gpu_colors, 0);
        mesh_desc.setDrawMode(MESH_DRAW_LINES);
        mesh_desc.setVertexCount(0);

        const char* vs = R"(
        #version 450 
        in vec3 inPosition;
        in vec4 inColorRGBA;
        out vec4 frag_color;

        layout(std140) uniform bufCamera3d {
            mat4 matProjection;
            mat4 matView;
        };    

        void main(){
            frag_color = inColorRGBA;
            vec4 pos = vec4(inPosition, 1);
            gl_Position = matProjection * matView * pos;
        })";
        const char* fs = R"(
        #version 450
        in vec4 frag_color;
        out vec4 outAlbedo;
        void main(){
            outAlbedo = frag_color;
        })";
        shader_program.reset(HANDLE_MGR<gpuShaderProgram>::acquire());
        shader_program->init(vs, fs);
        
        material = gpu_pipeline->createMaterial();
        auto tech = material->addTechnique("Debug");
        auto pass = tech->addPass();
        pass->setShader(shader_program);
        material->compile();

        renderable.setMaterial(material);
        renderable.setMeshDesc(&mesh_desc);
        renderable.compile();
    }

    void flushDrawData() {
        gpu_vertices.setArrayData(vertices.data(), vertices.size() * sizeof(vertices[0]));
        gpu_colors.setArrayData(colors.data(), colors.size() * sizeof(colors[0]));
        mesh_desc.setVertexCount(vertices.size());
        vertices.clear();
        colors.clear();
    }

    void draw() {
        gpuDrawRenderable(&renderable);
    }
    
    void onDrawLines(const gfxm::vec3* vertices_ptr, int vertex_count, const gfxm::vec4& color) override {
        vertices.insert(vertices.end(), vertices_ptr, vertices_ptr + vertex_count);
        int colors_size = colors.size();
        colors.resize(colors_size + vertex_count * 4);
        for (int i = colors_size / 4; i < colors.size() / 4; ++i) {
            colors[i * 4] = color.r * 255.0f;
            colors[i * 4 + 1] = color.g * 255.0f;
            colors[i * 4 + 2] = color.b * 255.0f;
            colors[i * 4 + 3] = color.a * 255.0f;
        }
    }
};

#include "gpu/pipeline/gpu_pipeline_default.hpp"

constexpr int TEST_INSTANCE_COUNT = 500;
class GameCommon {
    wWorld world;

    InputContext inputCtxBox = InputContext("Box");
    InputRange* inputBoxTranslation;
    InputRange* inputBoxRotation;
    InputContext inputCtx = InputContext("main");
    InputAction* inputFButtons[12];
    InputContext inputCtxChara = InputContext("Character");
    InputRange* inputCharaTranslation;
    InputAction* inputCharaUse;

    std::unique_ptr<build_config::gpuPipelineCommon> gpu_pipeline;

    gpuUniformBuffer* ubufCam3d;
    gpuUniformBuffer* ubufTime;

    Camera3dThirdPerson cam;
    
    gpuMesh mesh;
    gpuMesh mesh_sphere;
    gpuMesh gpu_mesh_plane;

    gfxm::vec4          positions[TEST_INSTANCE_COUNT];
    gpuBuffer           inst_pos_buffer;
    gpuInstancingDesc   instancing_desc;

    RHSHARED<gpuShaderProgram> shader_default;
    RHSHARED<gpuShaderProgram> shader_vertex_color;
    RHSHARED<gpuShaderProgram> shader_text;
    RHSHARED<gpuShaderProgram> shader_instancing;

    RHSHARED<gpuTexture2d> texture;
    RHSHARED<gpuTexture2d> texture2;
    RHSHARED<gpuTexture2d> texture3;
    RHSHARED<gpuTexture2d> texture4;

    RHSHARED<gpuMaterial> material;
    RHSHARED<gpuMaterial> material2;
    RHSHARED<gpuMaterial> material3;
    RHSHARED<gpuMaterial> material_color;
    RHSHARED<gpuMaterial> material_instancing;

    std::unique_ptr<gpuRenderable> renderable;
    std::unique_ptr<gpuRenderable> renderable2;
    std::unique_ptr<gpuRenderable> renderable_plane;
    //gpuUniformBuffer* renderable_ubuf;
    gpuUniformBuffer* renderable2_ubuf;
    gpuUniformBuffer* renderable_plane_ubuf;

    // Text
    Typeface typeface;
    Typeface typeface_nimbusmono;
    std::unique_ptr<Font> font;
    std::unique_ptr<Font> font2;

    //
    Character chara;
    Character chara2;
    Door door;

    // Collision
    std::unique_ptr<CollisionDebugDraw> collision_debug_draw;
    CollisionWorld collision_world;
    CollisionSphereShape shape_sphere;
    CollisionBoxShape    shape_box;
    CollisionBoxShape    shape_box2;
    Collider collider_a;
    Collider collider_b;
    Collider collider_c;
    Collider collider_d;

    // gui?
    std::unique_ptr<GuiDockSpace> gui_root;
public:
    void Init();
    void Cleanup();

    void Update(float dt);
    void Draw(float dt);

    void onViewportResize(int width, int height);
};

extern GameCommon* g_game_comn;