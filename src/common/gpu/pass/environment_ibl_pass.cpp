#include "environment_ibl_pass.hpp"



EnvironmentIBLPass::EnvironmentIBLPass() {
    setColorTarget("Lightness", "Lightness");
    setTargetSampler("Albedo");
    setTargetSampler("Position");
    setTargetSampler("Normal");
    setTargetSampler("Metalness");
    setTargetSampler("Roughness");
    setTargetSampler("Emission");

    prog_env_ibl = resGet<gpuShaderProgram>("shaders/postprocess/environment_ibl.glsl");

    ibl_maps = loadIBLMapsFromHDRI(
        "cubemaps/hdri/belfast_sunset_puresky_1k.hdr"
    );
    /*
    ibl_maps = loadIBLMapsFromCubeSides(
        "cubemaps/Yokohama3/posx.jpg",
        "cubemaps/Yokohama3/negx.jpg",
        "cubemaps/Yokohama3/posy.jpg",
        "cubemaps/Yokohama3/negy.jpg",
        "cubemaps/Yokohama3/posz.jpg",
        "cubemaps/Yokohama3/negz.jpg"
    );*/

    //glGenBuffers(1, &ub_common);
}

void EnvironmentIBLPass::onDraw(gpuRenderTarget* target, gpuRenderBucket* bucket, int technique_id, const DRAW_PARAMS& params) {
    gpuFrameBufferBind(target->framebuffers[framebuffer_id].get());
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_LINE_SMOOTH);
    glDepthMask(GL_TRUE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    GLenum draw_buffers[] = { GL_COLOR_ATTACHMENT0, 0, 0, 0, 0, 0, 0, 0, 0, };
    glDrawBuffers(GPU_FRAME_BUFFER_MAX_DRAW_COLOR_BUFFERS, draw_buffers);
        
    static const string_id albedo_id("Albedo");
    static const string_id position_id("Position");
    static const string_id normal_id("Normal");
    static const string_id metalness_id("Metalness");
    static const string_id roughness_id("Roughness");
    static const string_id emission_id("Emission");

    int slot = prog_env_ibl->getDefaultSamplerSlot("texDiffuse");
    if (slot != -1) {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, target->textures[getTargetSamplerTextureIndex(albedo_id)]->getId());
    }
    slot = prog_env_ibl->getDefaultSamplerSlot("texWorldPos");
    if (slot != -1) {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, target->textures[getTargetSamplerTextureIndex(position_id)]->getId());
    }
    slot = prog_env_ibl->getDefaultSamplerSlot("texNormal");
    if (slot != -1) {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, target->textures[getTargetSamplerTextureIndex(normal_id)]->getId());
    }
    slot = prog_env_ibl->getDefaultSamplerSlot("texMetallic");
    if (slot != -1) {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, target->textures[getTargetSamplerTextureIndex(metalness_id)]->getId());
    }
    slot = prog_env_ibl->getDefaultSamplerSlot("texRoughness");
    if (slot != -1) {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, target->textures[getTargetSamplerTextureIndex(roughness_id)]->getId());
    }
    slot = prog_env_ibl->getDefaultSamplerSlot("texEmission");
    if (slot != -1) {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, target->textures[getTargetSamplerTextureIndex(emission_id)]->getId());
    }
    slot = prog_env_ibl->getDefaultSamplerSlot("texCubemapIrradiance");
    if (slot != -1) {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_CUBE_MAP, ibl_maps.irradiance);
    }
    slot = prog_env_ibl->getDefaultSamplerSlot("texCubemapSpecular");
    if (slot != -1) {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_CUBE_MAP, ibl_maps.specular);
    }
    slot = prog_env_ibl->getDefaultSamplerSlot("texBrdfLut");
    if (slot != -1) {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, tex_brdf_lut);
    }
    /*
    UniformBufferCommon ub_common_data;
    ub_common_data.matView = view;
    ub_common_data.matProjection = projection;
    ub_common_data.cameraPosition = gfxm::inverse(view)[3];
    ub_common_data.time = .0f;
    ub_common_data.viewportSize = gfxm::vec2(target->getWidth(), target->getHeight());
    ub_common_data.zNear = .1f; // TODO:
    ub_common_data.zFar = 1000.f; // TODO:
    glBindBuffer(GL_UNIFORM_BUFFER, ub_common);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(ub_common_data), &ub_common_data, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glBindBufferBase(GL_UNIFORM_BUFFER, 0, ub_common);
    */
    glUseProgram(prog_env_ibl->getId());
    //prog_env_ibl->setUniform3f("camPos", gfxm::inverse(view)[3]);
    int vp_x = params.viewport_x;
    int vp_y = params.viewport_y;
    int vp_width = params.viewport_width;
    int vp_height = params.viewport_height;
    glViewport(vp_x, vp_y, vp_width, vp_height);
    glScissor(vp_x, vp_y, vp_width, vp_height);
    gpuDrawFullscreenTriangle();
        
    glUseProgram(0);
    gpuFrameBufferUnbind();
}
