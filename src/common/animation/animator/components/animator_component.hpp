#pragma once

#include "animation/animator/animator_sync_group.hpp"

class animAnimatorComponent {
public:
    virtual ~animAnimatorComponent() {}

    virtual void onUpdate(animAnimatorSyncGroup* group, float cursor_from, float cursor_to) = 0;
};