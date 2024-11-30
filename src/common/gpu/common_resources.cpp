#include "gpu/common_resources.hpp"
#include <assert.h>
#include <map>

#include "log/log.hpp"
#include "gpu/util_shader.hpp"


GLuint vao_screen_triangle = 0;
static GLuint vbo_screen_triangle_vertices = 0;
GLuint vao_inverted_cube = 0;
static GLuint vbo_inverted_cube_vertices = 0;

GLuint tex_brdf_lut = 0;

static std::map<std::string, RHSHARED<gpuTexture2d>> default_textures;


static GLuint createFramebufferTexture2d(int width, int height, GLint internalFormat) {
    GLuint tex;
    glGenTextures(1, &tex);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    //glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}

GLuint makeBrdfLut(GLuint vao_triangle) {
    GLuint tex_brdf = 0;
    auto prog_brdf = loadUtilShader("core/shaders/ibl/integrate_brdf.glsl");
    tex_brdf = createFramebufferTexture2d(512, 512, GL_RG16F);
    GLuint fbo;
    {
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex_brdf, 0);
        GLenum draw_buffers[] = {
            GL_COLOR_ATTACHMENT0,
        };
        glDrawBuffers(1, draw_buffers);
        if (!glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) {
            LOG_ERR("gl/framebuffer", "Framebuffer is incomplete");
            return -1;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    glFrontFace(GL_CCW);
    glViewport(0, 0, 512, 512);
    glScissor(0, 0, 512, 512);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glUseProgram(prog_brdf);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glBindVertexArray(vao_triangle);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glUseProgram(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindVertexArray(0);
    return tex_brdf;
}

bool initCommonResources() {
    // Screen triangle
    {
        float vertices[] = {
            -1.f, -1.f, .0f,    3.f, -1.f, .0f,     -1.f, 3.f, .0f
        };
        vbo_screen_triangle_vertices = 0;
        glGenBuffers(1, &vbo_screen_triangle_vertices);
        assert(vbo_screen_triangle_vertices != 0);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_screen_triangle_vertices);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glGenVertexArrays(1, &vao_screen_triangle);
        glBindVertexArray(vao_screen_triangle);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_screen_triangle_vertices);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glBindVertexArray(0);
    }

    // Inverted cube for cubemapping
    {
        float vertices[] = {
            -.5f, -.5f, .5f,     -.5f,  .5f, .5f,    .5f, -.5f, .5f,
             .5f, -.5f, .5f,     -.5f,  .5f, .5f,    .5f, .5f,  .5f,

             .5f, -.5f, .5f,     .5f,  .5f, .5f,    .5f, -.5f, -.5f,
             .5f, -.5f, -.5f,     .5f,  .5f, .5f,   .5f,  .5f, -.5f,

             .5f, -.5f, -.5f,     .5f,  .5f, -.5f,  -.5f, -.5f, -.5f,
            -.5f, -.5f, -.5f,     .5f,  .5f, -.5f,  -.5f,  .5f, -.5f,

            -.5f, -.5f, -.5f,    -.5f,  .5f, -.5f,  -.5f, -.5f,  .5f,
            -.5f, -.5f,  .5f,    -.5f,  .5f, -.5f,  -.5f,  .5f,  .5f,

            -.5f,  .5f,  .5f,    -.5f,  .5f, -.5f,   .5f,  .5f,  .5f,
             .5f,  .5f,  .5f,    -.5f,  .5f, -.5f,   .5f,  .5f, -.5f,

            -.5f, -.5f, -.5f,    -.5f, -.5f,  .5f,   .5f, -.5f, -.5f,
             .5f, -.5f, -.5f,    -.5f, -.5f,  .5f,   .5f, -.5f,  .5f,
        };

        vbo_inverted_cube_vertices = 0;

        glGenBuffers(1, &vbo_inverted_cube_vertices);
        assert(vbo_inverted_cube_vertices != 0);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_inverted_cube_vertices);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glGenVertexArrays(1, &vao_inverted_cube);
        glBindVertexArray(vao_inverted_cube);

        glBindBuffer(GL_ARRAY_BUFFER, vbo_inverted_cube_vertices);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

        glBindVertexArray(0);
    }

    tex_brdf_lut = makeBrdfLut(vao_screen_triangle);

    // Default textures
    {

        RHSHARED<gpuTexture2d> albedo;
        RHSHARED<gpuTexture2d> normal;
        RHSHARED<gpuTexture2d> ao;
        RHSHARED<gpuTexture2d> roughness;
        RHSHARED<gpuTexture2d> metallic;
        RHSHARED<gpuTexture2d> emission;
        RHSHARED<gpuTexture2d> lightmap;
        uint32_t albedo_color[] = { 0xFFFFFFFF };
        uint32_t normal_color[] = { 0xFFFF8080 };
        uint32_t ao_color[] = { 0xFFFFFFFF };
        uint32_t roughness_color[] = { 0xFF0000FB };
        uint32_t metallic_color[] = { 0xFF000000 };
        uint32_t emission_color[] = { 0xFF000000 };
        uint32_t lightmap_color[] = { 0xFFFFFFFF };
        albedo.reset_acquire();
        albedo->setData(albedo_color, 1, 1, 4, IMAGE_CHANNEL_UNSIGNED_BYTE, false);
        normal.reset_acquire();
        normal->setData(normal_color, 1, 1, 4, IMAGE_CHANNEL_UNSIGNED_BYTE, false);
        ao.reset_acquire();
        ao->setData(ao_color, 1, 1, 4, IMAGE_CHANNEL_UNSIGNED_BYTE, false);
        roughness.reset_acquire();
        roughness->setData(roughness_color, 1, 1, 4, IMAGE_CHANNEL_UNSIGNED_BYTE, false);
        metallic.reset_acquire();
        metallic->setData(metallic_color, 1, 1, 4, IMAGE_CHANNEL_UNSIGNED_BYTE, false);
        emission.reset_acquire();
        emission->setData(emission_color, 1, 1, 4, IMAGE_CHANNEL_UNSIGNED_BYTE, false);
        lightmap.reset_acquire();
        lightmap->setData(lightmap_color, 1, 1, 4, IMAGE_CHANNEL_UNSIGNED_BYTE, false);
        default_textures["texAlbedo"] = albedo;
        default_textures["texNormal"] = normal;
        default_textures["texAmbientOcclusion"] = ao;
        default_textures["texRoughness"] = roughness;
        default_textures["texMetallic"] = metallic;
        default_textures["texEmission"] = emission;
        default_textures["texLightmap"] = lightmap;
    }

    return true;
}

void cleanupCommonResources() {
    glDeleteTextures(1, &tex_brdf_lut);

    glDeleteVertexArrays(1, &vao_inverted_cube);
    glDeleteBuffers(1, &vbo_inverted_cube_vertices);

    glDeleteVertexArrays(1, &vao_screen_triangle);
    glDeleteBuffers(1, &vbo_screen_triangle_vertices);
}

RHSHARED<gpuTexture2d> getDefaultTexture(const char* name) {
    auto it = default_textures.find(name);
    if (it != default_textures.end()) {
        return it->second;
    }
    return RHSHARED<gpuTexture2d>();
}
