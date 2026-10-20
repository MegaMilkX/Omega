#include "default_renderer.hpp"
#include "gpu/gpu.hpp"
#include "gpu/skinning/skinning_compute.hpp"


gpuDefaultRenderer::gpuDefaultRenderer() {
    rseq_clear.init("CLEAR", {
        "Clear/Zero",
        "Clear/Normal",
        "Clear/Inf",
        "Clear/DepthOverlay",
    });
    rseq_world.init("WORLD", {
        "Clear/Depth",
        "Default",
        "PreDecalBlit",
        "Decals_GBuffer",
    });
    rseq_layer.init("LAYER", {
        "Clear/Depth",
        "Default",
        "PreDecalBlit",
        "Decals_GBuffer",
    });
    rseq_blit_depth.init("BLIT_DEPTH", {
        "ViewModel/BlitDepth",
    });
    rseq_post.init("POST", {
        "SSAO/AO",
        "SSAO/Blur",
        "SSAO/Blit",
        "EnvironmentIBL",
        "ShadowmapTest",
        "SSGI/GI",
        "SSGI/Denoise",
        "SSGI/Compose",
        "SSGI/PrevBlit",
        "VelocityMapTest",      
        "PBRCompose",
        "Decals",
        "Fog",
        //"Posteffects/MotionBlur"
        "Skybox",
        "HL2/PreWaterBlit",
        "HL2/Water",
        "HL2/Translucent",
        "Posteffects/GammaTonemap",
        "VFX",
        "Error",
        "Wireframe",
        "Outline/Color",
        "Outline/Blur",
        "Outline/Cutout",
        "Outline/Blit",
        "PreOverlayBlit",
        "Overlay",
    });

    // =================
    rseq_shadowmap.init("SHADOWMAP", {
        "Clear/Shadowmap",
        "Shadowmap"
    });
}

void gpuDefaultRenderer::initView(EngineRenderView* rv) {
    rv->render_target.reset(new gpuRenderTarget(800, 600));
    gpuGetPipeline()->initRenderTarget(rv->render_target.get());
    gpuGetPipeline()->initRenderTargetMap(
        rv->render_target.get(), &rseq_clear
    );
    gpuGetPipeline()->initRenderTargetMap(
        rv->render_target.get(), &rseq_world
    );
    gpuGetPipeline()->initRenderTargetMap(
        rv->render_target.get(), &rseq_layer,
        { { "Depth", "DepthLayer" } }
    );
    gpuGetPipeline()->initRenderTargetMap(
        rv->render_target.get(), &rseq_blit_depth
    );
    gpuGetPipeline()->initRenderTargetMap(
        rv->render_target.get(), &rseq_post
    );
    gpuGetPipeline()->initRenderTargetMap(
        rv->render_target.get(), &rseq_shadowmap
    );
}

void gpuDefaultRenderer::draw(gpuRenderBucket* bucket, EngineRenderView* rv, DRAW_PARAMS& params) {
    gpuRunSkinTasks();
    gpuUpdateTransformSync();

    gpuRenderTarget* tgt = rv->getRenderTarget();

    bucket->sort(params);

    params.layer = -1;
    rseq_clear.run(bucket, tgt, params);

    auto& layers = bucket->getLayers();
    auto it = layers.begin();
    params.layer = *it;
    rseq_world.run(bucket, tgt, params);
    ++it;
    for (; it != layers.end(); ++it) {
        int layer = *it;
        params.layer = *it;
        rseq_layer.run(bucket, tgt, params);
        params.layer = -1; // Unnecessary, blit depth sequence has no passes that can utilize layers
        // TODO: Actually negative layers are now possible, so need to rethink how to signal "draw all layers immediately"
        // Might keep negative values as "draw all layers", then make cmds with negative layer no-ops
        rseq_blit_depth.run(bucket, tgt, params);
    }

    const float C = params.projection[2][2];
    const float D = params.projection[3][2];
    const float zNear = D / (C - 1.f);
    const float zFar = D / (C + 1.f);

    const float shadowDistance = zFar;
    const float firstCascadeFar = 4.f;
    const float lambda = .8f; // blend between uniform and logarithmic cascade distribution
    const float casterPad = 150.f; // padding towards the sun to capture far casters
    const float shadowRes = float(SHADOW_RESOLUTION); // for shadowmap view snapping, to avoid shimmer, TODO: make a constant like N_CASCADES

    float splits[N_CASCADES + 1];
    splits[0] = zNear;
    splits[N_CASCADES] = shadowDistance;
    if (N_CASCADES > 1) {
        const float first = gfxm::_min(firstCascadeFar, shadowDistance * .5f); // TODO: not sure about shadowDistance * .5f
        splits[1] = first;
        const int M = N_CASCADES - 1;
        for (int j = 1; j < M; ++j) {
            const float t = float(j) / float(M);
            const float uniform = first + (shadowDistance - first) * t;
            const float logarithmic = first * powf(shadowDistance / first, t);
            splits[j + 1] = gfxm::lerp(uniform, logarithmic, lambda);
        }
    }

    const gfxm::vec3 ldir = gfxm::normalize(gfxm::vec3(-1, -1, 1));
    const gfxm::vec3 up(0, 1, 0);
    const gfxm::mat4 invview = gfxm::inverse(params.view);
    const gfxm::mat4 lrot    = gfxm::lookAt(gfxm::vec3(0, 0, 0), ldir, up); // rotation only
    const gfxm::mat4 invlrot = gfxm::inverse(lrot);

    const float tx = 1.f / params.projection[0][0]; // tan(half fov x)
    const float ty = 1.f / params.projection[1][1]; // tan(half fov y)
    const float k = tx * tx + ty * ty;

    for(int icascade = 0; icascade < N_CASCADES; ++icascade) {
        const float n = splits[icascade];
        const float f = splits[icascade + 1];

        float cz = 0.5f * (n + f) * (1.f + k);
        cz = gfxm::_min(cz, f);
        const gfxm::vec3 center_view(0.f, 0.f, -cz);
        const gfxm::vec3 far_corner(f * tx, f * ty, -f);
        float radius = gfxm::length(far_corner - center_view);
        radius = ceilf(radius * 16.f) / 16.f;

        gfxm::vec3 center = gfxm::vec3(invview * gfxm::vec4(center_view, 1.f));
        
        // Snap the center to whole shadow texels in light space (kills shimmering)
        const float texel = (2.f * radius) / shadowRes;
        gfxm::vec3 cl = gfxm::vec3(lrot * gfxm::vec4(center, 1.f));
        cl.x = floorf(cl.x / texel) * texel;
        cl.y = floorf(cl.y / texel) * texel;
        center = gfxm::vec3(invlrot * gfxm::vec4(cl, 1.f));

        gfxm::mat4 lview = gfxm::lookAt(center, center + ldir, up);

        // View-space z = -distance along ldir. The sun side is +z, so a negative
        // "near" pushes the near plane back toward the sun to catch off-slice casters.
        const float ortho_near = -(radius + casterPad);
        const float ortho_far  = radius;
        gfxm::mat4 lproj = gfxm::ortho(-radius, radius, -radius, radius, ortho_near, ortho_far);
        const float depthRange = ortho_far - ortho_near; // 2*radius + casterPad

        gpuGetPipeline()->setCascadeParams(lview, lproj, f, 1.f / (2.f * radius), depthRange, icascade);

        DRAW_PARAMS params2 = params;
        params2.view = lview;
        params2.view_prev = lview;
        params2.projection = lproj;/*
        params2.viewport_x = 0;
        params2.viewport_y = 0;
        params2.viewport_width = rv->getWidth();
        params2.viewport_height = rv->getHeight();*/
        params2.layer = -1;
        rseq_shadowmap.run(bucket, tgt, params2, icascade);
    }

    gpuGetPipeline()->setLightDir(-ldir);
    rseq_post.run(bucket, tgt, params);

    bucket->clear();
}

