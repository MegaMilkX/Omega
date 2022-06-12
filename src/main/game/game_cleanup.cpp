
#include "game_common.hpp"

void GameCommon::Cleanup() {
    guiCleanup();

    mdlCleanup();
    gpuCleanup();

    delete shader_default;
}