#pragma once


struct FILE_TRACKER_HANDLE;

FILE_TRACKER_HANDLE*    fileTrackerInit(const char* dir);
void                    fileTrackerCleanup(FILE_TRACKER_HANDLE* h);
void                    fileTrackerUpdate(FILE_TRACKER_HANDLE* h);


class FileTracker {
    FILE_TRACKER_HANDLE* h = nullptr;

public:
    FileTracker(const char* dir) {
        h = fileTrackerInit(dir);
    }
    ~FileTracker() {
        fileTrackerCleanup(h);
    }

    void update() {
        fileTrackerUpdate(h);
    }
};

