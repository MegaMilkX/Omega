#include "test_game.hpp"



void TestGameInstance::onPlayerJoined(IPlayer* player) {
    LOG("IGameInstance: onPlayerJoined");
    LocalPlayer* local = dynamic_cast<LocalPlayer*>(player);
    if (!local) {
        return;
    }
    assert(local->getViewport());
}
void TestGameInstance::onPlayerLeft(IPlayer* player) {
    LOG("IGameInstance: onPlayerLeft");
    LocalPlayer* local = dynamic_cast<LocalPlayer*>(player);
    if (!local) {
        return;
    }
    assert(local->getViewport());
}

