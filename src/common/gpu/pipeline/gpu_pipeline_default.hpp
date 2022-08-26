#pragma once


#include "gpu/gpu_pipeline.hpp"
#include "gpu/render/uniform.hpp"
#include "platform/platform.hpp"


class gpuPipelineDefault : public gpuPipeline {
    void onViewportResize(int width, int height) override {
        tex_albedo->changeFormat(GL_RGB, width, height, 3);
        tex_depth->changeFormat(GL_DEPTH_COMPONENT, width, height, 1);
    }

public:
    std::unique_ptr<gpuFrameBuffer> frame_buffer;
    std::unique_ptr<gpuFrameBuffer> fb_color;
    HSHARED<gpuTexture2d> tex_albedo;
    HSHARED<gpuTexture2d> tex_depth;

    gpuPipelineDefault() {
        int screen_width = 0, screen_height = 0;
        platformGetWindowSize(screen_width, screen_height);

        tex_albedo.reset(HANDLE_MGR<gpuTexture2d>::acquire());
        tex_depth.reset(HANDLE_MGR<gpuTexture2d>::acquire());
        tex_albedo->changeFormat(GL_RGB, screen_width, screen_height, 3);
        tex_depth->changeFormat(GL_DEPTH_COMPONENT, screen_width, screen_height, 1);

        fb_color.reset(new gpuFrameBuffer());
        fb_color->addColorTarget("Albedo", tex_albedo.get());
        if (!fb_color->validate()) {
            LOG_ERR("Color only framebuffer is not valid");
        }
        fb_color->prepare();

        frame_buffer.reset(new gpuFrameBuffer());
        frame_buffer->addColorTarget("Albedo", tex_albedo.get());
        frame_buffer->addDepthTarget(tex_depth);
        if (!frame_buffer->validate()) {
            LOG_ERR("Framebuffer not valid!");
        }
        frame_buffer->prepare();

        auto tech = createTechnique("Normal", 1);
        tech->getPass(0)->setFrameBuffer(frame_buffer.get());
        tech = createTechnique("Decals", 1);
        tech->getPass(0)->setFrameBuffer(fb_color.get());
        tech = createTechnique("VFX", 1);
        tech->getPass(0)->setFrameBuffer(frame_buffer.get());
        tech = createTechnique("GUI", 1);
        tech->getPass(0)->setFrameBuffer(frame_buffer.get());
        tech = createTechnique("Debug", 1);
        tech->getPass(0)->setFrameBuffer(frame_buffer.get());

        compile();

        createUniformBufferDesc(UNIFORM_BUFFER_CAMERA_3D)
            ->define(UNIFORM_PROJECTION, UNIFORM_MAT4)
            .define(UNIFORM_VIEW_TRANSFORM, UNIFORM_MAT4)
            .compile();
        createUniformBufferDesc(UNIFORM_BUFFER_TIME)
            ->define(UNIFORM_TIME, UNIFORM_FLOAT)
            .compile();
        createUniformBufferDesc(UNIFORM_BUFFER_MODEL)
            ->define(UNIFORM_MODEL_TRANSFORM, UNIFORM_MAT4)
            .compile();
        createUniformBufferDesc(UNIFORM_BUFFER_TEXT)
            ->define(UNIFORM_TEXT_LOOKUP_TEXTURE_WIDTH, UNIFORM_INT)
            .compile();
        createUniformBufferDesc(UNIFORM_BUFFER_DECAL)
            ->define("boxSize", UNIFORM_VEC3)
            .define("RGBA", UNIFORM_VEC4)
            .define("screenSize", UNIFORM_VEC2)
            .compile();
    }
};