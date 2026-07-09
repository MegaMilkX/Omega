#include "gpu_util.hpp"

#include "gpu/gpu_render_target.hpp"
#include "gpu/pass/gpu_pass.hpp"
#include "gpu/gpu_material.hpp"

static GLuint fullscreen_triangle_vao = 0;
static GLuint fullscreen_triangle_vbo = 0;
static GLuint cube_map_cube_vao = 0;
static GLuint cube_map_cube_vbo = 0;


bool gpuUtilInit() {{
        float vertices[] = {
            -1.0f, -1.0f, 0.0f,     3.0f, -1.0f, 0.0f,      -1.0f, 3.0f, 0.0f
        };

        glGenVertexArrays(1, &fullscreen_triangle_vao);
        glBindVertexArray(fullscreen_triangle_vao);
        glGenBuffers(1, &fullscreen_triangle_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, fullscreen_triangle_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(
            0,
            3, GL_FLOAT, GL_FALSE,
            0, (void*)0 /* offset */
        );

        glBindVertexArray(0);
    }
    {
        static const GLfloat vertices[] = {
            -1.0f,-1.0f,-1.0f,
            -1.0f, 1.0f, 1.0f,
            -1.0f,-1.0f, 1.0f,
            1.0f, 1.0f,-1.0f,
            -1.0f, 1.0f,-1.0f,
            -1.0f,-1.0f,-1.0f,
            1.0f,-1.0f, 1.0f,
            1.0f,-1.0f,-1.0f,
            -1.0f,-1.0f,-1.0f,
            1.0f, 1.0f,-1.0f,
            -1.0f,-1.0f,-1.0f,
            1.0f,-1.0f,-1.0f,
            -1.0f,-1.0f,-1.0f,
            -1.0f, 1.0f,-1.0f,
            -1.0f, 1.0f, 1.0f,
            1.0f,-1.0f, 1.0f,
            -1.0f,-1.0f,-1.0f,
            -1.0f,-1.0f, 1.0f,
            -1.0f, 1.0f, 1.0f,
            1.0f,-1.0f, 1.0f,
            -1.0f,-1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f,-1.0f,
            1.0f,-1.0f,-1.0f,
            1.0f,-1.0f,-1.0f,
            1.0f,-1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            -1.0f, 1.0f,-1.0f,
            1.0f, 1.0f,-1.0f,
            1.0f, 1.0f, 1.0f,
            -1.0f, 1.0f, 1.0f,
            -1.0f, 1.0f,-1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f,-1.0f, 1.0f,
            -1.0f, 1.0f, 1.0f
        };

        glGenVertexArrays(1, &cube_map_cube_vao);
        glBindVertexArray(cube_map_cube_vao);
        glGenBuffers(1, &cube_map_cube_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, cube_map_cube_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(
            0,
            3, GL_FLOAT, GL_FALSE,
            0, (void*)0 /* offset */
        );

        glBindVertexArray(0);
    }
    return true;
}
void gpuUtilCleanup() {
    glDeleteVertexArrays(1, &fullscreen_triangle_vao);
    glDeleteBuffers(1, &fullscreen_triangle_vbo);

    glDeleteVertexArrays(1, &cube_map_cube_vao);
    glDeleteBuffers(1, &cube_map_cube_vbo);
}

void gpuBindSamplers(const gpuRenderTarget* target, gpuPassInstance* pass_inst, const ShaderSamplerSet* sampler_set) {
    gpuPass* pass = pass_inst->pass;

    for (int j = 0; j < sampler_set->count(); ++j) {
        const auto& sampler = sampler_set->get(j);
        glActiveTexture(GL_TEXTURE0 + sampler.slot);
        GLuint texture_id = 0;
        switch (sampler.source) {
        case SHADER_SAMPLER_SOURCE_GPU:
            texture_id = sampler.texture_id;
            break;
        case SHADER_SAMPLER_SOURCE_CHANNEL_IDX: {
            // TODO: Handle double buffered
            int rt_ch_idx = target->pipe_channel_to_layer[sampler.pipe_channel_index];
            if (rt_ch_idx < 0) {
                assert(false);
                continue;
            }
            gpuPassInstance::ChannelDesc* inst_ch_desc = &pass_inst->channels[pass_inst->rt_chan_to_pass[rt_ch_idx]];
            texture_id = target->layers[rt_ch_idx].textures[inst_ch_desc->lwt_buffer_idx]->getId();
            break;
        }
        default:
            // Unsupported
            assert(false);
            continue;
        }
        GLenum target = 0;
        switch (sampler.type) {
        case SHADER_SAMPLER_TEXTURE2D:
            target = GL_TEXTURE_2D;
            break;
        case SHADER_SAMPLER_CUBE_MAP:
            target = GL_TEXTURE_CUBE_MAP;
            break;
        case SHADER_SAMPLER_TEXTURE_BUFFER:
            target = GL_TEXTURE_BUFFER;
            break;
        default:
            // Unsupported
            assert(false);
            continue;
        }
        GL_CHECK(glBindTexture(target, texture_id));
    }
}

void gpuMakeDrawBuffersArray(gpuPass* pip_pass, GLuint programid, GLenum* draw_buffers, int max_count) {
    const int MAX_COLOR_OUTPUTS = platformGeti(PLATFORM_MAX_COLOR_OUTPUTS);

    int fb_attachment_idx = 0;
    memset(draw_buffers, 0, max_count * sizeof(draw_buffers[0]));
    for (int j = 0; j < pip_pass->channelCount(); ++j) {
        const gpuPass::ChannelDesc* ch_desc = pip_pass->getChannelDesc(j);
        if (!ch_desc->writes) {
            continue;
        }

        const std::string& tgt_name = ch_desc->target_local_name;
        std::string out_name = MKSTR("out" << tgt_name);
        GLint loc = glGetFragDataLocation(programid, out_name.c_str());
        if (loc == -1) {
            ++fb_attachment_idx;
            continue;
        }
        if (loc >= platformGeti(PLATFORM_MAX_COLOR_OUTPUTS)) {
            LOG_ERR("Renderable: Fragment shader output location exceeds limit");
            assert(false);
            break;
        }

        draw_buffers[loc] = GL_COLOR_ATTACHMENT0 + fb_attachment_idx;
        ++fb_attachment_idx;
    }
}

void gpuDrawFullscreenTriangle() {
    glBindVertexArray(fullscreen_triangle_vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}
void gpuDrawCubeMapCube() {
    glBindVertexArray(cube_map_cube_vao);
    glDrawArrays(GL_TRIANGLES, 0, 12 * 3);
    glBindVertexArray(0);
}