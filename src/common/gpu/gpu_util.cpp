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


void gpuBindMeshBinding(const gpuMeshShaderBinding* binding) {
    glBindVertexArray(binding->vao);
    //gpuBindMeshBindingDirect(binding);
}
void gpuBindMeshBindingDirect(const gpuMeshShaderBinding* binding) {
    for (auto& a : binding->attribs) {
        if (!a.buffer) {
            LOG_ERR("gpuBindMeshBindingDirect: " << VFMT::getAttribDesc(a.guid)->name << " buffer is null");
            assert(false);
            continue;
        }
        if (a.buffer->getId() == 0) {
            LOG_ERR("gpuBindMeshBindingDirect: invalid " << VFMT::getAttribDesc(a.guid)->name << " buffer id");
            assert(false);
            continue;
        }
        if (a.buffer->getSize() == 0) {
            LOG_WARN("gpuBindMeshBindingDirect: " << VFMT::getAttribDesc(a.guid)->name << " buffer is empty");
            // NOTE: On first renderable compilation buffers can be empty, which is not an error
            // Still, would be nice to catch empty buffers before drawing
            //assert(false);
            //continue;
        }
        GL_CHECK(glEnableVertexAttribArray(a.location));
        GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, a.buffer->getId()));
        GL_CHECK(glVertexAttribPointer(
            a.location, a.count, a.gl_type, a.normalized, a.stride, (void*)0
        ));
        if (a.is_instance_array) {
            glVertexAttribDivisor(a.location, 1);
        } else {
            glVertexAttribDivisor(a.location, 0);
        }
    }
    if (binding->index_buffer) {
        GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, binding->index_buffer->getId()));
    }
}
void gpuDrawMeshBinding(const gpuMeshShaderBinding* b) {
    GLenum mode;
    switch (b->draw_mode) {
    case MESH_DRAW_POINTS: mode = GL_POINTS; break;
    case MESH_DRAW_LINES: mode = GL_LINES; break;
    case MESH_DRAW_LINE_STRIP: mode = GL_LINE_STRIP; break;
    case MESH_DRAW_LINE_LOOP: mode = GL_LINE_LOOP; break;
    case MESH_DRAW_TRIANGLES: mode = GL_TRIANGLES; break;
    case MESH_DRAW_TRIANGLE_STRIP: mode = GL_TRIANGLE_STRIP; break;
    case MESH_DRAW_TRIANGLE_FAN: mode = GL_TRIANGLE_FAN; break;
    default: assert(false);
    };
    if (b->index_buffer) {
        GL_CHECK(glDrawElements(mode, b->index_count, GL_UNSIGNED_INT, 0));
    } else {
        GL_CHECK(glDrawArrays(mode, 0, b->vertex_count));
    }
}
void gpuDrawMeshBindingInstanced(const gpuMeshShaderBinding* binding, int instance_count) {
    GLenum mode;
    switch (binding->draw_mode) {
    case MESH_DRAW_POINTS: mode = GL_POINTS; break;
    case MESH_DRAW_LINES: mode = GL_LINES; break;
    case MESH_DRAW_LINE_STRIP: mode = GL_LINE_STRIP; break;
    case MESH_DRAW_LINE_LOOP: mode = GL_LINE_LOOP; break;
    case MESH_DRAW_TRIANGLES: mode = GL_TRIANGLES; break;
    case MESH_DRAW_TRIANGLE_STRIP: mode = GL_TRIANGLE_STRIP; break;
    case MESH_DRAW_TRIANGLE_FAN: mode = GL_TRIANGLE_FAN; break;
    default: assert(false);
    };
    if (binding->index_buffer) {
        glDrawElementsInstanced(mode, binding->index_count, GL_UNSIGNED_INT, 0, instance_count);
    } else {
        glDrawArraysInstanced(mode, 0, binding->vertex_count, instance_count);
    }
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
        case SHADER_SAMPLER_SOURCE_FRAME_IMAGE_STRING_ID:
            // TODO: Handle double buffered
            texture_id = target->layers[pass->getColorSourceTextureIndex(sampler.frame_image_id)].texture_a->getId();
            break;
        case SHADER_SAMPLER_SOURCE_FRAME_IMAGE_IDX:
            // TODO: Handle double buffered
            texture_id = target->layers[sampler.frame_image_idx].texture_a->getId();
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
        case SHADER_SAMPLER_TEXTURE_BUFFER:
            target = GL_TEXTURE_BUFFER;
            break;
        default:
            // Unsupported
            assert(false);
            continue;
        }
        glBindTexture(target, texture_id);
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