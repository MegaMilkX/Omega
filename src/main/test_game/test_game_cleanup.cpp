
#include "test_game.hpp"

void TestGame::cleanup() {
    gameuiCleanup();

    GameBase::cleanup();
}