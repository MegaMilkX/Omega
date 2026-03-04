#include "gpu_pipeline_default.hpp"

#include "resource_manager/resource_manager.hpp"

#include "gpu/param_block/transform_block_mgr.hpp"
#include "gpu/param_block/decal_block_mgr.hpp"
#include "gpu/param_block/common_block_mgr.hpp"


gpuPipelineDefault::gpuPipelineDefault() {
    addColorChannel("Albedo", GL_RGB32F);
    addColorChannel("Position", GL_RGB32F);
    addColorChannel("Normal", GL_RGBA); // NOTE: Alpha for lighting mask
    addColorChannel("ORMM", GL_RGBA); // Occlusion, Roughness, Metallness, LightMask
    addColorChannel("Metalness", GL_RED);
    addColorChannel("Roughness", GL_RED);
    addColorChannel("AmbientOcclusion", GL_RED, true);
    addColorChannel("Lightness", GL_RGB16F);
    addColorChannel("ObjectOutline", GL_RGBA16F, true, GPU_TEXTURE_WRAP_CLAMP);
    addColorChannel("DOFMask", GL_RGB, true);
    addColorChannel("VelocityMap", GL_RGB16F);
    addColorChannel("Final", GL_RGB32F, true, GPU_TEXTURE_WRAP_CLAMP);
    //addColorChannel("FinalSmall", GL_RGB32F, false, GPU_TEXTURE_WRAP_CLAMP, 1024, 1024);
    addDepthChannel("Depth");
    addDepthChannel("DepthViewModel");
    addDepthChannel("DepthOverlay");
    setOutputChannel("Final");

    createUniformBufferDesc(UNIFORM_BUFFER_COMMON)
        ->define(UNIFORM_PROJECTION, UNIFORM_MAT4)
        .define(UNIFORM_VIEW_TRANSFORM, UNIFORM_MAT4)
        .define(UNIFORM_VIEW_TRANSFORM_PREV, UNIFORM_MAT4)
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
    createUniformBufferDesc(UNIFORM_BUFFER_MODEL)
        ->define(UNIFORM_MODEL_TRANSFORM, UNIFORM_MAT4)
        .define(UNIFORM_MODEL_TRANSFORM_PREV, UNIFORM_MAT4)
        .compile();
    createUniformBufferDesc(UNIFORM_BUFFER_DECAL)
        ->define("boxSize", UNIFORM_VEC3)
        .define("RGBA", UNIFORM_VEC4)
        .compile();

    ubufShadowmapCamera3d = createUniformBuffer("bufShadowmapCamera3d");
    //ubufTime = createUniformBuffer(UNIFORM_BUFFER_TIME);
    //ubufModel = createUniformBuffer(UNIFORM_BUFFER_MODEL);
    //ubufDecal = createUniformBuffer(UNIFORM_BUFFER_DECAL);

    loc_shadowmap_projection = ubufShadowmapCamera3d->getDesc()->getUniform(UNIFORM_PROJECTION);
    loc_shadowmap_view = ubufShadowmapCamera3d->getDesc()->getUniform(UNIFORM_VIEW_TRANSFORM);
    attachUniformBuffer(ubufShadowmapCamera3d);
}

gpuPipelineDefault::~gpuPipelineDefault() {
    destroyUniformBuffer(ubufShadowmapCamera3d);
}

void gpuPipelineDefault::init() {
    constexpr float inf = std::numeric_limits<float>::infinity();

    addPass("Clear/Zero", new gpuClearPass(gfxm::vec4(0, 0, 0, 0)))
        ->setColorTarget("Normal", "Normal")
        ->setColorTarget("Final", "Final")
        ->setColorTarget("Lightness", "Lightness")
        ->setColorTarget("ObjectOutline", "ObjectOutline")
        ->setColorTarget("VelocityMap", "VelocityMap");
    addPass("Clear/Inf", new gpuClearPass(gfxm::vec4(inf, inf, inf, inf)))
        ->setColorTarget("Position", "Position")
        ->setDepthTarget("Depth");
    addPass("Clear/Depth", new gpuClearPass(gfxm::vec4(0,0,0,0)))
        ->setDepthTarget("DepthOverlay");
    addPass("Clear/DepthViewModel", new gpuClearPass(gfxm::vec4(inf, inf, inf, inf)))
        ->setDepthTarget("DepthViewModel");

    addPass("Default", new gpuDeferredGeometryPass)
        ->setColorTarget("Albedo", "Albedo")
        ->setColorTarget("Position", "Position")
        ->setColorTarget("Normal", "Normal")
        ->setColorTarget("Metalness", "Metalness")
        ->setColorTarget("Roughness", "Roughness")
        ->setColorTarget("Lightness", "Lightness")
        ->setColorTarget("AmbientOcclusion", "AmbientOcclusion")
        ->setColorTarget("VelocityMap", "VelocityMap")
        ->setDepthTarget("Depth");

    addPass("ViewModel/Default", new gpuDeferredGeometryPass)
        ->setColorTarget("Albedo", "Albedo")
        ->setColorTarget("Position", "Position")
        ->setColorTarget("Normal", "Normal")
        ->setColorTarget("Metalness", "Metalness")
        ->setColorTarget("Roughness", "Roughness")
        ->setColorTarget("Lightness", "Lightness")
        ->setColorTarget("AmbientOcclusion", "AmbientOcclusion")
        ->setColorTarget("VelocityMap", "VelocityMap")
        ->setDepthTarget("DepthViewModel");
    addPass("ViewModel/BlitDepth", new gpuDepthMergePass("DepthViewModel", "Depth"))
        ->setBlending(GPU_BLEND_MODE::OVERWRITE);

    addPass("SSAO/AO", new gpuSSAOPass("Position", "Normal", "AmbientOcclusion"));
    addPass("SSAO/Blur", new gpuTestPosteffectPass("AmbientOcclusion", "AmbientOcclusion", "core/shaders/post/ssao_blur.glsl"));

    addPass("EnvironmentIBL", new EnvironmentIBLPass);

    addPass("LightPass", new gpuDeferredLightPass);

    addPass("VelocityMapTest", new gpuVelocityMapPass("VelocityMap"));

    addPass("PBRCompose", new gpuDeferredComposePass);

    addPass("Decals", new gpuGeometryPass)
        ->addColorSource("Normal", "Normal")
        ->addColorSource("Depth", "Depth")
        ->setColorTarget("Albedo", "Final");

    addPass("Fog", new gpuFogPass("Final"));

    //addPass("PreSSRCopy", new gpuBlitPass("Final", "FinalSmall"));
    //addPass("EnvironmentSSR", new EnvironmentSSRPass);

    addPass("Posteffects/MotionBlur", new gpuTestPosteffectPass("Final", "Final", "core/shaders/post/motion_blur.glsl"))
        ->addColorSource("VelocityMap", "VelocityMap");

    addPass("Skybox", new gpuSkyboxPass);

    addPass("HL2/PreWaterBlit", new gpuBlitPass("Final", "Final"));
    addPass("HL2/Water", new gpuTranslucentPass)
        ->addColorSource("Depth", "Depth")
        ->addColorSource("Color", "Final")
        ->setColorTarget("Albedo", "Final");

    addPass("HL2/Translucent", new gpuTranslucentPass)
        ->setDepthTarget("Depth")
        ->setColorTarget("Albedo", "Final");
    /*
    addPass("PostDbg", new gpuPass)
    ->setColorTarget("Albedo", "Final")
    ->setDepthTarget("Depth");*/

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
        ->addColorSource("Depth", "Depth")
        ->setColorTarget("Albedo", "Final");
    addPass("Posteffects/ChromaticAberration", new gpuTestPosteffectPass("Final", "Final", "core/shaders/post/chromatic_aberration.glsl"));
    addPass("Outline/Blit", new gpuBlitPass("ObjectOutline", "Final"))
        ->setBlending(GPU_BLEND_MODE::ADD);

    addPass("PreOverlayBlit", new gpuBlitPass("Final", "Final"));
    addPass("Overlay", new gpuGeometryPass)
        ->addColorSource("Depth", "Depth")
        ->addColorSource("Color", "Final")
        ->setColorTarget("Color", "Final")
        ->setDepthTarget("DepthOverlay");
    addPass("Wireframe", new gpuWireframePass)
        ->setColorTarget("Albedo", "Final")
        ->setDepthTarget("Depth");

    addPass("Posteffects/Lens", new gpuTestPosteffectPass("Final", "Final", "core/shaders/post/lens.glsl"));

    // TODO: Special case, no color targets since they can't be cubemaps
    addPass("ShadowCubeMap", new gpuGeometryPass)
        ->setDepthTarget("Depth")
        ->addFlags(PASS_FLAG_NO_DRAW);

    addPass("LightmapSample", new gpuGeometryPass)
        ->setDepthTarget("Depth")
        ->addFlags(PASS_FLAG_NO_DRAW);

    enableTechnique("SSAO", true);
    enableTechnique("EnvironmentIBL", true);
    enableTechnique("Skybox", true);
    enableTechnique("Fog", true);
    enableTechnique("Posteffects/DOF", false);
    enableTechnique("Posteffects/ChromaticAberration", true);
    enableTechnique("Posteffects/Lens", false);
    enableTechnique("Posteffects/GammaTonemap", true);
    enableTechnique("Posteffects/MotionBlur", true);

    getParamBlockContext()
        ->registerParamBlock(
            getUniformBufferDesc(UNIFORM_BUFFER_COMMON),
            new gpuCommonBlockManager
        )
        ->registerParamBlock(
            getUniformBufferDesc(UNIFORM_BUFFER_MODEL),
            new gpuTransformBlockManager
        )
        ->registerParamBlock(
            getUniformBufferDesc(UNIFORM_BUFFER_DECAL),
            new gpuDecalBlockManager
        );

    common_block = getParamBlockContext()->createParamBlock<gpuCommonBlock>();
    attachParamBlock(common_block);

    compile();

    setGamma(2.2f);
    setExposure(.1f);
}

void gpuResolveMaterialParams(GPU_INTERMEDIATE_PASS_DESC* pass, const gpuMaterial* mat, GPU_BLEND_MODE in_blending, draw_flags_t in_draw_flags) {
    GPU_BLEND_MODE blending = in_blending;
    draw_flags_t draw_flags = in_draw_flags;

    if(mat) {
        if (mat->getBlendingMode().has_value()) {
            blending = mat->getBlendingMode().value();
        }
        if (mat->getDepthTest().has_value()) {
            draw_flags &= ~GPU_DEPTH_TEST;
            draw_flags |= mat->getDepthTest().value() ? GPU_DEPTH_TEST : 0;
        }
        if (mat->getDepthWrite().has_value()) {
            draw_flags &= ~GPU_DEPTH_WRITE;
            draw_flags |= mat->getDepthWrite().value() ? GPU_DEPTH_WRITE : 0;
        }
        if (mat->getStencilTest().has_value()) {
            draw_flags &= ~GPU_STENCIL_TEST;
            draw_flags |= mat->getStencilTest().value() ? GPU_STENCIL_TEST : 0;
        }
        if (mat->getBackfaceCulling().has_value()) {
            draw_flags &= ~GPU_BACKFACE_CULLING;
            draw_flags |= mat->getBackfaceCulling().value() ? GPU_BACKFACE_CULLING : 0;
        }
    }

    pass->blend_mode = blending;
    pass->draw_flags = draw_flags;
}

void gpuPipelineDefault::resolveRenderableRole(GPU_Role t, GPU_INTERMEDIATE_RENDERABLE_CONTEXT& ctx, const gpuMaterial* mat) {
    switch (t) {
    case GPU_Role_None: return;
    case GPU_Role_Geometry: {
        GPU_INTERMEDIATE_PASS_DESC* int_pass = nullptr;
        bool is_transparent = (mat && mat->getTransparent().has_value()) ? mat->getTransparent().value() : false;

        if (is_transparent) {
            int_pass = ctx.getOrCreatePass(getPassId("HL2/Translucent"));
            int_pass->addBaseShaderSet(loadResource<gpuShaderSet>("core/shaders/modular/geo.main.vert").get());
            int_pass->addBaseShaderSet(loadResource<gpuShaderSet>("core/shaders/modular/geo.main.frag").get());
        } else {
            int_pass = ctx.getOrCreatePass(getPassId("Default"));
            int_pass->addBaseShaderSet(loadResource<gpuShaderSet>("core/shaders/modular/geo.main.vert").get());
            int_pass->addBaseShaderSet(loadResource<gpuShaderSet>("core/shaders/modular/geo.main.frag").get());
        }

        if(mat) {
            if (mat->hasVertexExtensionSet()) {
                int_pass->addExtensionShaderSet(mat->getVertexExtensionSet());
            }
            if (mat->hasFragmentExtensionSet()) {
                int_pass->addExtensionShaderSet(mat->getFragmentExtensionSet());
            }
            gpuResolveMaterialParams(
                int_pass, mat,
                GPU_BLEND_MODE::BLEND,
                GPU_DEPTH_TEST | GPU_DEPTH_WRITE | GPU_BACKFACE_CULLING
            );
        }

        GPU_INTERMEDIATE_PASS_DESC* wire_pass = ctx.getOrCreatePass(getPassId("Wireframe"));
        wire_pass->addBaseShaderSet(loadResource<gpuShaderSet>("core/shaders/modular/geo.main.vert").get());
        wire_pass->addBaseShaderSet(loadResource<gpuShaderSet>("core/shaders/modular/wireframe.main.frag").get());
        wire_pass->blend_mode = GPU_BLEND_MODE::BLEND;
        wire_pass->draw_flags = GPU_DEPTH_WRITE | GPU_DEPTH_TEST;
        if(mat) {
            if (mat->hasVertexExtensionSet()) {
                wire_pass->addExtensionShaderSet(mat->getVertexExtensionSet());
            }
        }

        break;
    }
    case GPU_Role_Decal: {
        GPU_INTERMEDIATE_PASS_DESC* int_pass = ctx.getOrCreatePass(getPassId("Decals"));
        int_pass->addBaseShaderSet(loadResource<gpuShaderSet>("core/shaders/modular/decal.main.vert").get());
        int_pass->addBaseShaderSet(loadResource<gpuShaderSet>("core/shaders/modular/decal.main.frag").get());
        if(mat) {
            if (mat->hasFragmentExtensionSet()) {
                int_pass->addExtensionShaderSet(mat->getFragmentExtensionSet());
            }
        }
        gpuResolveMaterialParams(
            int_pass, mat,
            GPU_BLEND_MODE::ADD,
            GPU_BACKFACE_CULLING
        );

        GPU_INTERMEDIATE_PASS_DESC* wire_pass = ctx.getOrCreatePass(getPassId("Wireframe"));
        wire_pass->addBaseShaderSet(loadResource<gpuShaderSet>("core/shaders/modular/decal.main.vert").get());
        wire_pass->addBaseShaderSet(loadResource<gpuShaderSet>("core/shaders/modular/wireframe.main.frag").get());
        wire_pass->blend_mode = GPU_BLEND_MODE::BLEND;
        wire_pass->draw_flags = GPU_DEPTH_WRITE | GPU_DEPTH_TEST;
        break;
    }
    case GPU_Role_Water: {
        GPU_INTERMEDIATE_PASS_DESC* int_pass = nullptr;
        
        int_pass = ctx.getOrCreatePass(getPassId("HL2/Water"));
        int_pass->addBaseShaderSet(loadResource<gpuShaderSet>("shaders/hl2/water").get());

        if(mat) {
            if (mat->hasVertexExtensionSet()) {
                int_pass->addExtensionShaderSet(mat->getVertexExtensionSet());
            }
            if (mat->hasFragmentExtensionSet()) {
                int_pass->addExtensionShaderSet(mat->getFragmentExtensionSet());
            }
            gpuResolveMaterialParams(
                int_pass, mat,
                GPU_BLEND_MODE::BLEND,
                GPU_DEPTH_TEST | GPU_DEPTH_WRITE | GPU_BACKFACE_CULLING
            );
        }

        GPU_INTERMEDIATE_PASS_DESC* wire_pass = ctx.getOrCreatePass(getPassId("Wireframe"));
        wire_pass->addBaseShaderSet(loadResource<gpuShaderSet>("core/shaders/modular/geo.main.vert").get());
        wire_pass->addBaseShaderSet(loadResource<gpuShaderSet>("core/shaders/modular/wireframe.main.frag").get());
        wire_pass->blend_mode = GPU_BLEND_MODE::BLEND;
        wire_pass->draw_flags = GPU_DEPTH_WRITE | GPU_DEPTH_TEST;
        if(mat) {
            if (mat->hasVertexExtensionSet()) {
                wire_pass->addExtensionShaderSet(mat->getVertexExtensionSet());
            }
        }

        break;
    }
    default:
        assert(false);
    }
}
void gpuPipelineDefault::resolveRenderableEffect(GPU_Effect t, GPU_INTERMEDIATE_RENDERABLE_CONTEXT& ctx, const gpuMaterial* mat) {
    switch (t) {
    case GPU_Effect_Outline: {
        GPU_INTERMEDIATE_PASS_DESC* color_pass = ctx.getOrCreatePass(getPassId("Outline/Color"));
        color_pass->addBaseShaderSet(loadResource<gpuShaderSet>("core/shaders/modular/geo.main.vert").get());
        color_pass->addBaseShaderSet(loadResource<gpuShaderSet>("core/shaders/modular/solid_color.main.frag").get());
        GPU_INTERMEDIATE_PASS_DESC* cutout_pass = ctx.getOrCreatePass(getPassId("Outline/Cutout"));
        cutout_pass->addBaseShaderSet(loadResource<gpuShaderSet>("core/shaders/modular/geo.main.vert").get());
        cutout_pass->addBaseShaderSet(loadResource<gpuShaderSet>("core/shaders/modular/outline_cutout.main.frag").get());
        if(mat && mat->hasVertexExtensionSet()) {
            color_pass->addExtensionShaderSet(mat->getVertexExtensionSet());
            cutout_pass->addExtensionShaderSet(mat->getVertexExtensionSet());
        }
        {
            GPU_BLEND_MODE blending = GPU_BLEND_MODE::BLEND;
            draw_flags_t draw_flags = GPU_DEPTH_TEST | GPU_DEPTH_WRITE | GPU_BACKFACE_CULLING;
            if (mat->getBackfaceCulling().has_value()) {
                draw_flags &= ~GPU_BACKFACE_CULLING;
                draw_flags |= mat->getBackfaceCulling().value() ? GPU_BACKFACE_CULLING : 0;
            }
            color_pass->blend_mode = blending;
            color_pass->draw_flags = draw_flags;
        }
        {
            GPU_BLEND_MODE blending = GPU_BLEND_MODE::MULTIPLY;
            draw_flags_t draw_flags = GPU_DEPTH_TEST | GPU_DEPTH_WRITE | GPU_BACKFACE_CULLING;
            if (mat->getBackfaceCulling().has_value()) {
                draw_flags &= ~GPU_BACKFACE_CULLING;
                draw_flags |= mat->getBackfaceCulling().value() ? GPU_BACKFACE_CULLING : 0;
            }
            cutout_pass->blend_mode = blending;
            cutout_pass->draw_flags = draw_flags;
        }

        break;
    }
    default:
        assert(false);
    }
}

void gpuPipelineDefault::setShadowmapCamera(const gfxm::mat4& projection, const gfxm::mat4& view) {
    ubufShadowmapCamera3d->setMat4(loc_shadowmap_projection, projection);
    ubufShadowmapCamera3d->setMat4(loc_shadowmap_view, view);
}

void gpuPipelineDefault::setCamera3d(const gfxm::mat4& projection, const gfxm::mat4& view) {
    common_block->setProjection(projection);
    common_block->setView(view);
    common_block->setCamPos(gfxm::inverse(view)[3]);
    float a = projection[2][2];
    float b = projection[3][2];
    float znear = b / (a - 1.f);
    float zfar = b / (a + 1.f);
    common_block->setZNear(znear);
    common_block->setZFar(zfar);
}
void gpuPipelineDefault::setCamera3dPrev(const gfxm::mat4& projection, const gfxm::mat4& view) {
    common_block->setViewPrev(view);
}

void gpuPipelineDefault::setViewportSize(float width, float height) {
    width = gfxm::_max(1.f, width);
    height = gfxm::_max(1.f, height);
    common_block->setViewportSize(gfxm::vec2(width, height));
}

void gpuPipelineDefault::setViewportRectRatio(const gfxm::vec4& rc) {
    common_block->setVpRectRatio(rc);
}

void gpuPipelineDefault::setTime(float t) {
    common_block->setTime(t);
}

void gpuPipelineDefault::setGamma(float gamma) {
    common_block->setGamma(gamma);
}

void gpuPipelineDefault::setExposure(float exposure) {
    common_block->setExposure(exposure);
}

