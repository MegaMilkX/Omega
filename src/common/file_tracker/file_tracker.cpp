#include "file_tracker.hpp"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include "Windows.h"

#include <filesystem>
#include "log/log.hpp"


const int CHANGE_BUFFER_SIZE = 16000;

struct FILE_TRACKER_HANDLE {
    std::filesystem::path path;
    uint8_t change_buffer[CHANGE_BUFFER_SIZE];
    HANDLE change_hdir = INVALID_HANDLE_VALUE;
    DWORD change_dwbytes = 0;
    OVERLAPPED change_overlapped = { 0 };
};

FILE_TRACKER_HANDLE* fileTrackerInit(const char* dir) {
    FILE_TRACKER_HANDLE* tracker = new FILE_TRACKER_HANDLE;

    tracker->path = dir;
    tracker->path = std::filesystem::absolute(tracker->path);
    tracker->path = std::filesystem::canonical(tracker->path);
    std::string str = tracker->path.string();
    std::wstring wstr(str.begin(), str.end());

    tracker->change_hdir = CreateFileW(
        wstr.c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        NULL
    );

    ZeroMemory(&tracker->change_overlapped, sizeof(tracker->change_overlapped));
    tracker->change_overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    BOOL success = ReadDirectoryChangesW(
        tracker->change_hdir,
        &tracker->change_buffer[0],
        CHANGE_BUFFER_SIZE,
        TRUE,
        FILE_NOTIFY_CHANGE_LAST_WRITE,
        &tracker->change_dwbytes,
        &tracker->change_overlapped,
        NULL
    );

    return tracker;
}

void fileTrackerCleanup(FILE_TRACKER_HANDLE* h) {
    CloseHandle(h->change_hdir);
    delete h;
}

void fileTrackerUpdate(FILE_TRACKER_HANDLE* tracker) {
    DWORD dwWaitStatus = WaitForSingleObject(tracker->change_overlapped.hEvent, 0);
    if (dwWaitStatus == WAIT_OBJECT_0) {
        LOG("Change detected in '" << tracker->path.string() << "'");

        FILE_NOTIFY_INFORMATION* inf = nullptr;
        int entry_offset = 0;
        do {
            inf = (FILE_NOTIFY_INFORMATION*)&tracker->change_buffer[entry_offset];

            if (inf->Action == FILE_ACTION_MODIFIED) {
                std::wstring wstr(inf->FileName, inf->FileNameLength / sizeof(wchar_t));
                std::string str(wstr.begin(), wstr.end());
                LOG("Modified: " << str);
            }
            entry_offset += inf->NextEntryOffset;
        } while(inf->NextEntryOffset > 0);

        ResetEvent(tracker->change_overlapped.hEvent);
        BOOL success = ReadDirectoryChangesW(
            tracker->change_hdir,
            &tracker->change_buffer[0],
            CHANGE_BUFFER_SIZE,
            TRUE,
            FILE_NOTIFY_CHANGE_LAST_WRITE,
            &tracker->change_dwbytes,
            &tracker->change_overlapped,
            NULL
        );
    } else if(dwWaitStatus != WAIT_TIMEOUT) {
        LOG_ERR("Wait for change notification failed: 0x" << std::hex << dwWaitStatus);
    }
}