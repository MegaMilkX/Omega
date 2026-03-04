
#include "test_game.hpp"

void TestGameInstance::onCleanup() {
    gameuiCleanup();
    
    world.reset();
}