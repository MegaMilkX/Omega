#include "gpu_pipeline_default.hpp"



gpuPipelineDefault::gpuPipelineDefault() {
    addColorChannel("Albedo", GL_RGB32F);
    addColorChannel("Position", GL_RGB32F);
    addColorChannel("Normal", GL_RGB);
    addColorChannel("Metalness", GL_RED);
    addColorChannel("Roughness", GL_RED);
    addColorChannel("AmbientOcclusion", GL_RED);
    addColorChannel("Emission", GL_RGB);
    addColorChannel("Lightness", GL_RGB32F);
    addColorChannel("ObjectOutline", GL_RGBA32F, true);
    addColorChannel("DOFMask", GL_RGB, true);
    addColorChannel("Final", GL_RGB32F, true);
    addDepthChannel("Depth");
    addDepthChannel("DepthOverlay");
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
        .define("gamma", UNIFORM_FLOAT)
        .define("exposure", UNIFORM_FLOAT)
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
    loc_zNear = ubufCommon->getDesc()->getUniform("zNear");
    loc_zFar = ubufCommon->getDesc()->getUniform("zFar");
    loc_vp_rect_ratio = ubufCommon->getDesc()->getUniform("vp_rect_ratio");
    loc_time = ubufCommon->getDesc()->getUniform("time");
    loc_gamma = ubufCommon->getDesc()->getUniform("gamma");
    loc_exposure = ubufCommon->getDesc()->getUniform("exposure");
    attachUniformBuffer(ubufCommon);/*
    loc_time = ubufTime->getDesc()->getUniform(UNIFORM_TIME);
    loc_model = ubufModel->getDesc()->getUniform(UNIFORM_MODEL_TRANSFORM);
    loc_boxSize = ubufDecal->getDesc()->getUniform("boxSize");
    loc_color = ubufDecal->getDesc()->getUniform("RGBA");
    loc_screenSize = ubufDecal->getDesc()->getUniform("screenSize");
    */
}

gpuPipelineDefault::~gpuPipelineDefault() {
    destroyUniformBuffer(ubufShadowmapCamera3d);        
    destroyUniformBuffer(ubufCommon);/*
    destroyUniformBuffer(ubufTime);
    destroyUniformBuffer(ubufModel);
    destroyUniformBuffer(ubufDecal);*/
}

void gpuPipelineDefault::init() {
    constexpr float inf = std::numeric_limits<float>::infinity();

    addPass("Color/Zero", new gpuClearPass(gfxm::vec4(0, 0, 0, 0)))
        ->setColorTarget("Lightness", "Lightness")
        ->setColorTarget("ObjectOutline", "ObjectOutline");
    addPass("Color/Inf", new gpuClearPass(gfxm::vec4(inf, inf, inf, inf)))
        ->setColorTarget("Position", "Position")
        ->setDepthTarget("Depth");
    addPass("DepthClear", new gpuClearPass(gfxm::vec4(0,0,0,0)))
        ->setDepthTarget("DepthOverlay");

    addPass("Normal", new gpuDeferredGeometryPass);

    addPass("SSAO", new gpuSSAOPass("Position", "Normal", "AmbientOcclusion"));

    addPass("EnvironmentIBL", new EnvironmentIBLPass);

    addPass("LightPass", new gpuDeferredLightPass);

    addPass("PBRCompose", new gpuDeferredComposePass);

    addPass("Decals", new gpuGeometryPass)
        ->addColorSource("Normal", "Normal")
        ->addColorSource("Depth", "Depth")
        ->setColorTarget("Albedo", "Final");

    addPass("Fog", new gpuFogPass("Final"));
    addPass("Skybox", new gpuSkyboxPass);

    /*
    addPass("PostDbg", new gpuPass)
    ->setColorTarget("Albedo", "Final")
    ->setDepthTarget("Depth");*/
    addPass("Overlay", new gpuGeometryPass)
        ->addColorSource("Depth", "Depth")            
        ->setColorTarget("Color", "Final")
        ->setDepthTarget("DepthOverlay");
    addPass("Wireframe", new gpuWireframePass)
        ->setColorTarget("Albedo", "Final")
        ->setDepthTarget("Depth");

    addPass("Outline/Color", new gpuGeometryPass)
        ->setColorTarget("Albedo", "ObjectOutline");
    addPass("Outline/Blur", new gpuBlurPass("ObjectOutline", "ObjectOutline"));
    addPass("Outline/Cutout", new gpuGeometryPass)
        ->setColorTarget("Albedo", "ObjectOutline");
    // -------------------------------------------

    addPass("Posteffects/DOF/Mask", new gpuTestPosteffectPass("Depth", "DOFMask", "core/shaders/post/dof_mask.glsl"));
    addPass("Posteffects/DOF/MaskMaxFilter", new gpuTestPosteffectPass("DOFMask", "DOFMask", "core/shaders/post/dof_max_filter.glsl"));
    /*addPass("Posteffects/DOF/DOF", new gpuTestPosteffectPass("Final", "Final", "core/shaders/post/dof_simple.glsl"))
        ->addColorSource("Depth", "Depth");*/
    addPass("Posteffects/DOF/DOF", new gpuTestPosteffectPass("Final", "Final", "core/shaders/post/dof.glsl"))
        ->addColorSource("DOFMask", "DOFMask");
    //addPass("Posteffects/Test0", new gpuTestPosteffectPass("Final", "Final", "core/shaders/test/test_posteffect.glsl"));
    //addPass("Posteffects/Test1", new gpuTestPosteffectPass("Final", "Final", "core/shaders/test/test_posteffect2.glsl"));
    //addPass("Posteffects/Test2", new gpuTestPosteffectPass("Final", "Final", "core/shaders/test/test_posteffect3.glsl"));
    addPass("Posteffects/GammaTonemap", new gpuTestPosteffectPass("Final", "Final", "core/shaders/post/gamma_tonemap.glsl"));
    addPass("VFX", new gpuGeometryPass)
        ->setColorTarget("Albedo", "Final")
        ->setDepthTarget("Depth");
    addPass("Posteffects/ChromaticAberration", new gpuTestPosteffectPass("Final", "Final", "core/shaders/post/chromatic_aberration.glsl"));
    addPass("Posteffects/Outline", new gpuBlitPass("ObjectOutline", "Final"));
    addPass("Posteffects/Lens", new gpuTestPosteffectPass("Final", "Final", "core/shaders/post/lens.glsl"));

    // TODO: Special case, no color targets since they can't be cubemaps
    addPass("ShadowCubeMap", new gpuGeometryPass)
        ->setDepthTarget("Depth")
        ->addFlags(PASS_FLAG_NO_DRAW);

    addPass("LightmapSample", new gpuGeometryPass)
        ->setDepthTarget("Depth")
        ->addFlags(PASS_FLAG_NO_DRAW);

    setGamma(2.2f);
    setExposure(.1f);

    enableTechnique("EnvironmentIBL", true);
    enableTechnique("Fog", false);
    enableTechnique("Posteffects/DOF", true);

    compile();
}

void gpuPipelineDefault::setShadowmapCamera(const gfxm::mat4& projection, const gfxm::mat4& view) {
    ubufShadowmapCamera3d->setMat4(loc_shadowmap_projection, projection);
    ubufShadowmapCamera3d->setMat4(loc_shadowmap_view, view);
}

void gpuPipelineDefault::setCamera3d(const gfxm::mat4& projection, const gfxm::mat4& view) {
    ubufCommon->setMat4(loc_projection, projection);
    ubufCommon->setMat4(loc_view, view);
    ubufCommon->setVec3(loc_camera_pos, gfxm::inverse(view)[3]);
    float a = projection[2][2];
    float b = projection[3][2];
    float znear = b / (a - 1.f);
    float zfar = b / (a + 1.f);
    ubufCommon->setFloat(loc_zNear, znear);
    ubufCommon->setFloat(loc_zFar, zfar);
}

void gpuPipelineDefault::setViewportSize(float width, float height) {
    ubufCommon->setVec2(loc_screenSize, gfxm::vec2(width, height));
}

void gpuPipelineDefault::setViewportRectRatio(const gfxm::vec4& rc) {
    ubufCommon->setVec4(loc_vp_rect_ratio, rc);
}

void gpuPipelineDefault::setTime(float t) {
    ubufCommon->setFloat(loc_time, t);
}

void gpuPipelineDefault::setGamma(float gamma) {
    ubufCommon->setFloat(loc_gamma, gamma);
}

void gpuPipelineDefault::setExposure(float exposure) {
    ubufCommon->setFloat(loc_exposure, exposure);
}

void gpuPipelineDefault::setModelTransform(const gfxm::mat4& model) {
    //ubufModel->setMat4(loc_model, model);
}

void gpuPipelineDefault::setDecalData(const gfxm::vec3& boxSize, const gfxm::vec4& color, const gfxm::vec2& vp_sz) {
    //ubufDecal->setVec3(loc_boxSize, boxSize);
    //ubufDecal->setVec4(loc_color, color);
    //ubufDecal->setVec2(loc_screenSize, vp_sz);
}

