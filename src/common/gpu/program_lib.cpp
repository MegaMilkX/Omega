#include "program_lib.hpp"


struct ShaderProgramNode {
    std::unordered_map<const gpuCompiledShader*, std::unique_ptr<ShaderProgramNode>> children;
    RHSHARED<gpuShaderProgram> prog;

    ShaderProgramNode* walk(const gpuCompiledShader* next) {
        auto it = children.find(next);
        if (it == children.end()) {
            it = children.insert(
                std::make_pair(
                    next,
                    std::unique_ptr<ShaderProgramNode>(new ShaderProgramNode)
                )
            ).first;
        }
        return it->second.get();
    }
};
static ShaderProgramNode root_program_node;


RHSHARED<gpuShaderProgram> gpuGetProgram(const gpuCompiledShader** shaders, int count) {
    std::vector<const gpuCompiledShader*> sorted(shaders, shaders + count);
    std::sort(sorted.begin(), sorted.end());

    ShaderProgramNode* current = &root_program_node;
    for (int i = 0; i < sorted.size(); ++i) {
        current = current->walk(sorted[i]);
    }

    if (!current->prog) {
        current->prog.reset_acquire();
        auto prog = current->prog;
        for (int i = 0; i < sorted.size(); ++i) {
            prog->addShader(sorted[i]);
        }
        if (!prog->init_2()) {
            return RHSHARED<gpuShaderProgram>();
        }
    }

    return current->prog;
}

