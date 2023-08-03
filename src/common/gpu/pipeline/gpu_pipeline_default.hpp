#pragma once


#include "gpu/gpu_pipeline.hpp"
#include "gpu/render/uniform.hpp"
#include "platform/platform.hpp"


class gpuPipelineDefault : public gpuPipeline {
    gpuUniformBuffer* ubufCamera3d = 0;
    gpuUniformBuffer* ubufShadowmapCamera3d = 0;
    gpuUniformBuffer* ubufTime = 0;
    gpuUniformBuffer* ubufModel = 0;
    gpuUniformBuffer* ubufDecal = 0;
    int loc_projection;
    int loc_view;
    int loc_screenSize;
    int loc_shadowmap_projection;
    int loc_shadowmap_view;
    int loc_time;
    int loc_model;
    int loc_boxSize;
    int loc_color;
public:
    gpuPipelineDefault() {
        addColorRenderTarget("Albedo", GL_RGB);
        addColorRenderTarget("Position", GL_RGB32F);
        addColorRenderTarget("Normal", GL_RGB32F);
        addColorRenderTarget("Metalness", GL_RED);
        addColorRenderTarget("Roughness", GL_RED);
        addColorRenderTarget("Emission", GL_RGB);
        addColorRenderTarget("Lightness", GL_RGB32F);
        addColorRenderTarget("Final", GL_RGB32F);
        addDepthRenderTarget("Depth");

        auto tech = createTechnique("Normal", 1);
        tech->getPass(0)->setColorTarget("Albedo", "Albedo");
        tech->getPass(0)->setColorTarget("Position", "Position");
        tech->getPass(0)->setColorTarget("Normal", "Normal");
        tech->getPass(0)->setColorTarget("Metalness", "Metalness");
        tech->getPass(0)->setColorTarget("Roughness", "Roughness");
        tech->getPass(0)->setColorTarget("Emission", "Emission");
        tech->getPass(0)->setDepthTarget("Depth");
        tech = createTechnique("Decals", 1);
        tech->getPass(0)->setTargetSampler("Depth");
        tech->getPass(0)->setColorTarget("Albedo", "Final");
        tech = createTechnique("VFX", 1);
        tech->getPass(0)->setColorTarget("Albedo", "Final");
        tech->getPass(0)->setDepthTarget("Depth");
        tech = createTechnique("GUI", 1);
        tech->getPass(0)->setColorTarget("Albedo", "Final");
        tech->getPass(0)->setDepthTarget("Depth");
        tech = createTechnique("Debug", 1);
        tech->getPass(0)->setColorTarget("Albedo", "Albedo");
        tech->getPass(0)->setDepthTarget("Depth");
        tech = createTechnique("LightPass", 1);
        tech->getPass(0)->setColorTarget("Lightness", "Lightness");
        tech->getPass(0)->setTargetSampler("Albedo");
        tech->getPass(0)->setTargetSampler("Position");
        tech->getPass(0)->setTargetSampler("Normal");
        tech->getPass(0)->setTargetSampler("Metalness");
        tech->getPass(0)->setTargetSampler("Roughness");
        tech->getPass(0)->setTargetSampler("Emission");
        tech = createTechnique("PBRCompose", 1);
        tech->getPass(0)->setColorTarget("Final", "Final");
        tech->getPass(0)->setTargetSampler("Albedo");
        tech->getPass(0)->setTargetSampler("Lightness");
        tech->getPass(0)->setTargetSampler("Emission");
        tech = createTechnique("Skybox", 1);
        tech->getPass(0)->setColorTarget("Albedo", "Final");
        tech->getPass(0)->setDepthTarget("Depth");
        tech = createTechnique("PostDbg", 1);
        tech->getPass(0)->setColorTarget("Albedo", "Final");
        tech->getPass(0)->setDepthTarget("Depth");
        // TODO: Special case, no usual targets since they can't be cubemaps
        tech = createTechnique("ShadowCubeMap", 1);
        tech->getPass(0)->setDepthTarget("Depth");

        compile();

        createUniformBufferDesc(UNIFORM_BUFFER_CAMERA_3D)
            ->define(UNIFORM_PROJECTION, UNIFORM_MAT4)
            .define(UNIFORM_VIEW_TRANSFORM, UNIFORM_MAT4)
            .define("screenSize", UNIFORM_VEC2)
            .compile();
        createUniformBufferDesc("bufShadowmapCamera3d")
            ->define(UNIFORM_PROJECTION, UNIFORM_MAT4)
            .define(UNIFORM_VIEW_TRANSFORM, UNIFORM_MAT4)
            .compile();
        createUniformBufferDesc(UNIFORM_BUFFER_TIME)
            ->define(UNIFORM_TIME, UNIFORM_FLOAT)
            .compile();
        createUniformBufferDesc(UNIFORM_BUFFER_MODEL)
            ->define(UNIFORM_MODEL_TRANSFORM, UNIFORM_MAT4)
            .compile();
        createUniformBufferDesc(UNIFORM_BUFFER_DECAL)
            ->define("boxSize", UNIFORM_VEC3)
            .define("RGBA", UNIFORM_VEC4)
            .compile();

        ubufCamera3d = createUniformBuffer(UNIFORM_BUFFER_CAMERA_3D);
        ubufShadowmapCamera3d = createUniformBuffer("bufShadowmapCamera3d");
        //ubufTime = createUniformBuffer(UNIFORM_BUFFER_TIME);
        //ubufModel = createUniformBuffer(UNIFORM_BUFFER_MODEL);
        //ubufDecal = createUniformBuffer(UNIFORM_BUFFER_DECAL);

        loc_shadowmap_projection = ubufShadowmapCamera3d->getDesc()->getUniform(UNIFORM_PROJECTION);
        loc_shadowmap_view = ubufShadowmapCamera3d->getDesc()->getUniform(UNIFORM_VIEW_TRANSFORM);
        attachUniformBuffer(ubufShadowmapCamera3d);
        
        loc_projection = ubufCamera3d->getDesc()->getUniform(UNIFORM_PROJECTION);
        loc_view = ubufCamera3d->getDesc()->getUniform(UNIFORM_VIEW_TRANSFORM);
        loc_screenSize = ubufCamera3d->getDesc()->getUniform("screenSize");
        attachUniformBuffer(ubufCamera3d);/*
        loc_time = ubufTime->getDesc()->getUniform(UNIFORM_TIME);
        loc_model = ubufModel->getDesc()->getUniform(UNIFORM_MODEL_TRANSFORM);
        loc_boxSize = ubufDecal->getDesc()->getUniform("boxSize");
        loc_color = ubufDecal->getDesc()->getUniform("RGBA");
        loc_screenSize = ubufDecal->getDesc()->getUniform("screenSize");
        */
        setOutputSource("Final");
    }
    ~gpuPipelineDefault() {
        destroyUniformBuffer(ubufShadowmapCamera3d);        
        destroyUniformBuffer(ubufCamera3d);/*
        destroyUniformBuffer(ubufTime);
        destroyUniformBuffer(ubufModel);
        destroyUniformBuffer(ubufDecal);*/
    }

    void setShadowmapCamera(const gfxm::mat4& projection, const gfxm::mat4& view) {
        ubufShadowmapCamera3d->setMat4(loc_shadowmap_projection, projection);
        ubufShadowmapCamera3d->setMat4(loc_shadowmap_view, view);
    }
    void setCamera3d(const gfxm::mat4& projection, const gfxm::mat4& view) {
        ubufCamera3d->setMat4(loc_projection, projection);
        ubufCamera3d->setMat4(loc_view, view);
    }
    void setViewportSize(float width, float height) {
        ubufCamera3d->setVec2(loc_screenSize, gfxm::vec2(width, height));
    }
    void setTime(float t) {
        //ubufTime->setFloat(loc_time, t);
    }
    void setModelTransform(const gfxm::mat4& model) {
        //ubufModel->setMat4(loc_model, model);
    }
    void setDecalData(const gfxm::vec3& boxSize, const gfxm::vec4& color, const gfxm::vec2& vp_sz) {
        //ubufDecal->setVec3(loc_boxSize, boxSize);
        //ubufDecal->setVec4(loc_color, color);
        //ubufDecal->setVec2(loc_screenSize, vp_sz);
    }
};