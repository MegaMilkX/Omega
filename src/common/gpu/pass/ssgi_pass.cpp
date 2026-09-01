#include "ssgi_pass.hpp"



gpuSSGIPass::gpuSSGIPass() {
    addColorSource("Normal", "Normal");
    addColorSource("Depth", "Depth");
    addColorSource("ORMM", "ORMM");
    addColorSource("PrevLightness", "PrevLightness");
    
    setBlending(GPU_BLEND_MODE::OVERWRITE);
    colorMask(1, 1, 1, 1);

    addBaseShaderSet(loadResource<gpuShaderSet>("core/shaders/ssgi"));
}

void gpuSSGIPass::onDraw(
    gpuPassInstance* inst,
    gpuRenderTargetMap* target_map,
    gpuRenderBucket* bucket,
    pipe_pass_id_t pass_id,
    const DRAW_PARAMS& params
) {
    bindFramebuffer(inst, target_map, params);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE);

    glEnable(GL_BLEND);
    gpuSetBlending(blend_mode);

    bindDefaultSamplerSet(target_map->getTarget(), inst);

    bindDefaultProgram();
    gpuDrawFullscreenTriangle();
    glBindVertexArray(0);

    gpuFrameBufferUnbind();
}



gpuSSGIDenoisePass::gpuSSGIDenoisePass() {
    addColorSource("Normal", "Normal");
    addColorSource("Depth", "Depth");
    addColorSource("SSGI_Raw", "SSGI_Test");
    setColorTarget("SSGI", "SSGI_Test");

    setBlending(GPU_BLEND_MODE::OVERWRITE);
    colorMask(1, 1, 1, 1);

    addBaseShaderSet(loadResource<gpuShaderSet>("core/shaders/ssgi_denoise"));
}

void gpuSSGIDenoisePass::onDraw(
    gpuPassInstance* inst,
    gpuRenderTargetMap* target_map,
    gpuRenderBucket* bucket,
    pipe_pass_id_t pass_id,
    const DRAW_PARAMS& params
) {
    bindFramebuffer(inst, target_map, params);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE);

    glEnable(GL_BLEND);
    gpuSetBlending(blend_mode);

    bindDefaultSamplerSet(target_map->getTarget(), inst);

    bindDefaultProgram();
    gpuDrawFullscreenTriangle();
    glBindVertexArray(0);

    gpuFrameBufferUnbind();
}


gpuSSGIComposePass::gpuSSGIComposePass() {
    addColorSource("Albedo", "Albedo");
    addColorSource("SSGI", "SSGI_Test");
    setColorTarget("Lightness", "Lightness");

    setBlending(GPU_BLEND_MODE::ADD);
    colorMask(1, 1, 1, 1);

    addBaseShaderSet(loadResource<gpuShaderSet>("core/shaders/ssgi_compose"));
}

void gpuSSGIComposePass::onDraw(
    gpuPassInstance* inst,
    gpuRenderTargetMap* target_map,
    gpuRenderBucket* bucket,
    pipe_pass_id_t pass_id,
    const DRAW_PARAMS& params
) {
    bindFramebuffer(inst, target_map, params);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE);

    glEnable(GL_BLEND);
    gpuSetBlending(blend_mode);

    bindDefaultSamplerSet(target_map->getTarget(), inst);

    bindDefaultProgram();
    gpuDrawFullscreenTriangle();
    glBindVertexArray(0);

    gpuFrameBufferUnbind();
}