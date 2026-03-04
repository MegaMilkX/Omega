#include "gui_host.hpp"



void GuiHost::processMessage(GuiElement* target, GUI_MSG msg, GUI_MSG_PARAMS params) {
    switch (msg) {
    default:
        if (target) {
            target->sendMessage(msg, params);
        } else {
            onMessage(msg, params);
        }
        break;
    case GUI_MSG::MOUSE_SCROLL:
        // TODO:
        /*if (hovered_elem) {
            hovered_elem->sendMessage(msg, params);
        }*/
        break;
    
    }
}

void GuiHost::postMsg(GuiElement* target, GUI_MSG msg, GUI_MSG_PARAMS params) {
    assert(msgq_new_message_count < MSG_QUEUE_LENGTH - 1);
    msg_queue[msgq_write_cur] = MSG_INTERNAL(target, msg, params);
    msgq_write_cur = (msgq_write_cur + 1) % MSG_QUEUE_LENGTH;
    if (msgq_new_message_count < MSG_QUEUE_LENGTH - 1) {
        ++msgq_new_message_count;
    }
}

void GuiHost::pollMessages() {
    while (hasMsg()) {
        auto msg_internal = readMsg();
        auto msg = msg_internal.msg;
        auto params = msg_internal.params;
        auto target = msg_internal.target;

        if (target) {
            target->sendMessage(msg, params);
        } else {
            onMessage(msg, params);
        }
        // TODO: ?
        //processMessage(target, msg, params);
    }
}

