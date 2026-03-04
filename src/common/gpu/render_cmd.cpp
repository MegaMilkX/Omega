#include "render_cmd.hpp"

#include <assert.h>
#include "platform/gl/glextutil.h"


void gpuSetModes(const gpuRenderCmd& cmd) {
    auto rdr_pass = cmd.rdr_pass;
    (rdr_pass->draw_flags & GPU_DEPTH_TEST) ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
    (rdr_pass->draw_flags & GPU_STENCIL_TEST) ? glEnable(GL_STENCIL_TEST) : glDisable(GL_STENCIL_TEST);
    (rdr_pass->draw_flags & GPU_BACKFACE_CULLING) ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
    (rdr_pass->draw_flags & GPU_DEPTH_WRITE) ? glDepthMask(GL_TRUE) : glDepthMask(GL_FALSE);
}
void gpuSetBlending(GPU_BLEND_MODE mode) {
    //glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    switch (mode) {
    case GPU_BLEND_MODE::BLEND:
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        break;
    case GPU_BLEND_MODE::ADD:
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        break;
    case GPU_BLEND_MODE::MULTIPLY:
        glBlendFunc(GL_DST_COLOR, GL_ZERO);
        break;
    case GPU_BLEND_MODE::OVERWRITE:
        glBlendFunc(GL_ONE, GL_ZERO);
        break;
    default:
        assert(false);
    }
}
void gpuSetBlending(const gpuRenderCmd& cmd) {
    gpuSetBlending(cmd.rdr_pass->blend_mode);
}

void gpuBindDrawBuffers(const gpuRenderCmd& cmd) {
    glDrawBuffers(GPU_FRAME_BUFFER_MAX_DRAW_COLOR_BUFFERS, cmd.rdr_pass->gl_draw_buffers);
}
void gpuBindProgram(const gpuRenderCmd& cmd) {
    GLuint p = cmd.program;
    glUseProgram(p);
}

