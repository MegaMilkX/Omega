#pragma once

#include <string>
#include "spawnable/spawnable.hpp"
#include "scene.hpp"


class SceneManager : public ISpawnable {
    std::unique_ptr<IScene> active_scene;
public:
    ~SceneManager() {
        tryDespawn();
    }
    void onSpawn(WorldSystemRegistry& reg) override {
        if (active_scene) {
            active_scene->onSpawnScene(reg);
        }
    }
    void onDespawn(WorldSystemRegistry& reg) override {
        if (active_scene) {
            active_scene->onDespawnScene(reg);
        }
    }

    void clearScene() {
        if (!active_scene) {
            return;
        }
        if (isSpawned()) {
            active_scene->onDespawnScene(*getRegistry());
        }
        active_scene.reset();
    }

    template<typename T>
    void createScene() {
        static_assert(std::is_base_of_v<IScene, T>, "T must derive from IScene");
        
        clearScene();

        auto ptr = new T();
        active_scene.reset(ptr);

        if (isSpawned()) {
            active_scene->onSpawnScene(*getRegistry());
        }
    }
    template<typename T>
    void loadScene(const std::string& path) {        
        static_assert(std::is_base_of_v<IScene, T>, "T must derive from IScene");
        
        clearScene();

        auto ptr = new T();
        active_scene.reset(ptr);

        active_scene->load(path);

        if (isSpawned()) {
            active_scene->onSpawnScene(*getRegistry());
        }
    }
};

