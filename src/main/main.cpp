#include "platform/platform.hpp"
#include "common/input/input.hpp"
#include "common/typeface/typeface.hpp"
#include "common/util/timer.hpp"
#include "game/game_common.hpp"



static bool is_running = true;

void onWindowResize(int width, int height) {
    g_game_comn->onViewportResize(width, height);
}

int main(int argc, char* argv) {
    platformInit();
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
    platformCleanup();
    return 0;
}