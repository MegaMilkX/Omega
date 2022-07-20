#pragma once

#include "math/gfxm.hpp"


typedef void(*platform_window_resize_cb_t)(int, int);


int platformInit(bool show_window = true);
void platformCleanup();

bool platformIsRunning();
void platformPollMessages();

void platformSwapBuffers();


void platformGetWindowSize(int& w, int &h);
const gfxm::rect& platformGetViewportRect();
void platformSetWindowResizeCallback(platform_window_resize_cb_t cb);


void platformGetMousePos(int* x, int* y);