#include "dirty_system.hpp"

#include <assert.h>
#include "log/log.hpp"
#include "util/timer.hpp"


void IDirty::markDirty() {
    if(is_dirty) return;
    if (sys) {
        sys->markDirty(dirty_idx);
    }
    is_dirty = true;
}
void IDirty::resolveDirty() {
    if(!is_dirty) return;
    if (sys) {
        sys->clearDirty(dirty_idx);
    }
    onResolveDirty();
    is_dirty = false;
}

void DirtySystem::addObject(IDirty* obj) {
    obj->dirty_idx = objects.size();
    objects.push_back(obj);
    obj->sys = this;
    if (obj->is_dirty) {
        int at = obj->dirty_idx;
        std::swap(objects[at], objects[dirty_count]);
        objects[dirty_count]->dirty_idx = dirty_count;
        objects[at]->dirty_idx = at;
        ++dirty_count;
    }
}
void DirtySystem::removeObject(IDirty* obj) {
    if (obj->sys != this) {
        assert(false);
        return;
    }
    int at = obj->dirty_idx;
    objects.erase(objects.begin() + at);
    for (int i = at; i < dirty_count; ++i) {
        objects[i]->dirty_idx = i;
    }
}
void DirtySystem::markDirty(int at) {
    if(objects[at]->is_dirty) {
        assert(false);
        return;
    }
    std::swap(objects[at], objects[dirty_count]);
    objects[dirty_count]->dirty_idx = dirty_count;
    objects[at]->dirty_idx = at;
    ++dirty_count;
}
void DirtySystem::clearDirty(int at) {
    if (!objects[at]->is_dirty) {
        assert(false);
        return;
    }
    --dirty_count;
    std::swap(objects[at], objects[dirty_count]);
    objects[dirty_count]->dirty_idx = dirty_count;
    objects[at]->dirty_idx = at;
}
void DirtySystem::update() {
    if (dirty_count == 0) {
        return;
    }

    timer timer_;
    timer_.start();
    for (int i = 0; i < dirty_count; ++i) {
        objects[i]->onResolveDirty();
        objects[i]->is_dirty = false;
    }
    dirty_count = 0;
    LOG_DBG("DirtySystem update in " << timer_.stop() * 1000.f << "ms");
}

