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

void gpuBindSamplers(gpuRenderTarget* target, gpuPass* pass, const ShaderSamplerSet* sampler_set) {
    for (int j = 0; j < sampler_set->count(); ++j) {
        const auto& sampler = sampler_set->get(j);
        glActiveTexture(GL_TEXTURE0 + sampler.slot);
        GLuint texture_id = 0;
        switch (sampler.source) {
        case SHADER_SAMPLER_SOURCE_GPU:
            texture_id = sampler.texture_id;
            break;
        case SHADER_SAMPLER_SOURCE_CHANNEL_IDX:
            // TODO: Handle double buffered
            texture_id = target->layers[sampler.channel_idx.idx].textures[sampler.channel_idx.buffer_idx]->getId();
            break;
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