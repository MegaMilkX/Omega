#include "gpu_util.hpp"

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
    for (auto& a : binding->attribs) {
        if (!a.buffer) {
            assert(false);
            continue;
        }
        GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, a.buffer->getId()));
        glEnableVertexAttribArray(a.location);
        glVertexAttribPointer(
            a.location, a.count, a.gl_type, a.normalized, a.stride, (void*)0
        );
        if (a.is_instance_array) {
            glVertexAttribDivisor(a.location, 1);
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
        glDrawElements(mode, b->index_count, GL_UNSIGNED_INT, 0);
    } else {
        glDrawArrays(mode, 0, b->vertex_count);
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

void gpuDrawFullscreenTriangle() {
    glBindVertexArray(fullscreen_triangle_vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}
void gpuDrawCubeMapCube() {
    glBindVertexArray(cube_map_cube_vao);
    glDrawArrays(GL_TRIANGLES, 0, 12 * 3);
}