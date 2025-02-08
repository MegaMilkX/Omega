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
#include "gpu/pass/blur_pass.hpp"


class gpuPipelineDefault : public gpuPipeline {
    gpuUniformBuffer* ubufCommon = 0;
    gpuUniformBuffer* ubufShadowmapCamera3d = 0;
    gpuUniformBuffer* ubufTime = 0;
    gpuUniformBuffer* ubufModel = 0;
    gpuUniformBuffer* ubufDecal = 0;
    int loc_projection = -1;
    int loc_view = -1;
    int loc_camera_pos = -1;
    int loc_screenSize = -1;
    int loc_vp_rect_ratio = -1;
    int loc_time = -1;
    int loc_shadowmap_projection = -1;
    int loc_shadowmap_view = -1;
    int loc_model = -1;
    int loc_boxSize = -1;
    int loc_color = -1;
public:
    gpuPipelineDefault() {
        addColorChannel("Albedo", GL_RGB);
        addColorChannel("Position", GL_RGB32F);
        addColorChannel("Normal", GL_RGB);
        addColorChannel("Metalness", GL_RED);
        addColorChannel("Roughness", GL_RED);
        addColorChannel("AmbientOcclusion", GL_RED);
        addColorChannel("Emission", GL_RGB);
        addColorChannel("Lightness", GL_RGB32F);
        addColorChannel("ObjectOutlineA", GL_RGBA32F);
        addColorChannel("ObjectOutlineB", GL_RGBA32F);
        addColorChannel("Final", GL_RGB32F);
        addDepthChannel("Depth");
        setOutputChannel("Final");

        createUniformBufferDesc(UNIFORM_BUFFER_COMMON)
            ->define(UNIFORM_PROJECTION, UNIFORM_MAT4)
            .define(UNIFORM_VIEW_TRANSFORM, UNIFORM_MAT4)
            .define("cameraPosition", UNIFORM_VEC3)
            .define("time", UNIFORM_FLOAT)
            .define("viewportSize", UNIFORM_VEC2)
            .define("zNear", UNIFORM_FLOAT)
            .define("zFar", UNIFORM_FLOAT)
            .define("vp_rect_ratio", UNIFORM_VEC4)
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
        loc_vp_rect_ratio = ubufCommon->getDesc()->getUniform("vp_rect_ratio");
        loc_time = ubufCommon->getDesc()->getUniform("time");
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

        // TODO: This should all be a single technique
        tech = createTechnique("Outline");
        addPass(tech, new gpuGeometryPass)
            ->setColorTarget("Albedo", "ObjectOutlineA");
        addPass(tech, new gpuBlurPass("ObjectOutlineA", "ObjectOutlineB"));
        tech = createTechnique("OutlineCutout");
        addPass(tech, new gpuGeometryPass)
            ->setColorTarget("Albedo", "ObjectOutlineB");
        // TODO: Separate pass to overlay the outline on the final image
        // -------------------------------------------

        tech = createTechnique("PBRCompose");
        addPass(tech, new gpuDeferredComposePass);
        
        tech = createTechnique("Decals");
        addPass(tech, new gpuGeometryPass)
            ->addColorSource("Normal", "Normal")
            ->addColorSource("Depth", "Depth")
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

        tech = createTechnique("Overlay");
        addPass(tech, new gpuGeometryPass)
            ->setColorTarget("Color", "Final")
            ->setDepthTarget("Depth");

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
            ->setDepthTarget("Depth");

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
    void setViewportRectRatio(const gfxm::vec4& rc) {
        ubufCommon->setVec4(loc_vp_rect_ratio, rc);
    }
    void setTime(float t) {
        ubufCommon->setFloat(loc_time, t);
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