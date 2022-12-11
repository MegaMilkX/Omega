#include "platform/platform.hpp"
#include "input/input.hpp"
#include "typeface/typeface.hpp"
#include "util/timer.hpp"
#include "game/game_common.hpp"
#include "resource/resource.hpp"
#include "animation/animation.hpp"


static bool is_running = true;

void onWindowResize(int width, int height) {
    g_game_comn->onViewportResize(width, height);
}

int main(int argc, char* argv) {
    platformInit();
    resInit();
    typefaceInit();
    animInit();
    std::unique_ptr<build_config::gpuPipelineCommon> gpu_pipeline;
    gpu_pipeline.reset(new build_config::gpuPipelineCommon);
    gpuInit(gpu_pipeline.get()); // !!


    g_game_comn = new GameCommon();
    g_game_comn->Init();

    platformSetWindowResizeCallback(&onWindowResize);

    timer timer_;
    float dt = 1.0f / 60.0f;
    while (platformIsRunning()) {
        timer_.start();
        platformPollMessages();

        inputUpdate(dt);
        g_game_comn->Update(dt);
        g_game_comn->Draw(dt);

        platformSwapBuffers();
        dt = timer_.stop();
    }

    g_game_comn->Cleanup();
    
    
    gpuCleanup();
    gpu_pipeline.reset();
    animCleanup();
    typefaceCleanup();
    resCleanup();
    platformCleanup();
    return 0;
}