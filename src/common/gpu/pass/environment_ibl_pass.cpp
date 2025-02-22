#include "environment_ibl_pass.hpp"



EnvironmentIBLPass::EnvironmentIBLPass() {
    setColorTarget("Lightness", "Lightness");
    addColorSource("texDiffuse", "Albedo");
    addColorSource("texWorldPos", "Position");
    addColorSource("texNormal", "Normal");
    addColorSource("texMetallic", "Metalness");
    addColorSource("texRoughness", "Roughness");
    addColorSource("texEmission", "Emission");

    prog_env_ibl = addShader(resGet<gpuShaderProgram>("shaders/postprocess/environment_ibl.glsl"));

    ibl_maps = loadIBLMapsFromHDRI(
        //"cubemaps/hdri/belfast_sunset_puresky_1k.hdr"
        //"cubemaps/hdri/studio_small_02_1k.hdr"
        //"cubemaps/hdri/2/moonless_golf_2k.hdr"
        //"cubemaps/hdri/3/mud_road_puresky_2k.hdr"
        "cubemaps/hdri/3/overcast_soil_puresky_2k.hdr"
        //""
    );

    addTexture("texCubemapIrradiance", ibl_maps.irradiance, SHADER_SAMPLER_CUBE_MAP);
    addTexture("texCubemapSpecular", ibl_maps.specular, SHADER_SAMPLER_CUBE_MAP);
    addTexture("texBrdfLut", tex_brdf_lut);

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
    
    gpuBindSamplers(target, this, getSamplerSet(0));

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
