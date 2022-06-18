#include "platform/platform.hpp"
#include "common/input/input.hpp"
#include "common/typeface/typeface.hpp"
#include "common/util/timer.hpp"
#include "game/game_common.hpp"
#include "resource/resource.hpp"
#include "game/resource/res_cache_shader_program.hpp"
#include "game/resource/res_cache_gpu_material.hpp"
#include "game/resource/res_cache_texture_2d.hpp"


static bool is_running = true;

void onWindowResize(int width, int height) {
    g_game_comn->onViewportResize(width, height);
}

int main(int argc, char* argv) {
    platformInit();

    resInit();
    resAddCache<gpuShaderProgram>(new resCacheShaderProgram);
    resAddCache<gpuTexture2d>(new resCacheTexture2d);

    typefaceInit();


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
    typefaceCleanup();
    resCleanup();
    platformCleanup();
    return 0;
}