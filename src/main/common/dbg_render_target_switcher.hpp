#pragma once

#include "input/input.hpp"
#include "log/log.hpp"
#include "gpu/gpu_render_target.hpp"


class DbgRenderTargetSwitcher {
    InputContext input_ctx = InputContext("DbgRenderTargetSwitcher");
    InputAction* inputFButtons[12] = { 0 };
public:
    DbgRenderTargetSwitcher(InputState* input_state) {
        input_state->pushContext(&input_ctx);
        for (int i = 0; i < 12; ++i) {
            inputFButtons[i] = input_ctx.createAction(MKSTR("F" << (i + 1)).c_str());
        }
    }
    ~DbgRenderTargetSwitcher() {
        input_ctx.remove();
    }
    void update(gpuRenderTarget* render_target) {
        if (inputFButtons[0]->isJustPressed()) {
            render_target->setDefaultOutput("Final", RT_OUTPUT_RGB);
        } else if (inputFButtons[1]->isJustPressed()) {
            render_target->setDefaultOutput("Albedo", RT_OUTPUT_RGB);
        } else if (inputFButtons[2]->isJustPressed()) {
            render_target->setDefaultOutput("Position", RT_OUTPUT_RGB);
        } else if (inputFButtons[3]->isJustPressed()) {
            render_target->setDefaultOutput("Normal", RT_OUTPUT_RGB);
        } else if (inputFButtons[4]->isJustPressed()) {
            render_target->setDefaultOutput("Metalness", RT_OUTPUT_RRR);
        } else if (inputFButtons[5]->isJustPressed()) {
            render_target->setDefaultOutput("Roughness", RT_OUTPUT_RRR);
        } else if (inputFButtons[6]->isJustPressed()) {
            render_target->setDefaultOutput("VelocityMap", RT_OUTPUT_RGB);
        } else if (inputFButtons[7]->isJustPressed()) {
            render_target->setDefaultOutput("Lightness", RT_OUTPUT_RGB);
        } else if (inputFButtons[8]->isJustPressed()) {
            render_target->setDefaultOutput("Depth", RT_OUTPUT_DEPTH);
        } else if (inputFButtons[10]->isJustPressed()) {
            render_target->setDefaultOutput("AmbientOcclusion", RT_OUTPUT_RRR);
        }
    }
};