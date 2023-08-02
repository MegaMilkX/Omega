#pragma once

#include <stdint.h>

typedef uint32_t entity_id;
constexpr entity_id ENTITY_NULL = UINT32_MAX;

class ecsRegistry {
public:
    entity_id entityCreate();
    void entityDestroy(entity_id id);
    
    void componentCreate(entity_id id);
};