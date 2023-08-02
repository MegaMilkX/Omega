#pragma once

#include "handle/hshared.hpp"


class IMaster {
public:
    virtual ~IMaster() {}
};

template<typename INSTANCE_T>
class IMasterT : public IMaster {
public:
    using instance_t = INSTANCE_T;

    HSHARED<instance_t> createInstance() = 0;
    void destroyInstance(HSHARED<instance_t> instance) = 0;
};