#pragma once

#include <assert.h>
#include <vector>


class TransformDirtyList;
struct TransformTicket {
    TransformDirtyList* list = nullptr;
    TransformTicket* next = nullptr;
    void* user_ptr = nullptr;
    int index = -1;

    void markDirty();
};

class TransformDirtyList {
    std::vector<TransformTicket*> list;
    int dirty_count = 0;
public:
    virtual ~TransformDirtyList() {}

    TransformTicket* createTicket(void* user_ptr);
    void destroyTicket(TransformTicket* t);

    void markDirty(TransformTicket* t);

    void clearDirty() {
        dirty_count = 0;
    }
    int dirtyCount() const {
        return dirty_count;
    }
    int totalCount() const {
        return list.size();
    }
    TransformTicket* getDirty(int i) {
        return list[i];
    }
};

template<typename USER_T>
class TransformDirtyList_T : public TransformDirtyList {
public:
    TransformTicket* createTicket(USER_T* user_ptr) {
        return TransformDirtyList::createTicket(user_ptr);
    }
};



