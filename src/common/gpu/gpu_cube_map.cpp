#include "gpu_cube_map.hpp"

#include "gpu_shader_program.hpp"
#include "gpu.hpp"

#include "gpu/util_shader.hpp"

static void cubemapInit(GLuint id, int width, int height, GLint internalFormat) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, id);
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

gpuCubeMap::gpuCubeMap() {
    GL_CHECK(0);
    glGenTextures(1, &id);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, id);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    GL_CHECK(0);
}
gpuCubeMap::~gpuCubeMap() {
    glDeleteTextures(1, &id);
}
void gpuCubeMap::reserve(int side, GLint internal_format, GLenum format, GLenum type) {
    GL_CHECK(0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, id);
    for (int i = 0; i < 6; ++i) {
        glTexImage2D(
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
            0, internal_format, side, side, 0, format, type,
            0
        );
    }
    
    // TODO:
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    GL_CHECK(0);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_FUNC, GL_LESS);
    GL_CHECK(0);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    GL_CHECK(0);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GL_CHECK(0);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    GL_CHECK(0);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    GL_CHECK(0);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    GL_CHECK(0);
    /*
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    */
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}
void gpuCubeMap::setData(const ktImage* image) {
    const int side = 512;
        
    gpuTexture2d tex;
    tex.changeFormat(GL_RGB16F, 0, 0, 3, GL_FLOAT);
    tex.setData(image);
    GLuint tex_id = tex.getId();

    auto prog_hdri_to_cubemap = loadUtilShader("core/shaders/ibl/hdri_to_cubemap.glsl");
    cubemapInit(id, 512, 512, GL_RGB16F);
    cubemapFromHdri(vao_inverted_cube, prog_hdri_to_cubemap, tex_id, id, 512, 512);
}

void gpuCubeMap::build(
    const ktImage* posx,
    const ktImage* negx,
    const ktImage* posy,
    const ktImage* negy,
    const ktImage* posz,
    const ktImage* negz
) {
    const ktImage* faces[] = { posx, negx, posy, negy, posz, negz };
    glBindTexture(GL_TEXTURE_CUBE_MAP, id);
    for (int i = 0; i < 6; ++i) {
        // TODO: Handle different formats
        glTexImage2D(
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
            0, GL_RGBA, faces[i]->getWidth(), faces[i]->getHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE,
            faces[i]->getData()
        );
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

bool gpuCubeMap::load(byte_reader& in) {
    auto view = in.try_slurp();
    if (!view) {
        return false;
    }

    ktImage img;
    bool ret = loadImagef(&img, view.data, view.size);
    if (!ret) {
        assert(false);
        return false;
    }
    setData(&img);
    return true;
}

