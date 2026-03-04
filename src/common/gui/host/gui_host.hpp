#pragma once

#include "gui/gui_hit.hpp"
#include "gui/elements/element.hpp"


class GuiHost {
    struct MSG_INTERNAL {
        MSG_INTERNAL()
            : target(0), msg(GUI_MSG::UNKNOWN), params(GUI_MSG_PARAMS()) {}
        MSG_INTERNAL(GuiElement* target, const GUI_MSG& msg, const GUI_MSG_PARAMS& params)
            : target(target), msg(msg), params(params) {}

        GuiElement* target;
        GUI_MSG msg;
        GUI_MSG_PARAMS params;
    };

    static const int MSG_QUEUE_LENGTH = 1024;
    MSG_INTERNAL msg_queue[MSG_QUEUE_LENGTH];
    int msgq_write_cur = 0;
    int msgq_read_cur = 0;
    int msgq_new_message_count = 0;

    bool hasMsg() {
        return msgq_new_message_count > 0;
    }
    MSG_INTERNAL readMsg() {
        if (msgq_new_message_count == 0) {
            return MSG_INTERNAL(0, GUI_MSG::UNKNOWN, GUI_MSG_PARAMS());
        }
        MSG_INTERNAL msg = msg_queue[msgq_read_cur];
        msgq_read_cur = (msgq_read_cur + 1) % MSG_QUEUE_LENGTH;
        --msgq_new_message_count;
        return msg;
    }
    void processMessage(GuiElement* target, GUI_MSG msg, GUI_MSG_PARAMS params);
public:
    virtual ~GuiHost() {}

    virtual bool isMouseCaptured() = 0;

    virtual void insert(GuiElement*) = 0;
    virtual void remove(GuiElement*) = 0;

    virtual void hitTest(GuiHitResult& hit, int x, int y) = 0;
    virtual bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) = 0;
    virtual void layout(const gfxm::vec2&) = 0;
    virtual void draw() = 0;

    void postMsg(GuiElement* target, GUI_MSG msg, GUI_MSG_PARAMS params);
    void pollMessages();
};