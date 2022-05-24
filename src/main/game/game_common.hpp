#pragma once

#include "common/render/gpu_pipeline.hpp"
#include "common/render/gpu_mesh.hpp"
#include "common/render/gpu_material.hpp"
#include "common/render/gpu_renderable.hpp"

#include "assimp_load_scene.hpp"

#include "common/input/input.hpp"

#include "common/typeface/typeface.hpp"
#include "common/typeface/font.hpp"
#include "common/render/gpu_uniform_buffer.hpp"
#include "common/render/gpu_text.hpp"

#include "common/collision/collision_world.hpp"

#include "render/uniform.hpp"

#include "common/render/render.hpp"

#include "common/gui/gui.hpp"

class SceneNode {
    SceneNode* parent = 0;
    SceneNode* first_child = 0;
    SceneNode* next_sibling = 0;
    gfxm::mat4 world_transform = gfxm::mat4(1.0f);
    gfxm::mat4 local_transform = gfxm::mat4(1.0f);
    gfxm::vec3 translation = gfxm::vec3(0,0,0);
    gfxm::quat rotation = gfxm::quat(0,0,0,1);
    gfxm::vec3 scale = gfxm::vec3(1,1,1);
    bool dirty = true;

    void setDirty() {
        if (dirty) {
            return;
        }
        dirty = true;
        SceneNode* current_child = first_child;
        while (current_child != 0) {
            current_child->setDirty();
            current_child = current_child->next_sibling;
        }
    }
public:
    ~SceneNode() {
        SceneNode* current_child = first_child;
        while (current_child != 0) {
            delete current_child;
            current_child = current_child->next_sibling;
        }
    }

    SceneNode* createChild() {
        SceneNode* node = new SceneNode();
        node->parent = this;
        
        SceneNode* last_sibling = 0;
        SceneNode* current_child = first_child;
        while (current_child != 0) {
            last_sibling = current_child;
            current_child = current_child->next_sibling;
        }
        last_sibling->next_sibling = node;
    }

    void setTranslation(const gfxm::vec3& t) {
        setDirty();
        translation = t;
    }
    void setRotation(const gfxm::quat& q) {
        setDirty();
        rotation = q;
    }
    void setScale(const gfxm::vec3& s) {
        setDirty();
        scale = s;
    }
    const gfxm::mat4& getLocalTransform() {
        return local_transform
            = gfxm::translate(gfxm::mat4(1.0f), translation)
            * gfxm::to_mat4(rotation)
            * gfxm::scale(gfxm::mat4(1.0f), scale);
    }
    const gfxm::mat4& getWorldTransform() {
        if (dirty) {
            if (parent) {
                world_transform = parent->getWorldTransform() * getLocalTransform();
            } else {
                world_transform = getLocalTransform();
            }
            dirty = false;
        }
        return world_transform;
    }
};


class SceneMesh {
    gpuUniformBuffer*                   ubuf_model;
    int                                 loc_model;

public:
    gfxm::mat4                          transform;
    gpuRenderable                       renderable;

    SceneMesh(gpuPipeline* pipeline)
    : transform(1.0f) {
        auto ubuf_model_desc = pipeline->getUniformBufferDesc(UNIFORM_BUFFER_MODEL);
        loc_model = ubuf_model_desc->getUniform(UNIFORM_MODEL_TRANSFORM);
        ubuf_model = pipeline->createUniformBuffer(UNIFORM_BUFFER_MODEL);
        
        renderable.attachUniformBuffer(ubuf_model);
    }

    void update() {
        ubuf_model->setMat4(loc_model, transform);
    }
};

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
        inputRotation
            ->linkKeyY(InputDeviceType::Mouse, Key.Mouse.AxisX, 1.0f)
            .linkKeyX(InputDeviceType::Mouse, Key.Mouse.AxisY, 1.0f);
        inputLeftClick
            ->linkKey(InputDeviceType::Mouse, Key.Mouse.BtnLeft);

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



struct Model3d {
    std::vector<std::unique_ptr<gpuMesh>>       gpu_meshes;
    std::vector<std::unique_ptr<gpuRenderable>> gpu_renderables;
    std::vector<gpuUniformBuffer*>              gpu_renderable_ubufs;

    void init(gpuPipeline* pipeline, const ImportedScene& imported, gpuRenderMaterial* material) {
        gpu_meshes.clear();
        gpu_renderables.clear();
        gpu_renderable_ubufs.clear();

        gpu_meshes.resize(imported.meshes.size());
        for (int i = 0; i < imported.meshes.size(); ++i) {
            gpu_meshes[i].reset(new gpuMesh);
            gpu_meshes[i]->setData(&imported.meshes[i]);
        }

        gpu_renderables.resize(imported.mesh_instances.size());
        gpu_renderable_ubufs.resize(imported.mesh_instances.size());
        for (int i = 0; i < imported.mesh_instances.size(); ++i) {
            auto ubuf = pipeline->createUniformBuffer(UNIFORM_BUFFER_MODEL);
            ubuf->setMat4(ubuf->getDesc()->getUniform(UNIFORM_MODEL_TRANSFORM), imported.world_transforms[imported.mesh_instances[i].transform_id]);
            gpu_renderable_ubufs[i] = ubuf;

            gpu_renderables[i].reset(new gpuRenderable);
            gpu_renderables[i]->setMaterial(material);
            gpu_renderables[i]->setMeshDesc(gpu_meshes[imported.mesh_instances[i].mesh_id].get()->getMeshDesc());
            gpu_renderables[i]->attachUniformBuffer(ubuf);
            gpu_renderables[i]->compile();
        }
    }

    void draw() {
        for (int i = 0; i < gpu_renderables.size(); ++i) {
            auto r = gpu_renderables[i].get();
            gpuDrawRenderable(r);
        }
    }
};


class CollisionDebugDraw : public CollisionDebugDrawCallbackInterface {
    std::vector<gfxm::vec3> vertices;
    std::vector<unsigned char> colors;
public:
    gpuBuffer gpu_vertices;
    gpuBuffer gpu_colors;
    gpuMeshDesc mesh_desc;
    std::unique_ptr<gpuShaderProgram> shader_program;
    gpuRenderMaterial* material;
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
        shader_program.reset(new gpuShaderProgram(vs, fs));
        
        material = gpu_pipeline->createMaterial();
        auto tech = material->addTechnique("Debug");
        auto pass = tech->addPass();
        pass->setShader(shader_program.get());
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

class GameCommon {
    InputContext inputCtxBox = InputContext("Box");
    InputRange* inputBoxTranslation;
    InputRange* inputBoxRotation;
    InputContext inputCtx = InputContext("main");
    InputAction* inputFButtons[12];
    InputContext inputCtxChara = InputContext("Character");
    InputRange* inputCharaTranslation;
    InputAction* inputCharaUse;

    std::unique_ptr<gpuPipeline> gpu_pipeline;

    std::unique_ptr<gpuFrameBuffer> frame_buffer;
    std::unique_ptr<gpuFrameBuffer> fb_color;
    std::unique_ptr<gpuTexture2d> tex_albedo;
    std::unique_ptr<gpuTexture2d> tex_depth;

    gpuUniformBufferDesc* ubufCam3dDesc;
    gpuUniformBufferDesc* ubufTimeDesc;
    gpuUniformBufferDesc* ubufModelDesc;
    gpuUniformBufferDesc* ubufTextDesc;
    gpuUniformBuffer* ubufCam3d;
    gpuUniformBuffer* ubufTime;
    gpuUniformBuffer* ubufText; // TODO: Should be a buffer per font

    Camera3dThirdPerson cam;
    
    gpuMesh mesh;
    gpuMesh mesh_sphere;
    gpuMesh gpu_mesh_plane;

    gfxm::vec4          positions[100];
    gpuBuffer           inst_pos_buffer;
    gpuInstancingDesc   instancing_desc;

    ImportedScene importedScene;

    gpuShaderProgram* shader_default;
    gpuShaderProgram* shader_vertex_color;
    gpuShaderProgram* shader_text;
    gpuShaderProgram* shader_instancing;

    gpuTexture2d texture;
    gpuTexture2d texture2;
    gpuTexture2d texture3;
    gpuTexture2d texture4;
    gpuTexture2d tex_font_atlas;
    gpuTexture2d tex_font_lookup;

    gpuRenderMaterial* material;
    gpuRenderMaterial* material2;
    gpuRenderMaterial* material3;
    gpuRenderMaterial* material_color;
    gpuRenderMaterial* material_text;
    gpuRenderMaterial* material_instancing;

    std::unique_ptr<gpuRenderable> renderable;
    std::unique_ptr<gpuRenderable> renderable2;
    std::unique_ptr<gpuRenderable> renderable_plane;
    std::unique_ptr<gpuRenderable> renderable_text;
    //gpuUniformBuffer* renderable_ubuf;
    gpuUniformBuffer* renderable2_ubuf;
    gpuUniformBuffer* renderable_plane_ubuf;
    gpuUniformBuffer* renderable_text_ubuf;

    std::unique_ptr<Model3d> model_3d;
    std::unique_ptr<SceneMesh> scene_mesh;

    // Text
    Typeface typeface;
    Typeface typeface_nimbusmono;
    std::unique_ptr<Font> font;
    std::unique_ptr<Font> font2;
    std::unique_ptr<gpuText> gpu_text;

    //
    Character chara;
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