#pragma once

#include "math/gfxm.hpp"

typedef void(*platform_window_resize_cb_t)(int, int);


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

void platformLockMouse(bool lock);
void platformHideMouse(bool hide);
void platfromCaptureMouse();
void platformReleaseMouse();