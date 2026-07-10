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
    });
}

void gpuDefaultRenderer::initView(EngineRenderView* rv) {
    rv->render_target.reset(new gpuRenderTarget(800, 600));
    rv->rt_map_clear.reset(new gpuRenderTargetMap());
    rv->rt_map_world.reset(new gpuRenderTargetMap());
    rv->rt_map_view.reset(new gpuRenderTargetMap());
    rv->rt_map_blit_depth.reset(new gpuRenderTargetMap());
    rv->rt_map_post.reset(new gpuRenderTargetMap());
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

    rseq_post.run(bucket, rv->rt_map_post.get(), params);

    //gpuDraw(bucket, target, params);
    bucket->clear();
}

