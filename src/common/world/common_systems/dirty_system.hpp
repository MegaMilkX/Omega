#pragma once

#include "dirty_system.auto.hpp"
#include <vector>

class DirtySystem;

[[cppi_class, no_reflect]];
class IDirty {
    friend DirtySystem;

    DirtySystem* sys = nullptr;
    int dirty_idx = 0;
    bool is_dirty = true;

public:
    virtual ~IDirty() {}
    virtual void onResolveDirty() = 0;

    void markDirty();
    void resolveDirty();
};

[[cppi_class, no_reflect]];
class DirtySystem {
    std::vector<IDirty*> objects;
    int dirty_count = 0;
public:
    void addObject(IDirty*);
    void removeObject(IDirty*);
    void markDirty(int);
    void clearDirty(int);
    void update();
};