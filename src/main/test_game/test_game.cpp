#include "test_game.hpp"



void TestGameInstance::onPlayerJoined(IPlayer* player) {
    LOG("IGameInstance: onPlayerJoined");
    LocalPlayer* local = dynamic_cast<LocalPlayer*>(player);
    if (!local) {
        return;
    }
    assert(local->getViewport());
    
    Camera* cam = new Camera;
    world->spawn(cam);
    local->getViewport()->setCamera(cam);
    cam->setZNear(.01f);
    cam->setZFar(1000.f);
}
void TestGameInstance::onPlayerLeft(IPlayer* player) {
    LOG("IGameInstance: onPlayerLeft");
    LocalPlayer* local = dynamic_cast<LocalPlayer*>(player);
    if (!local) {
        return;
    }
    assert(local->getViewport());
    
    auto cam = local->getViewport()->getCamera();
    world->despawn(cam);
    delete cam;
}

