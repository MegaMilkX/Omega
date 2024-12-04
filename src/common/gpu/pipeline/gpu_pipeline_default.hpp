#pragma once


#include "gpu/gpu_pipeline.hpp"
#include "gpu/render/uniform.hpp"
#include "platform/platform.hpp"
#include "gpu/pass/gpu_pass.hpp"
#include "gpu/pass/gpu_deferred_geometry_pass.hpp"
#include "gpu/pass/wireframe_pass.hpp"
#include "gpu/pass/environment_ibl_pass.hpp"
#include "gpu/pass/gpu_deferred_light_pass.hpp"
#include "gpu/pass/gpu_deferred_compose_pass.hpp"
#include "gpu/pass/gpu_skybox_pass.hpp"


class gpuPipelineDefault : public gpuPipeline {
    gpuUniformBuffer* ubufCommon = 0;
    gpuUniformBuffer* ubufShadowmapCamera3d = 0;
    gpuUniformBuffer* ubufTime = 0;
    gpuUniformBuffer* ubufModel = 0;
    gpuUniformBuffer* ubufDecal = 0;
    int loc_projection;
    int loc_view;
    int loc_camera_pos;
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
        addColorRenderTarget("Normal", GL_RGB);
        addColorRenderTarget("Metalness", GL_RED);
        addColorRenderTarget("Roughness", GL_RED);
        addColorRenderTarget("AmbientOcclusion", GL_RED);
        addColorRenderTarget("Emission", GL_RGB);
        addColorRenderTarget("Lightness", GL_RGB32F);
        addColorRenderTarget("Final", GL_RGB32F);
        addDepthRenderTarget("Depth");
        setOutputSource("Final");

        createUniformBufferDesc(UNIFORM_BUFFER_COMMON)
            ->define(UNIFORM_PROJECTION, UNIFORM_MAT4)
            .define(UNIFORM_VIEW_TRANSFORM, UNIFORM_MAT4)
            .define("cameraPosition", UNIFORM_VEC3)
            .define("time", UNIFORM_FLOAT)
            .define("viewportSize", UNIFORM_VEC2)
            .define("zNear", UNIFORM_FLOAT)
            .define("zFar", UNIFORM_FLOAT)
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

        ubufCommon = createUniformBuffer(UNIFORM_BUFFER_COMMON);
        ubufShadowmapCamera3d = createUniformBuffer("bufShadowmapCamera3d");
        //ubufTime = createUniformBuffer(UNIFORM_BUFFER_TIME);
        //ubufModel = createUniformBuffer(UNIFORM_BUFFER_MODEL);
        //ubufDecal = createUniformBuffer(UNIFORM_BUFFER_DECAL);

        loc_shadowmap_projection = ubufShadowmapCamera3d->getDesc()->getUniform(UNIFORM_PROJECTION);
        loc_shadowmap_view = ubufShadowmapCamera3d->getDesc()->getUniform(UNIFORM_VIEW_TRANSFORM);
        attachUniformBuffer(ubufShadowmapCamera3d);
        
        loc_projection = ubufCommon->getDesc()->getUniform(UNIFORM_PROJECTION);
        loc_view = ubufCommon->getDesc()->getUniform(UNIFORM_VIEW_TRANSFORM);
        loc_camera_pos = ubufCommon->getDesc()->getUniform("cameraPosition");
        loc_screenSize = ubufCommon->getDesc()->getUniform("viewportSize");
        attachUniformBuffer(ubufCommon);/*
        loc_time = ubufTime->getDesc()->getUniform(UNIFORM_TIME);
        loc_model = ubufModel->getDesc()->getUniform(UNIFORM_MODEL_TRANSFORM);
        loc_boxSize = ubufDecal->getDesc()->getUniform("boxSize");
        loc_color = ubufDecal->getDesc()->getUniform("RGBA");
        loc_screenSize = ubufDecal->getDesc()->getUniform("screenSize");
        */
    }
    ~gpuPipelineDefault() {
        destroyUniformBuffer(ubufShadowmapCamera3d);        
        destroyUniformBuffer(ubufCommon);/*
        destroyUniformBuffer(ubufTime);
        destroyUniformBuffer(ubufModel);
        destroyUniformBuffer(ubufDecal);*/
    }

    void init() override {
        auto tech = createTechnique("Normal");
        addPass(tech, new gpuDeferredGeometryPass);/*
        tech = createTechnique("GUI");
        addPass(tech, new gpuPass)
            ->setColorTarget("Albedo", "Final")
            ->setDepthTarget("Depth");
        tech = createTechnique("Debug");
        addPass(tech, new gpuPass)
            ->setColorTarget("Albedo", "Albedo")
            ->setDepthTarget("Depth");*/

        tech = createTechnique("EnvironmentIBL");
        addPass(tech, new EnvironmentIBLPass);

        tech = createTechnique("LightPass");
        addPass(tech, new gpuDeferredLightPass);
        
        tech = createTechnique("PBRCompose");
        addPass(tech, new gpuDeferredComposePass);
        
        tech = createTechnique("Decals");
        addPass(tech, new gpuGeometryPass)
            ->setTargetSampler("Normal")
            ->setTargetSampler("Depth")
            ->setColorTarget("Albedo", "Final");
        
        tech = createTechnique("Skybox");
        addPass(tech, new gpuSkyboxPass);
        
        tech = createTechnique("VFX");
        addPass(tech, new gpuGeometryPass)
            ->setColorTarget("Albedo", "Final")
            ->setDepthTarget("Depth");/*
        tech = createTechnique("PostDbg");
        addPass(tech, new gpuPass)
            ->setColorTarget("Albedo", "Final")
            ->setDepthTarget("Depth");*/

        tech = createTechnique("Wireframe");
        addPass(tech, new gpuWireframePass)
            ->setColorTarget("Albedo", "Final")
            ->setDepthTarget("Depth");
        
        // TODO: Special case, no color targets since they can't be cubemaps
        tech = createTechnique("ShadowCubeMap", true);
        addPass(tech, new gpuGeometryPass)
            ->setDepthTarget("Depth");

        tech = createTechnique("LightmapSample", true);
        addPass(tech, new gpuGeometryPass)
            ->setDepthTarget("Depth");;

        compile();
    }

    void setShadowmapCamera(const gfxm::mat4& projection, const gfxm::mat4& view) {
        ubufShadowmapCamera3d->setMat4(loc_shadowmap_projection, projection);
        ubufShadowmapCamera3d->setMat4(loc_shadowmap_view, view);
    }
    void setCamera3d(const gfxm::mat4& projection, const gfxm::mat4& view) {
        ubufCommon->setMat4(loc_projection, projection);
        ubufCommon->setMat4(loc_view, view);
        ubufCommon->setVec3(loc_camera_pos, gfxm::inverse(view)[3]);
    }
    void setViewportSize(float width, float height) {
        ubufCommon->setVec2(loc_screenSize, gfxm::vec2(width, height));
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