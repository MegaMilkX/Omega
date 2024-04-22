#pragma once


constexpr int GAME_MSG_MAX_PAYLOAD_SIZE_BYTES = 32;

enum class GAME_MSG {
    UNKNOWN,
    NOT_HANDLED,
    HANDLED,
    PLAYER_ATTACH,
    PLAYER_DETACH,

    INTERACT,
    RESPONSE_DOOR_OPEN,
    EXPLOSION,
    MISSILE_SPAWN
};

template<GAME_MSG msg>
struct PAYLOAD_FOR_MSG {
    using type = void;
};
template<typename PAYLOAD>
struct MSG_FOR_PAYLOAD {
    constexpr static GAME_MSG msg = GAME_MSG::UNKNOWN;
};

struct GAME_MESSAGE {
    GAME_MSG msg = GAME_MSG::UNKNOWN;
    char payload[GAME_MSG_MAX_PAYLOAD_SIZE_BYTES] = { 0 };

    GAME_MESSAGE() {}
    GAME_MESSAGE(GAME_MSG msg)
        : msg(msg) {}

    template<GAME_MSG MSG>
    typename PAYLOAD_FOR_MSG<MSG>::type& getPayload() {
        return *(typename PAYLOAD_FOR_MSG<MSG>::type*)payload;
    }
};

template<typename PAYLOAD_T>
inline GAME_MESSAGE makeGameMessage(PAYLOAD_T payload) {
    GAME_MESSAGE msg;
    msg.msg = MSG_FOR_PAYLOAD<PAYLOAD_T>::msg;
    memcpy(msg.payload, &payload, sizeof(payload));
    return msg;
}

template<typename T>
struct PAYLOAD_SIZE_CHECKER {
    constexpr static bool value = sizeof(T) <= GAME_MSG_MAX_PAYLOAD_SIZE_BYTES;
};
template<>
struct PAYLOAD_SIZE_CHECKER<void> {
    constexpr static bool value = true;
};

#define DEFINE_MSG_PAYLOAD(MSG, PAYLOAD_T) \
static_assert(PAYLOAD_SIZE_CHECKER<PAYLOAD_T>::value, "message payload size too big"); \
template<> struct PAYLOAD_FOR_MSG<MSG> { using type = PAYLOAD_T; }; \
template<> struct MSG_FOR_PAYLOAD<PAYLOAD_T> { constexpr static GAME_MSG msg = MSG; };


class IPlayer;
struct PAYLOAD_PLAYER_ATTACH {
    IPlayer* player;
};
struct PAYLOAD_PLAYER_DETACH {
    IPlayer* player;
};
#include "math/gfxm.hpp"
struct PAYLOAD_MISSILE_SPAWN {
    gfxm::vec3 pos;
    gfxm::vec3 dir;
};
class Actor;
struct PAYLOAD_INTERACT {
    Actor* sender;
};
struct PAYLOAD_RESPONSE_DOOR_OPEN {
    gfxm::vec3 sync_pos;
    gfxm::quat sync_rot;
    bool is_front;
};

DEFINE_MSG_PAYLOAD(GAME_MSG::PLAYER_ATTACH, PAYLOAD_PLAYER_ATTACH);
DEFINE_MSG_PAYLOAD(GAME_MSG::PLAYER_DETACH, PAYLOAD_PLAYER_DETACH);
DEFINE_MSG_PAYLOAD(GAME_MSG::INTERACT, PAYLOAD_INTERACT);
DEFINE_MSG_PAYLOAD(GAME_MSG::MISSILE_SPAWN, PAYLOAD_MISSILE_SPAWN);
DEFINE_MSG_PAYLOAD(GAME_MSG::RESPONSE_DOOR_OPEN, PAYLOAD_RESPONSE_DOOR_OPEN);
