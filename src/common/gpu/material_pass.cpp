#include "material_pass.hpp"


gpuMaterialPass::gpuMaterialPass(const char* path)
    : path(path),
    depth_test(1),
    stencil_test(0),
    cull_faces(1),
    depth_write(1)
{}

pipe_pass_id_t gpuMaterialPass::getPipelineIdx() const {
    return pipeline_idx;
}
const std::string& gpuMaterialPass::getPath() const {
    return path;
}

const ShaderSamplerSet& gpuMaterialPass::getSamplerSet() const {
    return sampler_set;
}

void gpuMaterialPass::setShaderProgram(HSHARED<gpuShaderProgram> p) {
    prog = p;
    shaderInterface = SHADER_INTERFACE_GENERIC(p.get());
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
}
void gpuMaterialPass::bind() {
    assert(prog);
    bindDrawBuffers();
    bindShaderProgram();
}

