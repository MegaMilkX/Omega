#include "resource_entry.hpp"



uint32_t ResourceEntry::nextResourceEntryId() {
    static uint32_t next = 0;
    return ++next;
}

