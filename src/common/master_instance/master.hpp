#pragma once

#include "handle/hshared.hpp"


class Master {
public:
    virtual ~Master() {}
};

template<typename INSTANCE_T>
class MasterT : public Master {
public:
    typedef INSTANCE_T instance_t;

    RHSHARED<instance_t> createInstance() = 0;
    void destroyInstance(RHSHARED<instance_t> instance) = 0;
};