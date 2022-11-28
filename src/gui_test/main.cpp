#include <Windows.h>

#include <assert.h>

#include <string>

#include "platform/gl/glextutil.h"
#include "log/log.hpp"

#include <unordered_map>
#include <memory>

#include "window/window.hpp"
#include "gpu/gpu.hpp"
#include "gui/gui.hpp"
#include "typeface/font.hpp"
#include "resource/resource.hpp"
#include "input/input.hpp"
#include "mesh3d/generate_primitive.hpp"

#include "skeletal_model/skeletal_model.hpp"
#include "world/world.hpp"
#include "world/node/node_character_capsule.hpp"
#include "world/node/node_skeletal_model.hpp"
#include "world/component/components.hpp"
#include "world/controller/actor_controllers.hpp"

static float g_dt = 1.0f / 60.0f;
static float g_timeline_cursor = .0f;

class GuiTimelineWindow;

#include "animation/animator/animator_sequence.hpp"
struct SequenceEditorData {
    bool is_playing = false;
    float timeline_cursor = .0f;
    RHSHARED<sklSkeletonMaster> skeleton;
    RHSHARED<animAnimatorSequence> sequence;
    animSampler sampler;
    animSampleBuffer samples;
    std::set<sklSkeletonInstance*> skeleton_instances;
    gameActor* actor;
    GuiTimelineWindow* tl_window;
};

class GuiTimelineWindow : public GuiWindow {
    std::unique_ptr<GuiTimelineContainer> tl;
    SequenceEditorData* data;
public:
    GuiTimelineWindow(SequenceEditorData* data)
        : GuiWindow("TimelineWindow"), data(data) {
        size = gfxm::vec2(800, 300);

        tl.reset(new GuiTimelineContainer);
        tl->setOwner(this);
        addChild(tl.get());

        tl->on_play = [data]() {
            data->is_playing = true;
        };
        tl->on_pause = [data]() {
            data->is_playing = false;
        };
        tl->on_cursor = [data](int cur) {
            data->timeline_cursor = cur;
        };
    }
    void setCursor(float cur) {
        tl->setCursorSilent(cur);
    }
};
class EditorGuiSequenceResourceList : public GuiWindow {
    std::unique_ptr<GuiContainer> container;
public:
    EditorGuiSequenceResourceList()
    : GuiWindow("Sequence resources") {
        setSize(400, 500);
        setMinSize(200, 250);
        container.reset(new GuiContainer);
        container->setOwner(this);
        addChild(container.get());
    }
};

void sequenceEditorInit(
    SequenceEditorData& data,
    RHSHARED<sklSkeletonMaster> skl,
    RHSHARED<animAnimatorSequence> sequence,
    gameActor* actor,
    GuiTimelineWindow* tl_window
) {
    data.skeleton_instances.clear();
    data.skeleton = skl;
    data.sequence = sequence;
    data.sampler = animSampler(skl.get(), sequence->getSkeletalAnimation().get());
    data.samples.init(skl.get());
    actor->forEachNode<nodeSkeletalModel>([&data](nodeSkeletalModel* node) {
        data.skeleton_instances.insert(node->getModelInstance()->getSkeletonInstance());
    });
    data.actor = actor;
    data.tl_window = tl_window;
}
void sequenceEditorUpdateAnimFrame(SequenceEditorData& data) {
    data.sampler = animSampler(data.skeleton.get(), data.sequence->getSkeletalAnimation().get());
    data.samples.has_root_motion = false;
    for (auto& skl_inst : data.skeleton_instances) {
        auto model_inst_skel = skl_inst->getSkeletonMaster();
        if (data.skeleton.get() != model_inst_skel) {
            continue;
        }
        data.sampler.sample(&data.samples[0], data.samples.count(), data.timeline_cursor);
        data.samples.applySamples(skl_inst);
    }
    if (data.is_playing) {
        data.timeline_cursor += data.sequence->getSkeletalAnimation()->fps * g_dt;
        if (data.timeline_cursor > data.sequence->getSkeletalAnimation()->length) {
            data.timeline_cursor -= data.sequence->getSkeletalAnimation()->length;
        }
        data.tl_window->setCursor(data.timeline_cursor);
    }
}

struct GameRenderInstance {
    gameWorld world;
    gpuRenderTarget* render_target;
    gpuRenderBucket* render_bucket;
    gfxm::vec2 viewport_size;
    gfxm::mat4 view_transform;
};
std::vector<GameRenderInstance*> game_render_instances;



class GuiViewport : public GuiElement {
public:
    gfxm::vec2 last_mouse_pos;
    float cam_angle_y = .0f;
    float cam_angle_x = .0f;
    bool dragging = false;

    GameRenderInstance* render_instance;

    GuiViewport() {}

    void onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::MOUSE_MOVE: {
            gfxm::vec2 mouse_pos = gfxm::vec2(params.getA<int32_t>(), params.getB<int32_t>());
            if (dragging) {
                cam_angle_x -= mouse_pos.y - last_mouse_pos.y;
                cam_angle_y -= mouse_pos.x - last_mouse_pos.x;
            }
            last_mouse_pos = mouse_pos;
            } break;
        case GUI_MSG::LBUTTON_DOWN:
            dragging = true;
            guiCaptureMouse(this);
            break;
        case GUI_MSG::LBUTTON_UP:
            dragging = false;
            guiCaptureMouse(0);
            break;
        }
    }
    GuiHitResult hitTest(int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }

        return GuiHitResult{ GUI_HIT::CLIENT, this };
    }
    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) override {
        bounding_rect = rc;
        client_area = bounding_rect;
        render_instance->viewport_size = gfxm::vec2(
            client_area.max - client_area.min
        );
        gfxm::quat qx = gfxm::angle_axis(gfxm::radian(cam_angle_x), gfxm::vec3(1, 0, 0));
        gfxm::quat qy = gfxm::angle_axis(gfxm::radian(cam_angle_y), gfxm::vec3(0, 1, 0));
        gfxm::quat q = qy * qx;
        gfxm::mat4 m = gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(0, 1, 0)) * gfxm::to_mat4(q);
        m = gfxm::translate(m, gfxm::vec3(0,0,1) * 2.0f);
        render_instance->view_transform = gfxm::inverse(m);
    }
    void onDraw() override {
        guiDrawRectTextured(client_area, render_instance->render_target->textures[0].get(), GUI_COL_WHITE);
    }
};

static void gpuDrawTextureToDefaultFrameBuffer(gpuTexture2d* texture) {
    int screen_w = 0, screen_h = 0;
    platformGetWindowSize(screen_w, screen_h);

    const char* vs = R"(
        #version 450 
        in vec3 inPosition;
        out vec2 uv_frag;
        
        void main(){
            uv_frag = vec2((inPosition.x + 1.0) / 2.0, (inPosition.y + 1.0) / 2.0);
            vec4 pos = vec4(inPosition, 1);
            gl_Position = pos;
        })";
    const char* fs = R"(
        #version 450
        in vec2 uv_frag;
        out vec4 outAlbedo;
        uniform sampler2D texAlbedo;
        void main(){
            vec4 pix = texture(texAlbedo, uv_frag);
            float a = pix.a;
            outAlbedo = vec4(pix.rgb, 1);
        })";
    gpuShaderProgram prog(vs, fs);
    float vertices[] = {
        -1.0f, -1.0f, 0.0f,     3.0f, -1.0f, 0.0f,      -1.0f, 3.0f, 0.0f
    };

    GLuint gvao;
    glGenVertexArrays(1, &gvao);
    glBindVertexArray(gvao);

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0,
        3, GL_FLOAT, GL_FALSE,
        0, (void*)0 /* offset */
    );

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glViewport(0, 0, screen_w, screen_h);
    glScissor(0, 0, screen_w, screen_h);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture->getId());
    glUseProgram(prog.getId());
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glUseProgram(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindVertexArray(0);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &gvao);
}

#include "gui_cdt_test_window.hpp"
#include "util/timer.hpp"
int main(int argc, char* argv) {
    platformInit(true, true);
    gpuInit(new build_config::gpuPipelineCommon());
    typefaceInit();
    Typeface typeface_nimbusmono;
    typefaceLoad(&typeface_nimbusmono, "nimbusmono-bold.otf");
    Font* fnt = new Font(&typeface_nimbusmono, 12, 72);
    guiInit(fnt);

    resInit();
    animInit();

    SequenceEditorData seqed_data;

    std::unique_ptr<GuiDockSpace> gui_root;
    gui_root.reset(new GuiDockSpace);
    int screen_width = 0, screen_height = 0;
    platformGetWindowSize(screen_width, screen_height);

    gui_root.reset(new GuiDockSpace());
    gui_root->getRoot()->splitTop();
    //gui_root.getRoot()->left->split();
    gui_root->getRoot()->right->splitLeft();
    gui_root->pos = gfxm::vec2(0.0f, 0.0f);
    gui_root->size = gfxm::vec2(screen_width, screen_height);

    auto wnd = new GuiWindow("1 Test window");
    wnd->pos = gfxm::vec2(120, 160);
    wnd->size = gfxm::vec2(640, 700);
    wnd->addChild(new GuiTextBox());
    wnd->addChild(new GuiImage(resGet<gpuTexture2d>("1648920106773.jpg").get()));
    wnd->addChild(new GuiButton());
    wnd->addChild(new GuiButton());
    auto wnd2 = new GuiWindow("2 Other test window");
    wnd2->pos = gfxm::vec2(850, 200);
    wnd2->size = gfxm::vec2(320, 800);
    wnd2->addChild(new GuiImage(resGet<gpuTexture2d>("effect_004.png").get()));
    auto wnd3 = new GuiWindow("Viewport");
    wnd3->content_padding = gfxm::rect(0, 0, 0, 0);
    wnd3->pos = gfxm::vec2(850, 200);
    wnd3->size = gfxm::vec2(400, 700);
    //wnd3->addChild(new GuiImage(gpuGetPipeline()->tex_albedo.get()));
    auto gui_viewport = new GuiViewport;
    wnd3->addChild(gui_viewport);
    gui_root->getRoot()->right->right->addWindow(wnd3);
    auto wnd4 = new GuiDemoWindow();
    auto wnd6 = new GuiFileExplorerWindow();
    auto wnd7 = new GuiNodeEditorWindow();
    auto wnd8 = new GuiTimelineWindow(&seqed_data);
    auto wnd9 = new EditorGuiSequenceResourceList();
    auto wnd10 = new GuiCdtTestWindow();

    gui_root->getRoot()->left->addWindow(wnd);
    gui_root->getRoot()->left->addWindow(wnd2);
    gui_root->getRoot()->left->addWindow(wnd3);
    gui_root->getRoot()->left->splitLeft();
    gui_root->getRoot()->left->left->addWindow(wnd4);
    gui_root->getRoot()->right->left->addWindow(wnd7);
    gui_root->getRoot()->right->left->addWindow(wnd6);
    gui_root->getRoot()->right->left->addWindow(wnd9);
    gui_root->getRoot()->right->right->addWindow(wnd8);
    gui_root->getRoot()->split_pos = 0.7f;
    gui_root->getRoot()->left->split_pos = 0.20f;
    gui_root->getRoot()->right->split_pos = 0.3f;

    gpuRenderTarget render_target;
    gpuGetPipeline()->initRenderTarget(&render_target);
    gpuRenderBucket render_bucket(gpuGetPipeline(), 1000);
    RHSHARED<animAnimatorSequence> seq_run;
    {
        seq_run.reset_acquire();
        seq_run->setSkeletalAnimation(resGet<Animation>("models/chara_24/Run2.animation"));
    }
    gameActor actor;
    {
        auto root = actor.setRoot<nodeCharacterCapsule>("capsule");
        auto node = root->createChild<nodeSkeletalModel>("model");
        node->setModel(resGet<mdlSkeletalModelMaster>("models/chara_24/chara_24.skeletal_model"));
    }
    GameRenderInstance render_instance;
    render_instance.world.spawnActor(&actor);
    render_instance.render_bucket = &render_bucket;
    render_instance.render_target = &render_target;
    render_instance.viewport_size = gfxm::vec2(100, 100);
    render_instance.view_transform = gfxm::mat4(1.0f);
    game_render_instances.push_back(&render_instance);
    gui_viewport->render_instance = &render_instance;
    
    sequenceEditorInit(
        seqed_data,
        resGet<sklSkeletonMaster>("models/chara_24/chara_24.skeleton"),
        seq_run,
        &actor,
        wnd8
    );

    RHSHARED<gpuMaterial> material_color = resGet<gpuMaterial>("materials/color.mat");
    Mesh3d mesh_plane;
    meshGenerateCheckerPlane(&mesh_plane, 50, 50, 50);
    gpuMesh gpu_mesh_plane;
    gpu_mesh_plane.setData(&mesh_plane);
    std::unique_ptr<gpuRenderable> renderable_plane;
    renderable_plane.reset(new gpuRenderable(material_color.get(), gpu_mesh_plane.getMeshDesc()));
    gpuUniformBuffer* renderable_plane_ubuf = gpuGetPipeline()->createUniformBuffer(UNIFORM_BUFFER_MODEL);
    renderable_plane->attachUniformBuffer(renderable_plane_ubuf);
    renderable_plane_ubuf->setMat4(
        gpuGetPipeline()->getUniformBufferDesc(UNIFORM_BUFFER_MODEL)->getUniform("matModel"),
        gfxm::mat4(1.f)
    );

    gpuUniformBuffer* ubufCam3d = gpuGetPipeline()->createUniformBuffer(UNIFORM_BUFFER_CAMERA_3D);
    gpuGetPipeline()->attachUniformBuffer(ubufCam3d);    
    

    timer timer_;
    while (platformIsRunning()) {
        timer_.start();
        platformPollMessages();
        inputUpdate(g_dt);
        
        gpuFrameBufferUnbind();
        guiLayout();
        guiDraw();

        sequenceEditorUpdateAnimFrame(seqed_data);
        // Process and render world instances
        for (int i = 0; i < game_render_instances.size(); ++i) {
            auto& inst = game_render_instances[i];
            inst->render_target->setSize(inst->viewport_size.x, inst->viewport_size.y);
            inst->world.update(g_dt);
            inst->render_bucket->clear();
            inst->render_bucket->add(renderable_plane.get());
            inst->world.getRenderScene()->draw(&render_bucket);
            ubufCam3d->setMat4(
                gpuGetPipeline()->getUniformBufferDesc(UNIFORM_BUFFER_CAMERA_3D)->getUniform("matProjection"),
                gfxm::perspective(gfxm::radian(65.0f), inst->viewport_size.x / inst->viewport_size.y, 0.01f, 1000.0f)
            );
            ubufCam3d->setMat4(
                gpuGetPipeline()->getUniformBufferDesc(UNIFORM_BUFFER_CAMERA_3D)->getUniform("matView"),
                inst->view_transform
            );
            gpuDraw(inst->render_bucket, inst->render_target);
        }

        guiRender();

        //gpuDrawTextureToDefaultFrameBuffer(gpuGetPipeline()->tex_albedo.get());

        platformSwapBuffers();
        g_dt = timer_.stop();
    }

    resCleanup();
    animCleanup();

    gui_root.reset();
    guiCleanup();
    typefaceCleanup();
    gpuCleanup();
    platformCleanup();
    return 0;
}