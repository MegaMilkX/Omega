#pragma once

#include <stdint.h>
#include "reflection/reflection.hpp"


class gameWorld;

struct wMsg {
    type t;
    uint64_t payload[4];
};
struct wRsp {
    type t;
    uint64_t payload[4];

    wRsp() {}
    wRsp(int i) {}
};

class gameActor;
struct wMsgInteract {
    gameActor* sender;
};
struct wmsgMissileSpawn {
    gfxm::vec3 pos;
    gfxm::vec3 dir;
};
struct wRspInteractDoorOpen {
    gfxm::vec3 sync_pos;
    gfxm::quat sync_rot;
    bool is_front;
};
struct wRspInteractJukebox {};

template<typename T>
wMsg wMsgMake(const T& msg) {
    static_assert(sizeof(T) <= sizeof(wMsg::payload), "");
    wMsg w_msg;
    memcpy(w_msg.payload, &msg, gfxm::_min(sizeof(w_msg.payload), sizeof(msg)));
    w_msg.t = type_get<T>();
    return w_msg;
}
template<typename T>
const T* wMsgTranslate(const wMsg& msg) {
    if (type_get<T>() != msg.t) {
        return 0;
    }
    return (const T*)&msg.payload[0];
}

template<typename T>
wRsp wRspMake(const T& rsp) {
    static_assert(sizeof(T) <= sizeof(wRsp::payload), "");
    wRsp w_rsp;
    memcpy(w_rsp.payload, &rsp, gfxm::_min(sizeof(w_rsp.payload), sizeof(rsp)));
    w_rsp.t = type_get<T>();
    return w_rsp;
}
template<typename T>
const T* wRspTranslate(const wRsp& rsp) {
    if (type_get<T>() != rsp.t) {
        return 0;
    }
    return (const T*)&rsp.payload[0];
}


struct MSG_MESSAGE {
    int id;
    uint64_t payload[4];

    MSG_MESSAGE() : id(0) {}
    template<typename PAYLOAD_T>
    void make(int msg_id, const PAYLOAD_T& payload) {
        static_assert(sizeof(PAYLOAD_T) <= sizeof(payload), "payload size too big");
        id = msg_id;
        memcpy(&this->payload[0], &payload, sizeof(payload));
    }

    template<typename PAYLOAD_T>
    const PAYLOAD_T* getPayload() const {
        static_assert(sizeof(PAYLOAD_T) <= sizeof(payload), "getPayload(): payload size too big");
        return (const PAYLOAD_T*)&payload[0];
    }
};

enum MESSAGE_ID {
    MSGID_INTERACT,
    MSGID_EXPLOSION,
    MSGID_MISSILE_SPAWN
};

struct MSGPLD_INTERACT {
    gameActor* sender;
};
struct MSGPLD_EXPLOSION {
    gfxm::vec3 translation;
};
struct MSGPLD_MISSILE_SPAWN {
    gfxm::vec3 translation;
    gfxm::quat orientation;
};