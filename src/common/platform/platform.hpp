#pragma once

#include "math/gfxm.hpp"

typedef void(*platform_window_resize_cb_t)(int, int);


enum PLATFORM_PARAM {
    PLATFORM_MAX_ATTRIB_COUNT,
    PLATFORM_MAX_UNIFORM_BUFFER_BINDINGS,
    PLATFORM_MAX_TEXTURE_BUFFER_SZ,
    PLATFORM_MAX_FRAMEBUFFER_COLOR_LAYERS,  // GL_MAX_COLOR_ATTACHMENTS
    PLATFORM_MAX_COLOR_OUTPUTS              // GL_MAX_DRAW_BUFFERS
};


int platformInit(bool show_window = true, bool tooling_gui_enabled = false);
void platformCleanup();

bool platformIsRunning();
void platformPollMessages();

void platformSwapBuffers();
void platformRestoreContext();


void platformGetWindowSize(int& w, int &h);
const gfxm::rect& platformGetViewportRect();
void platformSetWindowResizeCallback(platform_window_resize_cb_t cb);


void platformGetMousePos(int* x, int* y);

void platformPushMouseState(bool lock, bool hide);
void platformPopMouseState();
void platformLockMouse(bool lock);
void platformHideMouse(bool hide);
void platfromCaptureMouse();
void platformReleaseMouse();

int platformGeti(PLATFORM_PARAM param);
