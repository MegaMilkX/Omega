#include "default_renderer.hpp"
#include "gpu/gpu.hpp"
#include "gpu/skinning/skinning_compute.hpp"


gpuDefaultRenderer::gpuDefaultRenderer() {
    rseq_clear.init({
        "Clear/Zero",
        "Clear/Normal",
        "Clear/Inf",
        "Clear/DepthOverlay",
    });
    rseq_world.init({
        "Clear/Depth",
        "Default",
    });
    rseq_blit_depth.init({
        "ViewModel/BlitDepth",
    });
    rseq_post.init({
        "SSAO/AO",
        "SSAO/Blur",
        "EnvironmentIBL",
        "ShadowmapTest",
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
        "Wireframe",
        "Outline/Color",
        "Outline/Blur",
        "Outline/Cutout",
        "Outline/Blit",
        "PreOverlayBlit",
        "Overlay",
    });

    // =================
    rseq_shadowmap.init({
        "Clear/Shadowmap",
        "Shadowmap"
    });
}

void gpuDefaultRenderer::initView(EngineRenderView* rv) {
    rv->render_target.reset(new gpuRenderTarget(800, 600));
    rv->rt_map_clear.reset(new gpuRenderTargetMap());
    rv->rt_map_world.reset(new gpuRenderTargetMap());
    rv->rt_map_view.reset(new gpuRenderTargetMap());
    rv->rt_map_blit_depth.reset(new gpuRenderTargetMap());
    rv->rt_map_post.reset(new gpuRenderTargetMap());
    rv->rt_map_shadowmap.reset(new gpuRenderTargetMap());
    gpuGetPipeline()->initRenderTarget(rv->render_target.get());
    gpuGetPipeline()->initRenderTargetMap(
        rv->rt_map_clear.get(), rv->render_target.get(), &rseq_clear
    );
    gpuGetPipeline()->initRenderTargetMap(
        rv->rt_map_world.get(), rv->render_target.get(), &rseq_world
    );
    gpuGetPipeline()->initRenderTargetMap(
        rv->rt_map_view.get(), rv->render_target.get(), &rseq_world,
        { { "Depth", "DepthLayer" } }
    );
    gpuGetPipeline()->initRenderTargetMap(
        rv->rt_map_blit_depth.get(), rv->render_target.get(), &rseq_blit_depth
    );
    gpuGetPipeline()->initRenderTargetMap(
        rv->rt_map_post.get(), rv->render_target.get(), &rseq_post
    );
    gpuGetPipeline()->initRenderTargetMap(
        rv->rt_map_shadowmap.get(), rv->render_target.get(), &rseq_shadowmap
    );
}

void gpuDefaultRenderer::draw(gpuRenderBucket* bucket, EngineRenderView* rv, DRAW_PARAMS& params) {
    gpuRunSkinTasks();
    gpuUpdateTransformSync();

    bucket->sort(params);

    params.layer = -1;
    rseq_clear.run(bucket, rv->rt_map_clear.get(), params);

    auto& layers = bucket->getLayers();
    auto it = layers.begin();
    params.layer = *it;
    rseq_world.run(bucket, rv->rt_map_world.get(), params);
    ++it;
    for (; it != layers.end(); ++it) {
        int layer = *it;
        params.layer = *it;
        rseq_world.run(bucket, rv->rt_map_view.get(), params);
        params.layer = -1; // Unnecessary, blit depth sequence has no passes that can utilize layers
        // TODO: Actually negative layers are now possible, so need to rethink how to signal "draw all layers immediately"
        // Might keep negative values as "draw all layers", then make cmds with negative layer no-ops
        rseq_blit_depth.run(bucket, rv->rt_map_blit_depth.get(), params);
    }

    {
        gfxm::vec3 points[8] = {
            { -1, -1, -1 },
            { 1, -1, -1 },
            { 1, 1, -1 },
            { -1, 1, -1 },
            { -1, -1, 1 },
            { 1, -1, 1 },
            { 1, 1, 1 },
            { -1, 1, 1 },
        };

        gfxm::mat4 proj = params.projection;//gfxm::perspective(gfxm::radian(65.f), 1.f/1.f, .1f, 30.f);
        {
            const float zfar_new = 30.f;
            float znear = proj[3][2] / (proj[2][2] - 1.0f);
            proj[2][2] = -(zfar_new + znear) / (zfar_new - znear);
            proj[3][2] = -(2.0f * zfar_new * znear) / (zfar_new - znear);
        }
        gfxm::mat4 invproj = gfxm::inverse(proj);
        gfxm::mat4 invview = gfxm::inverse(params.view);

        for (int i = 0; i < 8; ++i) {
            auto& pt = points[i];
            gfxm::vec4 pt4 = invproj * gfxm::vec4(pt, 1);
            pt = gfxm::vec3(pt4.x, pt4.y, pt4.z);
            pt /= pt4.w;
            pt = invview * gfxm::vec4(pt, 1);
        }

        gfxm::vec3 midpoint;
        gfxm::vec3 a;
        gfxm::vec3 b;
        gfxm::vec3 c;
        gfxm::vec3 d;
        a = gfxm::lerp(points[0], points[1], .5f);
        b = gfxm::lerp(points[2], points[3], .5f);
        c = gfxm::lerp(a, b, .5f);
        a = gfxm::lerp(points[4], points[5], .5f);
        b = gfxm::lerp(points[6], points[7], .5f);
        d = gfxm::lerp(a, b, .5f);
        midpoint = gfxm::lerp(c, d, .5f);

        gfxm::vec3 ldir = gfxm::normalize(gfxm::vec3(-1, -1, 1));
        gfxm::mat4 lview = gfxm::lookAt(midpoint, midpoint + ldir, gfxm::vec3(0, 1, 0));
        gfxm::mat4 invlview = gfxm::inverse(lview);
        
        gfxm::vec2 points2d[8] = {};
        for (int i = 0; i < 8; ++i) {
            auto& pt = points[i];
            const gfxm::vec3 xaxis = invlview[0];
            const gfxm::vec3 yaxis = invlview[1];
            float x = gfxm::dot(xaxis, pt);
            float y = gfxm::dot(yaxis, pt);
            points2d[i] = gfxm::vec2(x, y);
        }

        gfxm::rect rc;
        rc.min = points2d[0];
        rc.max = points2d[0];
        for (int i = 1; i < 8; ++i) {
            gfxm::expand(rc, points2d[i]);
        }
        gfxm::vec2 rc_half_size((rc.max.x - rc.min.x) * .5f, (rc.max.y - rc.min.y) * .5f);
        gfxm::mat4 lproj = gfxm::ortho(-rc_half_size.x, rc_half_size.x, -rc_half_size.y, rc_half_size.y, -100.f, 100.f);

        gpuGetPipeline()->setDirectLightParams(lview, lproj);

        DRAW_PARAMS params2 = params;
        params2.view = lview;
        params2.view_prev = lview;
        params2.projection = lproj;/*
        params2.viewport_x = 0;
        params2.viewport_y = 0;
        params2.viewport_width = rv->getWidth();
        params2.viewport_height = rv->getHeight();*/
        params2.layer = -1;
        rseq_shadowmap.run(bucket, rv->rt_map_shadowmap.get(), params2);
    }

    rseq_post.run(bucket, rv->rt_map_post.get(), params);

    //gpuDraw(bucket, target, params);
    bucket->clear();
}

