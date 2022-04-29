#pragma once


int platformInit();
void platformCleanup();

bool platformIsRunning();
void platformPollMessages();

void platformSwapBuffers();