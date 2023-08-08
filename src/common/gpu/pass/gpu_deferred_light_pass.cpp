#include "gpu_deferred_light_pass.hpp"

#include "gpu/gpu.hpp"


void gpuDeferredLightPass::gpuDrawShadowCubeMap(gpuRenderTarget* target, gpuRenderBucket* bucket, const gfxm::vec3& eye, gpuCubeMap* cubemap) {
    gfxm::mat4 proj = gfxm::perspective(gfxm::radian(90.0f), 1.0f, 0.1f, 1000.0f);
    gfxm::mat4 views[] =
    {
        gfxm::lookAt(eye, eye + gfxm::vec3(1.0f,  0.0f,  0.0f), gfxm::vec3(0.0f, -1.0f,  0.0f)),
        gfxm::lookAt(eye, eye + gfxm::vec3(-1.0f,  0.0f,  0.0f), gfxm::vec3(0.0f, -1.0f,  0.0f)),
        gfxm::lookAt(eye, eye + gfxm::vec3(0.0f,  1.0f,  0.0f), gfxm::vec3(0.0f,  0.0f,  1.0f)),
        gfxm::lookAt(eye, eye + gfxm::vec3(0.0f, -1.0f,  0.0f), gfxm::vec3(0.0f,  0.0f, -1.0f)),
        gfxm::lookAt(eye, eye + gfxm::vec3(0.0f,  0.0f,  1.0f), gfxm::vec3(0.0f, -1.0f,  0.0f)),
        gfxm::lookAt(eye, eye + gfxm::vec3(0.0f,  0.0f, -1.0f), gfxm::vec3(0.0f, -1.0f,  0.0f))
    };

    const int side = 1024;
    /*
    GLuint capFbo;
    glGenFramebuffers(1, &capFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, capFbo);
    // Disable writes to the color buffer
    //glDrawBuffer(GL_NONE);
    // Disable reads from the color buffer
    //glReadBuffer(GL_NONE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X, cubemap->getId(), 0);
    GLenum draw_buffers[GPU_FRAME_BUFFER_MAX_DRAW_COLOR_BUFFERS] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    glDrawBuffers(GPU_FRAME_BUFFER_MAX_DRAW_COLOR_BUFFERS, draw_buffers);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        LOG_ERR("Shadow cube map fbo not complete!");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    */

    glBindVertexArray(shadow_vao);

    glViewport(0, 0, side, side);
    glScissor(0, 0, side, side);
    
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glDisable(GL_SCISSOR_TEST);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ZERO);
    glDepthMask(GL_TRUE);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, cube_capture_fbo);

    for (unsigned s = 0; s < 6; ++s) {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X + s, cubemap->getId(), 0);
        GLenum err = glGetError();
        assert(err == GL_NO_ERROR);
        glClear(GL_DEPTH_BUFFER_BIT);

        gpuGetPipeline()->setShadowmapCamera(proj, views[s]);
        
        static auto pipe_tech = gpuGetPipeline()->findTechnique("ShadowCubeMap");
        for (int j = 0; j < pipe_tech->passCount(); ++j) {
            auto pipe_pass = pipe_tech->getPass(j);
            
            auto group = bucket->getTechniqueGroup(pipe_tech->getId());
            for (int i = group.start; i < group.end;) { // all commands of the same technique
                auto& cmd = bucket->commands[i];
                int material_end = cmd.next_material_id;

                const gpuMaterial* material = cmd.renderable->getMaterial();
                //material->bindSamplers();
                material->bindUniformBuffers();
                
                for (; i < material_end;) { // iterate over commands with the same material
                    auto material_tech = cmd.renderable->getMaterial()->getTechniqueByPipelineId(cmd.id.getTechnique());
                    auto pass = material_tech->getPass(cmd.id.getPass());
                    pass->depth_test ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
                    pass->stencil_test ? glEnable(GL_STENCIL_TEST) : glDisable(GL_STENCIL_TEST);
                    pass->cull_faces ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
                    pass->depth_write ? glDepthMask(GL_TRUE) : glDepthMask(GL_FALSE);
                    switch (pass->blend_mode) {
                    case GPU_BLEND_MODE::NORMAL:
                        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                        break;
                    case GPU_BLEND_MODE::ADD:
                        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                        break;
                    case GPU_BLEND_MODE::MULTIPLY:
                        glBlendFunc(GL_DST_COLOR, GL_ZERO);
                        break;
                    default:
                        assert(false);
                    }
                    pass->bindSamplers();
                    for (int pobid = 0; pobid < pass->passOutputBindingCount(); ++pobid) {
                        auto& pob = pass->getPassOutputBinding(pobid);
                        glActiveTexture(GL_TEXTURE0 + pob.texture_slot);
                        auto& texture = target->textures[pipe_pass->getTargetSamplerTextureIndex(pob.strid)];
                        glBindTexture(GL_TEXTURE_2D, texture->getId());
                    }
                    pass->bindDrawBuffers();
                    pass->bindShaderProgram();

                    int pass_end = cmd.next_pass_id;
                    for (; i < pass_end; ++i) { // iterate over commands with the same shader(pass)
                        auto& cmd = bucket->commands[i];
                        if (cmd.instance_count > 0) { // TODO: possible instance count mismatch in cmd
                            cmd.renderable->bindUniformBuffers();
                            gpuBindMeshBinding(cmd.binding);
                            gpuDrawMeshBindingInstanced(cmd.binding, cmd.renderable->getInstancingDesc()->getInstanceCount());
                        } else {
                            cmd.renderable->bindUniformBuffers();
                            gpuBindMeshBinding(cmd.binding);
                            gpuDrawMeshBinding(cmd.binding);
                        }
                    }
                    glUseProgram(0);
                }
            }
        }
    }

    glCullFace(GL_BACK);

    glBindVertexArray(0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);/*
    glDeleteFramebuffers(1, &capFbo);*/
}