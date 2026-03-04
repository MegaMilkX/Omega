#pragma once

#include <memory>
#include "input/input.hpp"
#include "viewport/viewport.hpp"
#include "player_agent.hpp"
#include "world/world_system_registry.hpp"
#include "world/agent/agent.hpp"

class Actor;

class IPlayer {
    std::string name = "Player";
    std::unique_ptr<InputState> input_state;

    std::vector<std::unique_ptr<IPlayerProxy>> roles;
protected:
    void setInputState(InputState* state) {
        input_state.reset(state);
    }
public:
    virtual ~IPlayer();

    void setName(const std::string& name) { this->name = name; }
    const std::string& getName() const { return name; }

    InputState* getInputState() {
        return input_state.get();
    }
    virtual EngineRenderView* getViewport() { return nullptr; }

    void clearRoles() {
        for (auto& r : roles) {
            if (r->isSpawned()) {
                r->despawn();
            }
            r->setPlayer(nullptr);
        }
        roles.clear();
    }
    template<typename T, typename... ARGS>
    T* addRole(WorldSystemRegistry& reg, ARGS&&... args) {
        static_assert(std::is_base_of_v<IPlayerProxy, T>, "T must be an IPlayerProxy");

        auto role = std::make_unique<T>(std::forward<ARGS>(args)...);
        auto ptr = role.get();

        roles.push_back(std::move(role));
        ptr->setPlayer(this);
        reg.spawn(ptr);

        return ptr;
    }
};

class LocalPlayer : public IPlayer {
    EngineRenderView* viewport = 0;
public:
    LocalPlayer(EngineRenderView* viewport, uint8_t input_id);

    EngineRenderView* getViewport() override { return viewport; }
};

class NetworkPlayer : public IPlayer {
public:
};

class AiPlayer : public IPlayer {
public:
};

class ReplayPlayer : public IPlayer {
public:
};


IPlayer*    playerGetPrimary();
void        playerSetPrimary(IPlayer* player);
void        playerAdd(IPlayer* player);
void        playerRemove(IPlayer* player);
int         playerCount();
IPlayer*    playerGet(int i);


[[cppi_decl, no_reflect]];
class IPlayerListener;

class IPlayerListener {
public:
    virtual ~IPlayerListener() {}
    virtual void onPlayerJoined(IPlayer*) = 0;
    virtual void onPlayerLeft(IPlayer*) = 0;
};

void        playerAddListener(IPlayerListener*);
void        playerRemoveListener(IPlayerListener*);

