#include "gpu_deferred_light_pass.hpp"

#include "gpu/gpu.hpp"


void gpuDeferredLightPass::gpuDrawShadowCubeMap(gpuRenderTarget* target, gpuRenderBucket* bucket, const gfxm::vec3& eye, gpuCubeMap* cubemap) {
    if (!shadow_cube_pass) {
        assert(false);
        return;
    }

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

        uint32_t last_prog_id = -1;
        uint32_t last_state_id = -1;
        uint32_t last_sampler_set_id = -1;
        auto& commands = bucket->getPassCommands(shadow_cube_pass->getId());
        for (int i = 0; i < commands.size(); ++i) { // all commands of the same technique
            auto& cmd = commands[i];
            if (last_sampler_set_id != cmd.sampler_set_id) {
                gpuBindSamplers(target, this, &cmd.rdr_pass->sampler_set);
                last_sampler_set_id = cmd.sampler_set_id;
            }
            if (last_prog_id != cmd.program_id) {
                gpuBindDrawBuffers(cmd);
                gpuBindProgram(cmd);
                last_prog_id = cmd.program_id;
            }
            if (last_state_id != cmd.state_id) {
                gpuSetModes(cmd);
                gpuSetBlending(cmd);
                last_state_id = cmd.state_id;
            }

            cmd.renderable->bindSamplerOverrides(cmd.renderable_pass_id);
            cmd.renderable->bindUniformBuffers();
            cmd.renderable->uploadUniforms(cmd.renderable_pass_id);

            auto binding = &cmd.rdr_pass->binding;
            if (cmd.instance_count > 0) { // TODO: possible instance count mismatch in cmd
                gpuBindMeshBinding(binding);
                gpuDrawMeshBindingInstanced(binding, cmd.renderable->getInstancingDesc()->getInstanceCount());
            } else {
                gpuBindMeshBinding(binding);
                gpuDrawMeshBinding(binding);
            }
            /*
            int material_end = cmd.next_material_id;

            const gpuMaterial* material = cmd.renderable->getMaterial();
            //material->bindSamplers();
            material->bindUniformBuffers();
                
            for (; i < material_end; ++i) { // iterate over commands with the same material
                gpuBindSamplers(target, this, &cmd.rdr_pass->sampler_set);
                //gpuBindSamplers(target, this, &pass->getSamplerSet());

                gpuBindDrawBuffers(cmd);
                //pass->bindDrawBuffers();

                gpuBindProgram(cmd);
                //pass->bindShaderProgram();

                auto& cmd = bucket->commands[i];
                gpuSetModes(cmd);
                gpuSetBlending(cmd);

                auto binding = &cmd.rdr_pass->binding;
                if (cmd.instance_count > 0) { // TODO: possible instance count mismatch in cmd
                    cmd.renderable->bindUniformBuffers();
                    gpuBindMeshBinding(binding);
                    gpuDrawMeshBindingInstanced(binding, cmd.renderable->getInstancingDesc()->getInstanceCount());
                } else {
                    cmd.renderable->bindUniformBuffers();
                    gpuBindMeshBinding(binding);
                    gpuDrawMeshBinding(binding);
                }
                glUseProgram(0);
            }*/
        }
    }

    glCullFace(GL_BACK);

    glBindVertexArray(0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);/*
    glDeleteFramebuffers(1, &capFbo);*/
}