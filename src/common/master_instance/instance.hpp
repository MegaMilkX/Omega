#pragma once

#include "handle/hshared.hpp"

class IInstance {
public:
    virtual ~IInstance() {}
};

template<typename MASTER_T>
class IInstanceT : public IInstance {
public:
    using master_t = MASTER_T;

    RHSHARED<master_t> getMaster() = 0;
};