#pragma once

#include "actor_controller.hpp"


#include "input/input.hpp"
class ctrlCharacterPlayerInput : public ActorController {
    InputRange* rangeTranslation = 0;
    InputAction* actionInteract = 0;
public:
    ctrlCharacterPlayerInput() {
        rangeTranslation = inputGetRange("Character", "Locomotion");
        actionInteract = inputGetAction("Character", "Interact");
    }
    void onReset() override {}
    void onSpawn(gameActor* actor) override {}
    void onDespawn(gameActor* actor) override {}
    void onActorNodeRegister(type t, gameActorNode* component, const std::string& name) override {}
    void onActorNodeUnregister(type t, gameActorNode* component, const std::string& name) override {}
    void onUpdate(gameWorld* world, gameActor* actor, float dt) override {
        // TODO
    }
};


class wFsmController;
class wFsmControllerState {
public:
    virtual ~wFsmControllerState() {}

    virtual void onReset() = 0;
    virtual void onActorNodeRegister(type t, gameActorNode* component, const std::string& name) = 0;

    virtual void onEnter() {}
    virtual void onUpdate(gameWorld* world, gameActor* actor, wFsmController* fsm, float dt) = 0;
};
class wFsmController : public ActorController {
    wFsmControllerState* initial_state = 0;
    wFsmControllerState* current_state = 0;
    wFsmControllerState* next_state = 0;
    std::map<std::string, std::unique_ptr<wFsmControllerState>> states;
public:
    void addState(const char* name, wFsmControllerState* state) {
        auto it = states.find(name);
        if (it != states.end()) {
            assert(false);
            LOG_ERR("State " << name << " already exists");
            return;
        }
        if (states.empty()) {
            initial_state = state;
            next_state = initial_state;
        }
        states.insert(std::make_pair(std::string(name), std::unique_ptr<wFsmControllerState>(state)));
    }

    void setState(const char* name) {
        auto it = states.find(name);
        if (it == states.end()) {
            assert(false);
            LOG_ERR("State " << name << " does not exist");
            return;
        }
        next_state = it->second.get();
    }

    void onReset() override {
        next_state = initial_state;
        for (auto& kv : states) {
            kv.second->onReset();
        }
    }
    void onSpawn(gameActor* actor) override {
        // TODO
    }
    void onDespawn(gameActor* actor) override {
        // TODO
    }
    void onActorNodeRegister(type t, gameActorNode* component, const std::string& name) override {
        for (auto& kv : states) {
            kv.second->onActorNodeRegister(t, component, name);
        }
    }
    void onActorNodeUnregister(type t, gameActorNode* component, const std::string& name) override {
        // TODO:?
    }
    void onUpdate(gameWorld* world, gameActor* actor, float dt) override {
        if (next_state) {
            current_state = next_state;
            next_state = 0;
            current_state->onEnter();
        }
        assert(current_state);
        current_state->onUpdate(world, actor, this, dt);
    }
};