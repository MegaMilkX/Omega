#pragma once

#include <stdint.h>


enum ePawnCommand : uint32_t {
    ePawnMoveDirection,
    ePawnLookOffset,
    ePawnJump,
    ePawnInteract,
    ePawnGrab,
    ePawnGrabRelease,
    ePawnGrabScroll,
    ePawnThrow,
};
