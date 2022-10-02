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


struct GameRenderInstance {
    gpuRenderTarget* render_target;
    gpuRenderBucket* render_bucket;
};
std::vector<GameRenderInstance> game_render_instances;


class GuiViewport : public GuiElement {
public:
    gpuRenderTarget render_target;
    gpuRenderBucket render_bucket;
    gpuMesh mesh;
    Mesh3d mesh_ram;
    std::unique_ptr<gpuRenderable> renderable;

    GuiViewport()
    : render_bucket(gpuGetPipeline(), 10000) {
        gpuGetPipeline()->initRenderTarget(&render_target);

        meshGenerateCube(&mesh_ram);
        mesh.setData(&mesh_ram);

        gpuUniformBuffer* ubufRenderable = gpuGetPipeline()->createUniformBuffer(UNIFORM_BUFFER_MODEL);
        ubufRenderable->setMat4(
            gpuGetPipeline()->getUniformBufferDesc(UNIFORM_BUFFER_MODEL)->getUniform("matModel"),
            gfxm::mat4(1.0f)
        );

        RHSHARED<gpuMaterial> material = resGet<gpuMaterial>("materials/default3.mat");
        renderable.reset(new gpuRenderable);
        renderable->setMaterial(material.get());
        renderable->setMeshDesc(mesh.getMeshDesc());
        renderable->attachUniformBuffer(ubufRenderable);
        renderable->compile();

        game_render_instances.push_back(
            GameRenderInstance{
                &render_target, &render_bucket
            }
        );
    }

    void onMessage(GUI_MSG msg, uint64_t a_param, uint64_t b_param) override {
        switch (msg) {/*
        case GUI_MSG::SIZE: {
            // TODO
        } break;*/
        }
    }
    GuiHitResult hitTest(int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }

        return GuiHitResult{ GUI_HIT::CLIENT, this };
    }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        bounding_rect = rc;
        client_area = bounding_rect;
    }
    void onDraw() override {
        render_bucket.clear();
        render_bucket.add(renderable.get());

        guiDrawRectTextured(client_area, render_target.textures[0].get(), GUI_COL_WHITE);
        //guiDrawRect(client_area, GUI_COL_BLACK);
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

int main(int argc, char* argv) {
    platformInit(true, true);
    gpuInit(new build_config::gpuPipelineCommon());
    typefaceInit();
    Typeface typeface_nimbusmono;
    typefaceLoad(&typeface_nimbusmono, "nimbusmono-bold.otf");
    Font* fnt = new Font(&typeface_nimbusmono, 13, 72);
    guiInit(fnt);

    std::unique_ptr<GuiDockSpace> gui_root;
    gui_root.reset(new GuiDockSpace);
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
    wnd->addChild(new GuiImage(resGet<gpuTexture2d>("1648920106773.jpg").get()));
    wnd->addChild(new GuiButton());
    wnd->addChild(new GuiButton());
    auto wnd2 = new GuiWindow("2 Other test window");
    wnd2->pos = gfxm::vec2(850, 200);
    wnd2->size = gfxm::vec2(320, 800);
    wnd2->addChild(new GuiImage(resGet<gpuTexture2d>("effect_004.png").get()));
    gui_root->getRoot()->left->addWindow(wnd);
    gui_root->getRoot()->right->left->addWindow(wnd2);
    auto wnd3 = new GuiWindow("3 Third test window");
    wnd3->pos = gfxm::vec2(850, 200);
    wnd3->size = gfxm::vec2(400, 700);
    //wnd3->addChild(new GuiImage(gpuGetPipeline()->tex_albedo.get()));
    auto gui_viewport = new GuiViewport;
    wnd3->addChild(gui_viewport);
    gui_root->getRoot()->right->right->addWindow(wnd3);
    auto wnd4 = new GuiDemoWindow();

    gpuUniformBuffer* ubufCam3d = gpuGetPipeline()->createUniformBuffer(UNIFORM_BUFFER_CAMERA_3D);
    gpuGetPipeline()->attachUniformBuffer(ubufCam3d);
    ubufCam3d->setMat4(
        gpuGetPipeline()->getUniformBufferDesc(UNIFORM_BUFFER_CAMERA_3D)->getUniform("matProjection"),
        gfxm::perspective(gfxm::radian(90.0f), 16.0f / 9.0f, 0.01f, 1000.0f)
    );
    ubufCam3d->setMat4(
        gpuGetPipeline()->getUniformBufferDesc(UNIFORM_BUFFER_CAMERA_3D)->getUniform("matView"),
        gfxm::inverse(gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(0,.0f,1.8f)))
    );

    //timer timer_;
    float dt = 1.0f / 60.0f;
    while (platformIsRunning()) {
        //timer_.start();
        platformPollMessages();
        inputUpdate(dt);

        // Process and render world instances
        for (int i = 0; i < game_render_instances.size(); ++i) {
            auto& inst = game_render_instances[i];
            gpuDraw(inst.render_bucket, inst.render_target);
        }
        
        gpuFrameBufferUnbind();
        guiLayout();
        guiDraw();

        //gpuDrawTextureToDefaultFrameBuffer(gpuGetPipeline()->tex_albedo.get());

        platformSwapBuffers();
        //dt = timer_.stop();
    }

    gui_root.reset();
    guiCleanup();
    typefaceCleanup();
    gpuCleanup();
    platformCleanup();
    return 0;
}