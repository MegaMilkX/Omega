#include "ibl_maps.hpp"
#include "log/log.hpp"
#include "stb_image.h"
#include "filesystem/filesystem.hpp"
#include "gpu/common_resources.hpp"
#include "gpu/util_shader.hpp"

static GLuint cubemapCreate(int width, int height, GLint internalFormat) {
    GLuint tex;
    glGenTextures(1, &tex);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, tex);
    for (int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat, width, height, 0, GL_RGB, GL_FLOAT, 0);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_LOD, 0);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LOD, 0);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    //glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    return tex;
}

static GLuint createSpecularCubeMap(int width, int height, GLint internalFormat) {
    GLuint tex;
    glGenTextures(1, &tex);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, tex);
    for (int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat, width, height, 0, GL_RGB, GL_FLOAT, 0);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, 4);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_LOD, 0);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LOD, 4);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    //glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    return tex;
}

static void cubemapFromHdri(GLuint vao_cube, GLuint progid, GLuint tex_hdri, GLuint cubemap_out, int width, int height) {
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    GLenum draw_buffers[] = {
        GL_COLOR_ATTACHMENT0
    };
    glDrawBuffers(1, draw_buffers);

    glActiveTexture(GL_TEXTURE0 + 12);
    glBindTexture(GL_TEXTURE_2D, tex_hdri);
    glViewport(0, 0, width, height);
    glBindVertexArray(vao_cube);
    //glFrontFace(GL_CW);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glEnable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(progid);
    gfxm::mat4 views[6] = {
        gfxm::lookAt(gfxm::vec3(.0f, .0f, .0f), gfxm::vec3( 1.f,  .0f,  .0f), gfxm::vec3(.0f, -1.f,  .0f)),
        gfxm::lookAt(gfxm::vec3(.0f, .0f, .0f), gfxm::vec3(-1.f,  .0f,  .0f), gfxm::vec3(.0f, -1.f,  .0f)),
        gfxm::lookAt(gfxm::vec3(.0f, .0f, .0f), gfxm::vec3( 0.f,  1.f,  .0f), gfxm::vec3(.0f,  .0f,  1.f)),
        gfxm::lookAt(gfxm::vec3(.0f, .0f, .0f), gfxm::vec3( 0.f, -1.f,  .0f), gfxm::vec3(.0f,  .0f, -1.f)),
        gfxm::lookAt(gfxm::vec3(.0f, .0f, .0f), gfxm::vec3( 0.f,  .0f,  1.f), gfxm::vec3(.0f, -1.f,  .0f)),
        gfxm::lookAt(gfxm::vec3(.0f, .0f, .0f), gfxm::vec3( 0.f,  .0f, -1.f), gfxm::vec3(.0f, -1.f,  .0f)),
    };
    gfxm::mat4 projection = gfxm::perspective(gfxm::radian(90.0f), 1.0f, 0.1f, 10.0f);

    glUniformMatrix4fv(glGetUniformLocation(progid, "matProjection"), 1, GL_FALSE, (float*)&projection);
    for (int i = 0; i < 6; ++i) {
        glUniformMatrix4fv(glGetUniformLocation(progid, "matView"), 1, GL_FALSE, (float*)&views[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, cubemap_out, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(0);
    glBindVertexArray(0);
    glDeleteFramebuffers(1, &fbo);

    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap_out);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

static void cubemapConvolute(GLuint vao_cube, GLuint progid, GLuint cubemap_in, GLuint cubemap_out, int width, int height) {
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    GLenum draw_buffers[] = {
        GL_COLOR_ATTACHMENT0
    };
    glDrawBuffers(1, draw_buffers);

    glActiveTexture(GL_TEXTURE0 + 8);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap_in);
    glViewport(0, 0, width, height);
    glBindVertexArray(vao_cube);
    //glFrontFace(GL_CW);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glEnable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(progid);
    gfxm::mat4 views[6] = {
        gfxm::lookAt(gfxm::vec3(.0f, .0f, .0f), gfxm::vec3( 1.f,  .0f,  .0f), gfxm::vec3(.0f, -1.f,  .0f)),
        gfxm::lookAt(gfxm::vec3(.0f, .0f, .0f), gfxm::vec3(-1.f,  .0f,  .0f), gfxm::vec3(.0f, -1.f,  .0f)),
        gfxm::lookAt(gfxm::vec3(.0f, .0f, .0f), gfxm::vec3( 0.f,  1.f,  .0f), gfxm::vec3(.0f,  .0f,  1.f)),
        gfxm::lookAt(gfxm::vec3(.0f, .0f, .0f), gfxm::vec3( 0.f, -1.f,  .0f), gfxm::vec3(.0f,  .0f, -1.f)),
        gfxm::lookAt(gfxm::vec3(.0f, .0f, .0f), gfxm::vec3( 0.f,  .0f,  1.f), gfxm::vec3(.0f, -1.f,  .0f)),
        gfxm::lookAt(gfxm::vec3(.0f, .0f, .0f), gfxm::vec3( 0.f,  .0f, -1.f), gfxm::vec3(.0f, -1.f,  .0f)),
    };
    gfxm::mat4 projection = gfxm::perspective(gfxm::radian(90.0f), 1.0f, 0.1f, 10.0f);

    glUniformMatrix4fv(glGetUniformLocation(progid, "matProjection"), 1, GL_FALSE, (float*)&projection);
    for (int i = 0; i < 6; ++i) {
        glUniformMatrix4fv(glGetUniformLocation(progid, "matView"), 1, GL_FALSE, (float*)&views[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, cubemap_out, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(0);
    glBindVertexArray(0);
    glDeleteFramebuffers(1, &fbo);
}

static void cubemapPrefilterConvolute(GLuint vao_cube, GLuint progid, GLuint cubemap_in, GLuint cubemap_out, int width, int height) {
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    GLenum draw_buffers[] = {
        GL_COLOR_ATTACHMENT0
    };
    glDrawBuffers(1, draw_buffers);

    glActiveTexture(GL_TEXTURE0 + 8);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap_in);
    glBindVertexArray(vao_cube);
    //glFrontFace(GL_CW);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glEnable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(progid);
    gfxm::mat4 views[6] = {
        gfxm::lookAt(gfxm::vec3(.0f, .0f, .0f), gfxm::vec3( 1.f,  .0f,  .0f), gfxm::vec3(.0f, -1.f,  .0f)),
        gfxm::lookAt(gfxm::vec3(.0f, .0f, .0f), gfxm::vec3(-1.f,  .0f,  .0f), gfxm::vec3(.0f, -1.f,  .0f)),
        gfxm::lookAt(gfxm::vec3(.0f, .0f, .0f), gfxm::vec3( 0.f,  1.f,  .0f), gfxm::vec3(.0f,  .0f,  1.f)),
        gfxm::lookAt(gfxm::vec3(.0f, .0f, .0f), gfxm::vec3( 0.f, -1.f,  .0f), gfxm::vec3(.0f,  .0f, -1.f)),
        gfxm::lookAt(gfxm::vec3(.0f, .0f, .0f), gfxm::vec3( 0.f,  .0f,  1.f), gfxm::vec3(.0f, -1.f,  .0f)),
        gfxm::lookAt(gfxm::vec3(.0f, .0f, .0f), gfxm::vec3( 0.f,  .0f, -1.f), gfxm::vec3(.0f, -1.f,  .0f)),
    };
    gfxm::mat4 projection = gfxm::perspective(gfxm::radian(90.0f), 1.0f, 0.1f, 10.0f);

    glUniformMatrix4fv(glGetUniformLocation(progid, "matProjection"), 1, GL_FALSE, (float*)&projection);
    const int nMipLevels = 5;
    for (int mip = 0; mip < nMipLevels; ++mip) {
        int mipW = width * powf(.5f, mip);
        int mipH = height * powf(.5f, mip);
        glViewport(0, 0, mipW, mipH);

        float roughness = (float)mip / (float)(nMipLevels - 1);
        glUniform1f(glGetUniformLocation(progid, "roughness"), roughness);
        for (int i = 0; i < 6; ++i) {
            glUniformMatrix4fv(glGetUniformLocation(progid, "matView"), 1, GL_FALSE, (float*)&views[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, cubemap_out, mip);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(0);
    glBindVertexArray(0);
    glDeleteFramebuffers(1, &fbo);
}

static bool makeIBLCubemaps(IBLMaps& set, GLuint vao_cube) {
    const int convoluted_width = 32;
    const int convoluted_height = 32;

    auto prog_convolute = loadUtilShader("core/shaders/ibl/convolute_cubemap.glsl");
    set.irradiance = cubemapCreate(convoluted_width, convoluted_height, GL_RGB16F);
    cubemapConvolute(vao_cube, prog_convolute, set.environment, set.irradiance, convoluted_width, convoluted_height);

    auto prog_prefilter_convolute = loadUtilShader("core/shaders/ibl/prefilter_convolute_cubemap.glsl");
    set.specular = createSpecularCubeMap(128, 128, GL_RGB16F);
    cubemapPrefilterConvolute(vao_cube, prog_prefilter_convolute, set.environment, set.specular, 128, 128);
    return true;
}

IBLMaps loadIBLMapsFromHDRI(const char* path) {
    IBLMaps maps;

    GLuint tex_hdri;
    glGenTextures(1, &tex_hdri);

    stbi_set_flip_vertically_on_load(true);
    int width, height, ncomp;
    float* data = stbi_loadf(path, &width, &height, &ncomp, 3);
    if (!data) {
        LOG_ERR("Failed to load hdri '" << path << "'");
        return IBLMaps();
    }

    glBindTexture(GL_TEXTURE_2D, tex_hdri);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);

    auto prog_hdri_to_cubemap = loadUtilShader("core/shaders/ibl/hdri_to_cubemap.glsl");
    maps.environment = cubemapCreate(512, 512, GL_RGB16F);
    cubemapFromHdri(vao_inverted_cube, prog_hdri_to_cubemap, tex_hdri, maps.environment, 512, 512);

    glDeleteTextures(1, &tex_hdri);

    makeIBLCubemaps(maps, vao_inverted_cube);

    return maps;
}

IBLMaps loadIBLMapsFromCubeSides(
    const char* posx, const char* negx,
    const char* posy, const char* negy,
    const char* posz, const char* negz
) {
    IBLMaps maps;

    glGenTextures(1, &maps.environment);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, maps.environment);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    const char* paths[] = {
        posx, negx, posy, negy, posz, negz
    };
    stbi_set_flip_vertically_on_load(false);
    for (int i = 0; i < 6; ++i) {
        int w, h, comp;
        stbi_uc* data = stbi_load(paths[i], &w, &h, &comp, 3);
        glTexImage2D(
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
            0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE,
            data
        );
        stbi_image_free(data);
    }
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    makeIBLCubemaps(maps, vao_inverted_cube);

    return maps;
}