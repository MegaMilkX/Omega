#include "gpu_pipeline_default.hpp"

#include "resource_manager/resource_manager.hpp"

#include "gpu/gpu.hpp"

#include "gpu/pass/gpu_pass.hpp"
#include "gpu/pass/gpu_deferred_geometry_pass.hpp"
#include "gpu/pass/gpu_translucent_pass.hpp"
#include "gpu/pass/wireframe_pass.hpp"
#include "gpu/pass/environment_ibl_pass.hpp"
#include "gpu/pass/environment_ssr_pass.hpp"
#include "gpu/pass/gpu_deferred_light_pass.hpp"
#include "gpu/pass/gpu_deferred_compose_pass.hpp"
#include "gpu/pass/gpu_skybox_pass.hpp"
#include "gpu/pass/blur_pass.hpp"
#include "gpu/pass/test_posteffect_pass.hpp"
#include "gpu/pass/fog_pass.hpp"
#include "gpu/pass/velocity_map_pass.hpp"
#include "gpu/pass//ssao_pass.hpp"
#include "gpu/pass/blit_pass.hpp"
#include "gpu/pass/clear_pass.hpp"

#include "gpu/default_renderer.hpp"

#include "gpu/param_block/transform_block_mgr.hpp"
#include "gpu/param_block/decal_block_mgr.hpp"
#include "gpu/param_block/common_block_mgr.hpp"
#include "gpu/param_block/direct_light_block_mgr.hpp"


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
    addDepthChannel("DepthLayer");
    addDepthChannel("DepthOverlay");
    addDepthChannel("Shadowmap", 4096, 4096, GPU_TEXTURE_WRAP_CLAMP_BORDER);
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
    createUniformBufferDesc(UNIFORM_BUFFER_DIRECT_LIGHT)
        ->define("dirLightProj", UNIFORM_MAT4)
        .define("dirLightView", UNIFORM_MAT4)
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
        ->setColorTarget("Albedo", "Albedo")
        ->setColorTarget("Final", "Final")
        ->setColorTarget("Lightness", "Lightness")
        ->setColorTarget("ObjectOutline", "ObjectOutline")
        ->setColorTarget("VelocityMap", "VelocityMap");
    addPass("Clear/Normal", new gpuClearPass(gfxm::vec4(0, 0, 0, 0)))
        ->setColorTarget("Normal", "Normal");
    addPass("Clear/Inf", new gpuClearPass(gfxm::vec4(inf, inf, inf, inf)))
        ->setColorTarget("Position", "Position");
    addPass("Clear/Depth", new gpuClearPass(gfxm::vec4(inf, inf, inf, inf)))
        ->setDepthTarget("Depth");
    addPass("Clear/DepthOverlay", new gpuClearPass(gfxm::vec4(0,0,0,0)))
        ->setDepthTarget("DepthOverlay");
    addPass("Clear/DepthLayer", new gpuClearPass(gfxm::vec4(inf, inf, inf, inf)))
        ->setDepthTarget("DepthLayer");
    addPass("Clear/Shadowmap", new gpuClearPass(gfxm::vec4(inf, inf, inf, inf)))
        ->setDepthTarget("Shadowmap");

    addPass("Default", new gpuDeferredGeometryPass)
        ->setDepthTarget("Depth");

    // NOTE: Make Normal layer double buffered if you uncomment this
    //addPass("BlurNormals", new gpuBlurPass("Normal", "Normal"));

    addPass("ViewModel/BlitDepth", new gpuDepthMergePass("DepthLayer", "Depth"))
        ->setBlending(GPU_BLEND_MODE::OVERWRITE);

    addPass("SSAO/AO", new gpuSSAOPass("Position", "Normal"));
    addPass("SSAO/Blur", new gpuTestPosteffectPass("AmbientOcclusion", "AmbientOcclusion", "core/shaders/post/ssao_blur"));

    addPass("EnvironmentIBL", new EnvironmentIBLPass);

    addPass("ShadowmapTest", new gpuDirectLightTest())
        ->addColorSource("Shadowmap", "Shadowmap")
        ->addColorSource("WorldPos", "Position")
        ->addColorSource("Normal", "Normal")
        ->setColorTarget("Lightness", "Lightness");

    addPass("LightPass", new gpuDeferredLightPass);

    addPass("VelocityMapTest", new gpuVelocityMapPass("VelocityMap"));

    addPass("PBRCompose", new gpuDeferredComposePass);

    addPass("Decals", new gpuDecalPass)
        ->addColorSource("Normal", "Normal")
        ->addColorSource("Depth", "Depth")
        ->setColorTarget("Albedo", "Final");

    addPass("Fog", new gpuFogPass("Final"));

    //addPass("PreSSRCopy", new gpuBlitPass("Final", "FinalSmall"));
    //addPass("EnvironmentSSR", new EnvironmentSSRPass);

    addPass("Posteffects/MotionBlur", new gpuTestPosteffectPass("Final", "Final", "core/shaders/post/motion_blur"))
        ->addColorSource("VelocityMap", "VelocityMap");

    addPass("Skybox", new gpuSkyboxPass);

    addPass("HL2/PreWaterBlit", new gpuBlitPass("Final", "Final"));
    addPass("HL2/Water", new gpuTranslucentPass)
        ->addColorSource("Depth", "Depth")
        ->addColorSource("Normal", "Normal")
        ->addColorSource("Color", "Final");

    addPass("HL2/Translucent", new gpuTranslucentPass)
        ->setDepthTarget("Depth");
    /*
    addPass("PostDbg", new gpuPass)
    ->setColorTarget("Albedo", "Final")
    ->setDepthTarget("Depth");*/

    addPass("Outline/Color", new gpuOutlineColorPass)
        ->setColorTarget("Albedo", "ObjectOutline");
    addPass("Outline/Blur", new gpuBlurPass("ObjectOutline", "ObjectOutline"));
    addPass("Outline/Cutout", new gpuOutlineCutoutPass)
        ->setColorTarget("Albedo", "ObjectOutline");
    // -------------------------------------------

    addPass("Posteffects/DOF/Mask", new gpuTestPosteffectPass("Depth", "DOFMask", "core/shaders/post/dof_mask"));
    addPass("Posteffects/DOF/MaskMaxFilter", new gpuTestPosteffectPass("DOFMask", "DOFMask", "core/shaders/post/dof_max_filter"));
    /*addPass("Posteffects/DOF/DOF", new gpuTestPosteffectPass("Final", "Final", "core/shaders/post/dof_simple"))
        ->addColorSource("Depth", "Depth");*/
    addPass("Posteffects/DOF/DOF", new gpuTestPosteffectPass("Final", "Final", "core/shaders/post/dof"))
        ->addColorSource("DOFMask", "DOFMask");
    //addPass("Posteffects/Test0", new gpuTestPosteffectPass("Final", "Final", "core/shaders/test/test_posteffect"));
    //addPass("Posteffects/Test1", new gpuTestPosteffectPass("Final", "Final", "core/shaders/test/test_posteffect2"));
    //addPass("Posteffects/Test2", new gpuTestPosteffectPass("Final", "Final", "core/shaders/test/test_posteffect3"));
    addPass("Posteffects/GammaTonemap", new gpuTestPosteffectPass("Final", "Final", "core/shaders/post/gamma_tonemap"));
    addPass("VFX", new gpuGeometryPass)
        ->addColorSource("Depth", "Depth")
        ->setColorTarget("Albedo", "Final");
    addPass("Posteffects/ChromaticAberration", new gpuTestPosteffectPass("Final", "Final", "core/shaders/post/chromatic_aberration"));
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

    addPass("Posteffects/Lens", new gpuTestPosteffectPass("Final", "Final", "core/shaders/post/lens"));

    addPass("Shadowmap", new gpuShadowmapPass())
        ->setDepthTarget("Shadowmap");

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
    enableTechnique("Posteffects/ChromaticAberration", false);
    enableTechnique("Posteffects/Lens", false);
    enableTechnique("Posteffects/GammaTonemap", true);
    enableTechnique("Posteffects/MotionBlur", true);

    gpuGetDevice()->getParamBlockContext()
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
        )
        ->registerParamBlock(
            getUniformBufferDesc(UNIFORM_BUFFER_DIRECT_LIGHT),
            new gpuDirectLightBlockMgr
        );

    common_block = gpuGetDevice()->createParamBlock<gpuCommonBlock>();
    attachParamBlock(common_block);
    dir_light_block = gpuGetDevice()->createParamBlock<gpuDirectLightBlock>();
    attachParamBlock(dir_light_block);

    compile();

    setGamma(2.2f);
    setExposure(.1f);
}

gpuRenderer* gpuPipelineDefault::getRenderer(RendererType rtype) {
    switch (rtype) {
    case RendererType::Default:
        if (!default_renderer) {
            default_renderer.reset(new gpuDefaultRenderer());
        }
        return default_renderer.get();
    }
    assert(false);
    return nullptr;
}

void gpuResolveMaterialParams(GPU_INTERMEDIATE_PASS_DESC* pass, const gpuMaterial* mat, GPU_BLEND_MODE in_blending, draw_flags_t in_draw_flags) {
    GPU_BLEND_MODE blending = in_blending;
    draw_flags_t draw_flags = in_draw_flags;

    if(mat) {
        blending = mat->getBlendingMode();

        draw_flags = mat->getDepthTest() ? (draw_flags | GPU_DEPTH_TEST) : (draw_flags & ~GPU_DEPTH_TEST);
        draw_flags = mat->getDepthWrite() ? (draw_flags | GPU_DEPTH_WRITE) : (draw_flags & ~GPU_DEPTH_WRITE);
        draw_flags = mat->getStencilTest() ? (draw_flags | GPU_STENCIL_TEST) : (draw_flags & ~GPU_STENCIL_TEST);
        draw_flags = mat->getBackfaceCulling() ? (draw_flags | GPU_BACKFACE_CULLING) : (draw_flags & ~GPU_BACKFACE_CULLING);
    }

    pass->blend_mode = blending;
    pass->draw_flags = draw_flags;
}

void gpuPipelineDefault::resolveRenderableRole(GPU_Role t, GPU_INTERMEDIATE_RENDERABLE_CONTEXT& ctx, const gpuMaterial* mat) {
    switch (t) {
    case GPU_Role_None: return;
    case GPU_Role_Geometry: {
        GPU_INTERMEDIATE_PASS_DESC* int_pass = nullptr;
        bool is_transparent = mat ? mat->getTransparent() : false;
        if (is_transparent) {
            int_pass = ctx.getOrCreatePass(getPassId("HL2/Translucent"));
        } else {
            int_pass = ctx.getOrCreatePass(getPassId("Default"));
        }

        GPU_INTERMEDIATE_PASS_DESC* shadow_pass = nullptr;
        shadow_pass = ctx.getOrCreatePass(getPassId("Shadowmap"));

        if(mat) {
            if (mat->hasVertexShaders()) {
                int_pass->addExtensionShaderSet(mat->getVertexShaders());
                int_pass->extended_by_material |= 1 << SHADER_VERTEX;
                if (shadow_pass) {
                    shadow_pass->addExtensionShaderSet(mat->getVertexShaders());
                    shadow_pass->extended_by_material |= 1 << SHADER_VERTEX;
                }
            }
            if (mat->hasFragmentShaders()) {
                int_pass->addExtensionShaderSet(mat->getFragmentShaders());
                int_pass->extended_by_material |= 1 << SHADER_FRAGMENT;
                if (shadow_pass) {
                    shadow_pass->addExtensionShaderSet(mat->getFragmentShaders());
                    shadow_pass->extended_by_material |= 1 << SHADER_FRAGMENT;
                }
            }
            gpuResolveMaterialParams(
                int_pass, mat,
                GPU_BLEND_MODE::BLEND,
                GPU_DEPTH_TEST | GPU_DEPTH_WRITE | GPU_BACKFACE_CULLING
            );
            // TODO: Figure out why omitting this call breaks shadowmap drawing completely
            gpuResolveMaterialParams(
                shadow_pass, mat,
                GPU_BLEND_MODE::BLEND,
                GPU_DEPTH_TEST | GPU_DEPTH_WRITE | GPU_BACKFACE_CULLING
            );
        }

        GPU_INTERMEDIATE_PASS_DESC* wire_pass = ctx.getOrCreatePass(getPassId("Wireframe"));
        wire_pass->blend_mode = GPU_BLEND_MODE::BLEND;
        wire_pass->draw_flags = GPU_DEPTH_WRITE | GPU_DEPTH_TEST;
        if(mat) {
            if (mat->hasVertexShaders()) {
                wire_pass->addExtensionShaderSet(mat->getVertexShaders());
                wire_pass->extended_by_material |= 1 << SHADER_VERTEX;
            }
        }

        break;
    }
    case GPU_Role_Decal: {
        GPU_INTERMEDIATE_PASS_DESC* int_pass = ctx.getOrCreatePass(getPassId("Decals"));
        if(mat) {
            if (mat->hasFragmentShaders()) {
                int_pass->addExtensionShaderSet(mat->getFragmentShaders());
                int_pass->extended_by_material |= 1 << SHADER_FRAGMENT;
            }
        }
        gpuResolveMaterialParams(
            int_pass, mat,
            GPU_BLEND_MODE::ADD,
            GPU_BACKFACE_CULLING
        );

        GPU_INTERMEDIATE_PASS_DESC* wire_pass = ctx.getOrCreatePass(getPassId("Wireframe"));
        wire_pass->blend_mode = GPU_BLEND_MODE::BLEND;
        wire_pass->draw_flags = GPU_DEPTH_WRITE | GPU_DEPTH_TEST;
        break;
    }
    case GPU_Role_Water: {
        GPU_INTERMEDIATE_PASS_DESC* int_pass = nullptr;
        
        int_pass = ctx.getOrCreatePass(getPassId("HL2/Water"));

        if(mat) {
            if (mat->hasVertexShaders()) {
                int_pass->addExtensionShaderSet(mat->getVertexShaders());
                int_pass->extended_by_material |= 1 << SHADER_VERTEX;
            }
            if (mat->hasFragmentShaders()) {
                int_pass->addExtensionShaderSet(mat->getFragmentShaders());
                int_pass->extended_by_material |= 1 << SHADER_FRAGMENT;
            }
            gpuResolveMaterialParams(
                int_pass, mat,
                GPU_BLEND_MODE::BLEND,
                GPU_DEPTH_TEST | GPU_DEPTH_WRITE | GPU_BACKFACE_CULLING
            );
        }

        GPU_INTERMEDIATE_PASS_DESC* wire_pass = ctx.getOrCreatePass(getPassId("Wireframe"));
        wire_pass->blend_mode = GPU_BLEND_MODE::BLEND;
        wire_pass->draw_flags = GPU_DEPTH_WRITE | GPU_DEPTH_TEST;
        if(mat) {
            if (mat->hasVertexShaders()) {
                wire_pass->addExtensionShaderSet(mat->getVertexShaders());
                wire_pass->extended_by_material |= 1 << SHADER_VERTEX;
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
        GPU_INTERMEDIATE_PASS_DESC* cutout_pass = ctx.getOrCreatePass(getPassId("Outline/Cutout"));
        if(mat && mat->hasVertexShaders()) {
            color_pass->addExtensionShaderSet(mat->getVertexShaders());
            color_pass->extended_by_material |= 1 << SHADER_VERTEX;
            cutout_pass->addExtensionShaderSet(mat->getVertexShaders());
            cutout_pass->extended_by_material |= 1 << SHADER_VERTEX;
        }
        {
            GPU_BLEND_MODE blending = GPU_BLEND_MODE::BLEND;
            draw_flags_t draw_flags = GPU_DEPTH_TEST | GPU_DEPTH_WRITE | GPU_BACKFACE_CULLING;

            draw_flags = mat->getBackfaceCulling() ? (draw_flags | GPU_BACKFACE_CULLING) : (draw_flags & ~GPU_BACKFACE_CULLING);

            color_pass->blend_mode = blending;
            color_pass->draw_flags = draw_flags;
        }
        {
            GPU_BLEND_MODE blending = GPU_BLEND_MODE::MULTIPLY;
            draw_flags_t draw_flags = GPU_DEPTH_TEST | GPU_DEPTH_WRITE | GPU_BACKFACE_CULLING;

            draw_flags = mat->getBackfaceCulling() ? (draw_flags | GPU_BACKFACE_CULLING) : (draw_flags & ~GPU_BACKFACE_CULLING);

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

void gpuPipelineDefault::setDirectLightParams(const gfxm::mat4& view, const gfxm::mat4& proj) {
    dir_light_block->setView(view);
    dir_light_block->setProjection(proj);
}