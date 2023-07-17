#pragma once

#include "handle/hshared.hpp"

class Instance {
public:
    virtual ~Instance() {}
};

template<typename MASTER_T>
class InstanceT : public Instance {
public:
    typedef MASTER_T master_t;

    RHSHARED<master_t> getMaster() = 0;
};