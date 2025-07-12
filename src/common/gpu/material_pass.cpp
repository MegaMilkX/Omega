#include "material_pass.hpp"


gpuMaterialPass::gpuMaterialPass(const char* path)
    : path(path),
    depth_test(1),
    stencil_test(0),
    cull_faces(1),
    depth_write(1)
{}

int gpuMaterialPass::getPipelineIdx() const {
    return pipeline_idx;
}
const std::string& gpuMaterialPass::getPath() const {
    return path;
}

const ShaderSamplerSet& gpuMaterialPass::getSamplerSet() const {
    return sampler_set;
}

ktRenderPassParam* gpuMaterialPass::addParam(const char* name) {
    params.push_back(std::unique_ptr<ktRenderPassParam>(new ktRenderPassParam(name)));
    if(prog) {
        GLint loc = glGetUniformLocation(prog->getId(), params.back()->getName());
        params.back()->setLocation(loc);
    }
    return params.back().get();
}

void gpuMaterialPass::setShaderProgram(HSHARED<gpuShaderProgram> p) {
    prog = p;
    shaderInterface = SHADER_INTERFACE_GENERIC(p.get());
    for(int i = 0; i < params.size(); ++i) {
        GLint loc = glGetUniformLocation(prog->getId(), params[i]->getName());
        params[i]->setLocation(loc);
    }
}

gpuShaderProgram* gpuMaterialPass::getShaderProgram() { 
    return prog.get();
}
HSHARED<gpuShaderProgram>& gpuMaterialPass::getShaderProgramHandle() {
    return prog;
}

void gpuMaterialPass::bindDrawBuffers() {
    glDrawBuffers(GPU_FRAME_BUFFER_MAX_DRAW_COLOR_BUFFERS, gl_draw_buffers);
}
void gpuMaterialPass::bindShaderProgram() {
    glUseProgram(prog->getId());
    for (int i = 0; i < params.size(); ++i) {
        auto& p = params[i];
        p->upload();
    }
}
void gpuMaterialPass::bind() {
    assert(prog);
    bindDrawBuffers();
    bindShaderProgram();
}

