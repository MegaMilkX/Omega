#include "common/render/render.hpp"

#include "platform/platform.hpp"
#include "common/render/render_bucket.hpp"

static gpuPipeline* s_pipeline = 0;
static RenderBucket* s_renderBucket = 0;

bool gpuInit(gpuPipeline* pp) {
    s_pipeline = pp;
    s_renderBucket = new RenderBucket(pp, 10000);
    return true;
}
void gpuCleanup() {
    delete s_renderBucket;
}

gpuPipeline* gpuGetPipeline() {
    return s_pipeline;
}


void gpuDrawRenderable(gpuRenderable* r) {
    s_renderBucket->add(r);
}

void gpuClearQueue() {
    s_renderBucket->clear();
}


void drawPass(gpuPipeline* pipe, RenderBucket* bucket, const char* technique_name, int pass) {    
    int screen_w = 0, screen_h = 0;
    platformGetWindowSize(screen_w, screen_h);
    
    auto pipe_tech = pipe->findTechnique(technique_name);
    auto pipe_pass = pipe_tech->getPass(pass);
    pipe_pass->bindFrameBuffer();
    glViewport(0, 0, screen_w, screen_h);
    glScissor(0, 0, screen_w, screen_h);

    auto group = bucket->getTechniqueGroup(pipe_tech->getId());
    for (int i = group.start; i < group.end;) { // all commands of the same technique
        auto& cmd = bucket->commands[i];
        int material_end = cmd.next_material_id;

        const gpuRenderMaterial* material = cmd.renderable->getMaterial();
        material->bindSamplers();
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
            default:
                assert(false);
            }
            pass->bindDrawBuffers();
            pass->bindShaderProgram();

            int pass_end = cmd.next_pass_id;
            for (; i < pass_end; ++i) { // iterate over commands with the same shader(pass)
                auto& cmd = bucket->commands[i];
                cmd.renderable->bindUniformBuffers();
                gpuUseMeshBinding(cmd.binding);
                cmd.renderable->getMeshDesc()->_draw();
            }
        }
    }
};

void gpuDraw() {
    glDisable(GL_CULL_FACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glDisable(GL_LINE_SMOOTH);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //glClearColor(0.129f, 0.586f, 0.949f, 1.0f);
    glClearColor(0.f, 0.f, 0.f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    s_renderBucket->sort();
    s_pipeline->bindUniformBuffers();
    
    GLuint gvao;
    glGenVertexArrays(1, &gvao);
    glBindVertexArray(gvao);

    drawPass(s_pipeline, s_renderBucket, "Normal", 0);
    drawPass(s_pipeline, s_renderBucket, "Decals", 0);
    drawPass(s_pipeline, s_renderBucket, "Debug", 0);
    drawPass(s_pipeline, s_renderBucket, "GUI", 0);

    glDeleteVertexArrays(1, &gvao);
}